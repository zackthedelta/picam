#include <arpa/inet.h>
#include <errno.h>
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

typedef struct {
    GMutex lock;
    GCond updated;
    guint8 *data;
    gsize size;
    guint64 sequence;
    gboolean running;
} FrameStore;

typedef struct {
    FrameStore *frames;
    char host[64];
    unsigned short port;
    int server_fd;
} HttpServer;

typedef struct {
    HttpServer *server;
    int fd;
} Client;

static const char *env_or_default(const char *name, const char *fallback)
{
    const char *value = getenv(name);
    return value && value[0] ? value : fallback;
}

static int env_int_or_default(const char *name, int fallback)
{
    const char *value = getenv(name);
    char *end = NULL;
    long parsed;

    if (!value || !value[0])
        return fallback;

    parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < 0 || parsed > 65535)
        return fallback;

    return (int)parsed;
}

static gboolean send_all(int fd, const void *data, size_t size)
{
    const guint8 *cursor = data;

    while (size > 0) {
        ssize_t sent = send(fd, cursor, size, MSG_NOSIGNAL);

        if (sent < 0) {
            if (errno == EINTR)
                continue;
            return FALSE;
        }

        if (sent == 0)
            return FALSE;

        cursor += sent;
        size -= (size_t)sent;
    }

    return TRUE;
}

static void discard_http_request(int fd)
{
    char buffer[1024];

    (void)recv(fd, buffer, sizeof(buffer), 0);
}

static void *client_thread(void *arg)
{
    Client *client = arg;
    FrameStore *frames = client->server->frames;
    guint64 last_sequence = 0;
    int fd = client->fd;

    free(client);
    discard_http_request(fd);

    if (!send_all(fd,
                  "HTTP/1.1 200 OK\r\n"
                  "Content-Type: multipart/x-mixed-replace; boundary=picam\r\n"
                  "Cache-Control: no-store\r\n"
                  "Pragma: no-cache\r\n"
                  "Connection: close\r\n"
                  "\r\n",
                  strlen("HTTP/1.1 200 OK\r\n"
                         "Content-Type: multipart/x-mixed-replace; boundary=picam\r\n"
                         "Cache-Control: no-store\r\n"
                         "Pragma: no-cache\r\n"
                         "Connection: close\r\n"
                         "\r\n"))) {
        close(fd);
        return NULL;
    }

    for (;;) {
        guint8 *copy = NULL;
        gsize size = 0;
        guint64 sequence;
        char header[160];
        int header_len;

        g_mutex_lock(&frames->lock);
        while (frames->running && frames->sequence == last_sequence)
            g_cond_wait(&frames->updated, &frames->lock);

        if (!frames->running) {
            g_mutex_unlock(&frames->lock);
            break;
        }

        size = frames->size;
        sequence = frames->sequence;
        copy = g_malloc(size);
        memcpy(copy, frames->data, size);
        g_mutex_unlock(&frames->lock);

        header_len = snprintf(header, sizeof(header),
                              "--picam\r\n"
                              "Content-Type: image/jpeg\r\n"
                              "Content-Length: %zu\r\n"
                              "\r\n",
                              size);

        if (header_len < 0 ||
            !send_all(fd, header, (size_t)header_len) ||
            !send_all(fd, copy, size) ||
            !send_all(fd, "\r\n", 2)) {
            g_free(copy);
            break;
        }

        g_free(copy);
        last_sequence = sequence;
    }

    close(fd);
    return NULL;
}

static void *http_server_thread(void *arg)
{
    HttpServer *server = arg;

    while (server->frames->running) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int fd = accept(server->server_fd, (struct sockaddr *)&peer, &peer_len);
        Client *client;
        pthread_t thread;

        if (fd < 0) {
            if (errno == EINTR)
                continue;
            break;
        }

        client = calloc(1, sizeof(*client));
        if (!client) {
            close(fd);
            continue;
        }

        client->server = server;
        client->fd = fd;

        if (pthread_create(&thread, NULL, client_thread, client) == 0)
            pthread_detach(thread);
        else {
            close(fd);
            free(client);
        }
    }

    return NULL;
}

static gboolean start_http_server(HttpServer *server)
{
    struct sockaddr_in address;
    int reuse = 1;

    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        g_printerr("Failed to create MJPEG HTTP socket: %s\n", strerror(errno));
        return FALSE;
    }

    setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(server->port);

    if (inet_pton(AF_INET, server->host, &address.sin_addr) != 1) {
        g_printerr("Invalid MJPEG HTTP host: %s\n", server->host);
        close(server->server_fd);
        return FALSE;
    }

    if (bind(server->server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        g_printerr("Failed to bind MJPEG HTTP server on %s:%u: %s\n",
                   server->host, server->port, strerror(errno));
        close(server->server_fd);
        return FALSE;
    }

    if (listen(server->server_fd, 8) < 0) {
        g_printerr("Failed to listen for MJPEG HTTP clients: %s\n", strerror(errno));
        close(server->server_fd);
        return FALSE;
    }

    return TRUE;
}

