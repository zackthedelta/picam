(function () {
  var host = window.location.hostname || "device-ip";
  var rtspUrl = "rtsp://" + host + ":8554/camera";
  var input = document.getElementById("rtspUrl");
  var copyButton = document.getElementById("copyButton");
  var openLink = document.getElementById("openLink");
  var httpHost = document.getElementById("httpHost");
  var hostStatus = document.getElementById("hostStatus");

  input.value = rtspUrl;
  openLink.href = rtspUrl;
  httpHost.textContent = window.location.host || "Port 80";

  copyButton.addEventListener("click", function () {
    function copied() {
      copyButton.textContent = "Copied";
      window.setTimeout(function () {
        copyButton.textContent = "Copy";
      }, 1500);
    }

    if (navigator.clipboard && window.isSecureContext) {
      navigator.clipboard.writeText(rtspUrl).then(copied);
      return;
    }

    input.select();
    document.execCommand("copy");
    input.blur();
    copied();
  });

  fetch("/healthz", { cache: "no-store" })
    .then(function (response) {
      hostStatus.textContent = response.ok ? "Ready" : "HTTP Issue";
    })
    .catch(function () {
      hostStatus.textContent = "Offline";
    });
}());
