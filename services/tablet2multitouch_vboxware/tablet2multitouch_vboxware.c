#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libtablet2multitouch.h>

#define LOG_TAG "tablet2multitouch_vboxware"

#ifdef DEBUG
#define LOG_ERROR(...) fprintf(stderr, LOG_TAG ": " __VA_ARGS__)
#define LOG_INFO(...) fprintf(stdout, LOG_TAG ": " __VA_ARGS__)
#else
#include <cutils/klog.h>
#define LOG_ERROR(...) KLOG_ERROR(LOG_TAG, __VA_ARGS__)
#define LOG_INFO(...) KLOG_INFO(LOG_TAG, __VA_ARGS__)
#endif

int main() {
    int fd_abs, fd_key, tmp_fd, uinput_fd;
    ssize_t sz = 0;
    struct input_absinfo abs_x_info, abs_y_info;
    struct input_event event;
    char cur_idev_name[64];
    fd_set read_fds;

    // Find the evdev device path
    for (int i = 0; i < 64; ++i) {
        snprintf(device_path, sizeof(device_path), "/dev/input/event%d", i);
        tmp_fd = open(device_path, O_RDONLY | O_NONBLOCK);
        if (tmp_fd < 0) continue;

        ioctl(tmp_fd, EVIOCGNAME(sizeof(cur_idev_name)), cur_idev_name);

        if (!strcmp(cur_idev_name, "ImExPS/2 Generic Explorer Mouse")) {
            LOG_INFO("Found source input device for EV_KEY\n");
            fd_key = tmp_fd;
        } else if (!strcmp(cur_idev_name, "VirtualBox mouse integration")) {
            LOG_INFO("Found source input device for EV_ABS\n");
            fd_abs = tmp_fd;
        }

        tmp_fd = -1;
        if (fd_abs >= 0 && fd_key >= 0) goto device_found;
    }

    LOG_ERROR("Device(s) not found\n");
    return EXIT_SUCCESS;

device_found:
    // Read ABS_X and ABS_Y info from the source device
    if (ioctl(fd_abs, EVIOCGABS(ABS_X), &abs_x_info) < 0) {
        LOG_ERROR("ioctl EVIOCGABS(ABS_X)\n");
        return EXIT_FAILURE;
    }

    if (ioctl(fd_abs, EVIOCGABS(ABS_Y), &abs_y_info) < 0) {
        LOG_ERROR("ioctl EVIOCGABS(ABS_Y)\n");
        return EXIT_FAILURE;
    }

    // Setup uinput device
    if (libtablet2multitouch_setup_uinput_device(&uinput_fd, &abs_x_info, &abs_y_info) < 0) {
        LOG_ERROR("Failed to setup uinput device\n");
        return EXIT_FAILURE;
    }

    // Receive and process events
    int tmp_fd = (fd_abs > fd_key ? fd_abs : fd_key) + 1;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(fd_abs, &read_fds);
        FD_SET(fd_key, &read_fds);

        if (select(tmp_fd, &read_fds, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(fd_abs, &read_fds)) {
                sz = read(fd, &event, sizeof(event));
                if (sz == sizeof(event)) {
                    if (event.type == EV_ABS) {
                        libtablet2multitouch_handle_event(uinput_fd, &event);
                    }
                } else if (sz < 0 && errno != EAGAIN) {
                    LOG_ERROR("read ABS\n");
                    break;
                }
            }
            if (FD_ISSET(fd_key, &read_fds)) {
                sz = read(fd, &event, sizeof(event));
                if (sz == sizeof(event)) {
                    if (event.type == EV_KEY) {
                        libtablet2multitouch_handle_event(uinput_fd, &event);
                    }
                } else if (sz < 0 && errno != EAGAIN) {
                    LOG_ERROR("read KEY\n");
                    break;
                }
            }
        }
    }

    ioctl(uinput_fd, UI_DEV_DESTROY);
    close(uinput_fd);
    close(fd_abs);
    close(fd_key);

    return EXIT_SUCCESS;
}