static GstFlowReturn on_new_sample(GstAppSink *sink, gpointer user_data)
{
    FrameStore *frames = user_data;
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;
    guint8 *copy;

    sample = gst_app_sink_pull_sample(sink);
    if (!sample)
        return GST_FLOW_OK;

    buffer = gst_sample_get_buffer(sample);
    if (!buffer || !gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    copy = g_malloc(map.size);
    memcpy(copy, map.data, map.size);

    g_mutex_lock(&frames->lock);
    g_free(frames->data);
    frames->data = copy;
    frames->size = map.size;
    frames->sequence++;
    g_cond_broadcast(&frames->updated);
    g_mutex_unlock(&frames->lock);

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

int main(int argc, char *argv[])
{
    FrameStore frames = { 0 };
    HttpServer server = { 0 };
    pthread_t http_thread;
    GMainLoop *loop;
    GstElement *pipeline;
    GstElement *appsink;
    GstBus *bus;
    GError *error = NULL;
    gchar *pipeline_description;
    const char *width = env_or_default("PICAM_MJPEG_WIDTH", "640");
    const char *height = env_or_default("PICAM_MJPEG_HEIGHT", "480");
    const char *framerate = env_or_default("PICAM_MJPEG_FRAMERATE", "10/1");
    const char *quality = env_or_default("PICAM_MJPEG_QUALITY", "75");
    const char *rtp_host = env_or_default("PICAM_RTP_HOST", "127.0.0.1");
    const char *rtp_port = env_or_default("PICAM_RTP_PORT", "5004");
    const char *h264_bitrate = env_or_default("PICAM_H264_BITRATE", "1200");
    const char *h264_key_int_max = env_or_default("PICAM_H264_KEY_INT_MAX", "30");

    signal(SIGPIPE, SIG_IGN);
    gst_init(&argc, &argv);

    g_mutex_init(&frames.lock);
    g_cond_init(&frames.updated);
    frames.running = TRUE;

    g_strlcpy(server.host, env_or_default("PICAM_MJPEG_HOST", "127.0.0.1"),
              sizeof(server.host));
    server.port = (unsigned short)env_int_or_default("PICAM_MJPEG_PORT", 8081);
    server.frames = &frames;

    if (!start_http_server(&server))
        return 1;

    if (pthread_create(&http_thread, NULL, http_server_thread, &server) != 0) {
        g_printerr("Failed to start MJPEG HTTP thread\n");
        close(server.server_fd);
        return 1;
    }

    pipeline_description = g_strdup_printf(
        "libcamerasrc ! "
        "video/x-raw,width=%s,height=%s,framerate=%s ! "
        "tee name=camera "
        "camera. ! queue leaky=downstream max-size-buffers=2 ! videoconvert ! "
        "jpegenc quality=%s ! "
        "appsink name=mjpeg_sink emit-signals=true max-buffers=2 drop=true sync=false "
        "camera. ! queue leaky=downstream max-size-buffers=30 ! videoconvert ! "
        "x264enc tune=zerolatency speed-preset=ultrafast bitrate=%s key-int-max=%s ! "
        "h264parse config-interval=1 ! "
        "rtph264pay config-interval=1 pt=96 ! "
        "udpsink host=%s port=%s sync=false async=false",
        width, height, framerate, quality,
        h264_bitrate, h264_key_int_max, rtp_host, rtp_port);

    pipeline = gst_parse_launch(pipeline_description, &error);
    if (!pipeline) {
        g_printerr("Failed to create shared camera pipeline: %s\n",
                   error ? error->message : "unknown error");
        g_clear_error(&error);
        g_free(pipeline_description);
        return 1;
    }

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mjpeg_sink");
    if (!appsink) {
        g_printerr("Failed to find MJPEG appsink in pipeline\n");
        gst_object_unref(pipeline);
        g_free(pipeline_description);
        return 1;
    }

    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), &frames);
    gst_object_unref(appsink);

    loop = g_main_loop_new(NULL, FALSE);
    bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect_swapped(bus, "message::error", G_CALLBACK(g_main_loop_quit), loop);
    g_signal_connect_swapped(bus, "message::eos", G_CALLBACK(g_main_loop_quit), loop);
    gst_object_unref(bus);

    g_print("Pi camera MJPEG preview ready at http://%s:%u/stream.mjpg\n",
            server.host, server.port);
    g_print("Shared RTSP RTP fanout sending H264 RTP to %s:%s\n", rtp_host, rtp_port);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_mutex_lock(&frames.lock);
    frames.running = FALSE;
    g_cond_broadcast(&frames.updated);
    g_mutex_unlock(&frames.lock);
    close(server.server_fd);

    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
    g_free(pipeline_description);
    g_free(frames.data);
    g_cond_clear(&frames.updated);
    g_mutex_clear(&frames.lock);

    return 0;
}
