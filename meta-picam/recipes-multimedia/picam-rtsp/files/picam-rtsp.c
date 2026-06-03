#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <stdlib.h>

static const char *env_or_default(const char *name, const char *fallback)
{
    const char *value = getenv(name);
    return value && value[0] ? value : fallback;
}

int main(int argc, char *argv[])
{
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    const char *port;
    const char *mount;
    const char *pipeline;

    gst_init(&argc, &argv);

    port = env_or_default("PICAM_RTSP_PORT", "8554");
    mount = env_or_default("PICAM_RTSP_MOUNT", "/camera");
    pipeline = env_or_default(
        "PICAM_RTSP_PIPELINE",
        "( libcamerasrc ! "
        "video/x-raw,width=640,height=480,framerate=15/1 ! "
        "queue ! videoconvert ! "
        "x264enc tune=zerolatency speed-preset=ultrafast bitrate=1200 key-int-max=30 ! "
        "h264parse config-interval=1 ! "
        "rtph264pay name=pay0 pt=96 )");

    loop = g_main_loop_new(NULL, FALSE);
    server = gst_rtsp_server_new();
    g_object_set(server, "service", port, NULL);

    mounts = gst_rtsp_server_get_mount_points(server);
    factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, pipeline);
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, mount, factory);
    g_object_unref(mounts);

    if (gst_rtsp_server_attach(server, NULL) == 0) {
        g_printerr("Failed to attach RTSP server on port %s\n", port);
        return 1;
    }

    g_print("Pi camera RTSP stream ready at rtsp://<device-ip>:%s%s\n", port, mount);
    g_main_loop_run(loop);

    g_object_unref(server);
    g_main_loop_unref(loop);
    return 0;
}
