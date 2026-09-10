#define LOG_TAG "vmwgfx_detect"

#include <cutils/log.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <vmwgfx_drm.h>
#include <xf86drm.h>

int main() {
    int fd, ret;
    uint32_t value;
    struct drm_vmwgfx_getparam get_param = {
            .value = (uint64_t)(uintptr_t)&value,
    };

    fd = drmOpen("vmwgfx", NULL);
    if (fd < 0) {
        ALOGE("drmOpen() failed");
        return EXIT_FAILURE;
    }

    get_param.param = DRM_VMW_PARAM_3D;
    ret = drmIoctl(fd, DRM_VMW_GET_PARAM, &get_param);
    if (!ret) {
        property_set("ro.vendor.graphics", value ? "mesa" : "swiftshader");
    } else {
        ALOGE("drmIoctl DRM_VMW_GET_PARAM failed");
    }

    close(fd);
    return EXIT_SUCCESS;
}
