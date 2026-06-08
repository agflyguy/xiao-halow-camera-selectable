#include "EspCameraStreamer.h"

#include "esp_camera.h"

EspCameraStreamer::EspCameraStreamer(u_short width, u_short height) : CStreamer(width, height) {}

void EspCameraStreamer::streamImage(uint32_t curMsec)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        return;
    }

    if (fb->format == PIXFORMAT_JPEG && fb->len >= 2 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8) {
        streamFrame(fb->buf, fb->len, curMsec);
    }

    esp_camera_fb_return(fb);
}
