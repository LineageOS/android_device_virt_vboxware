#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include <libtablet2multitouch.h>

#define LOG_TAG "vboxtablet2multitouch"

#ifdef DEBUG
#define LOG_ERROR(...) fprintf(stderr, LOG_TAG ": " __VA_ARGS__)
#define LOG_INFO(...) fprintf(stdout, LOG_TAG ": " __VA_ARGS__)
#else
#include <cutils/klog.h>
#define LOG_ERROR(...) KLOG_ERROR(LOG_TAG, __VA_ARGS__)
#define LOG_INFO(...) KLOG_INFO(LOG_TAG, __VA_ARGS__)
#endif

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(bits) (((bits) + BITS_PER_LONG - 1) / BITS_PER_LONG)

static bool test_bit(size_t bit, unsigned long* array) {
    return (array[bit / BITS_PER_LONG] & (1UL << (bit % BITS_PER_LONG))) != 0;
}

static const struct uinput_setup usetup_multitouch = {
        .id =
                {
                        .bustype = BUS_VIRTUAL,
                        .vendor = 0xCAFE,
                        .product = 0x7110,
                },
        .name = "vboxware-vboxtablet2multitouch",
};

static const struct uinput_setup usetup_mouse = {
        .id =
                {
                        .bustype = BUS_VIRTUAL,
                        .vendor = 0xCAFE,
                        .product = 0x7111,
                },
        .name = "vboxware-vboxtablet2multitouch-mouse",
};

static int mouse_setup_uinput_device(void) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        LOG_ERROR("Failed to open /dev/uinput\n");
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
    ioctl(fd, UI_SET_KEYBIT, BTN_SIDE);
    ioctl(fd, UI_SET_KEYBIT, BTN_EXTRA);

    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);

    ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_POINTER);

    if (ioctl(fd, UI_DEV_SETUP, &usetup_mouse) < 0) {
        LOG_ERROR("ioctl(UI_DEV_SETUP) failed\n");
        close(fd);
        return -1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        LOG_ERROR("ioctl(UI_DEV_CREATE) failed\n");
        close(fd);
        return -1;
    }

    return fd;
}

int main() {
    bool use_mouse = false;
    char buf[64];
    fd_set read_fds;
    int fd_tablet = -1, fd_mouse = -1, tmp_fd, uinput_fd, mouse_uinput_fd;
    ssize_t sz = 0;
    struct input_absinfo abs_x_info, abs_y_info;
    struct input_event event;
    unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];

    // Find source input devices
    for (int i = 0; i < 10; ++i) {
        snprintf(buf, sizeof(buf), "/dev/input/event%d", i);
        tmp_fd = open(buf, O_RDONLY | O_NONBLOCK);
        if (tmp_fd < 0) continue;

        ioctl(tmp_fd, EVIOCGNAME(sizeof(buf)), buf);
        if (!strcmp(buf, "VMware VMware Virtual USB Mouse")) {
            if (ioctl(tmp_fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) != -1) {
                if (test_bit(EV_ABS, ev_bits)) {
                    LOG_INFO("Found source VMware tablet input device\n");
                    fd_tablet = tmp_fd;
                } else if (test_bit(EV_REL, ev_bits)) {
                    LOG_INFO("Found source VMware mouse input device\n");
                    fd_mouse = tmp_fd;
                }
            }
        } else if (!strcmp(buf, "ImExPS/2 Generic Explorer Mouse")) {
            LOG_INFO("Found source VirtualBox mouse input device\n");
            fd_mouse = tmp_fd;
        } else if (!strcmp(buf, "VirtualBox mouse integration")) {
            LOG_INFO("Found source VirtualBox tablet input device\n");
            fd_tablet = tmp_fd;
        }

        if (fd_tablet >= 0 && fd_mouse >= 0) goto device_found;
        if (fd_tablet != tmp_fd && fd_mouse != tmp_fd) close(tmp_fd);
    }

    if (fd_mouse < 0) {
        LOG_ERROR("Missing source mouse input device\n");
        return EXIT_SUCCESS;
    }

device_found:
    if (fd_tablet >= 0) {
        // Read ABS_X and ABS_Y info from the source device
        if (ioctl(fd_tablet, EVIOCGABS(ABS_X), &abs_x_info) < 0) {
            LOG_ERROR("ioctl EVIOCGABS(ABS_X)\n");
            return EXIT_FAILURE;
        }

        if (ioctl(fd_tablet, EVIOCGABS(ABS_Y), &abs_y_info) < 0) {
            LOG_ERROR("ioctl EVIOCGABS(ABS_Y)\n");
            return EXIT_FAILURE;
        }

        uinput_fd = libtablet2multitouch_setup_uinput_device(&usetup_multitouch, &abs_x_info,
                                                             &abs_y_info);
        if (uinput_fd < 0) {
            LOG_ERROR("Failed to setup uinput device\n");
            return EXIT_FAILURE;
        }
    }

    mouse_uinput_fd = mouse_setup_uinput_device();
    if (mouse_uinput_fd < 0) {
        LOG_ERROR("Failed to setup mouse uinput device\n");
        if (fd_tablet >= 0) {
            ioctl(uinput_fd, UI_DEV_DESTROY);
            close(uinput_fd);
        }
        return EXIT_FAILURE;
    }

    // Receive and process events
    tmp_fd = (fd_tablet > fd_mouse ? fd_tablet : fd_mouse) + 1;
    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(fd_mouse, &read_fds);
        if (fd_tablet >= 0) FD_SET(fd_tablet, &read_fds);

        if (select(tmp_fd, &read_fds, NULL, NULL, NULL) > 0) {
            if (fd_tablet >= 0 && FD_ISSET(fd_tablet, &read_fds)) {
                sz = read(fd_tablet, &event, sizeof(event));
                if (sz == sizeof(event)) {
                    if (event.type == EV_ABS || event.type == EV_SYN) {
                        use_mouse = false;
                        libtablet2multitouch_handle_event(uinput_fd, &event);
                    }
                } else if (sz < 0 && errno != EAGAIN) {
                    LOG_ERROR("Failed to read tablet input event\n");
                    break;
                }
            }
            if (FD_ISSET(fd_mouse, &read_fds)) {
                sz = read(fd_mouse, &event, sizeof(event));
                if (sz == sizeof(event)) {
                    if (fd_tablet >= 0 && !use_mouse &&
                        (event.type == EV_KEY || event.type == EV_SYN)) {
                        libtablet2multitouch_handle_event(uinput_fd, &event);
                    } else {
                        if (event.type == EV_REL) use_mouse = true;
                        if (use_mouse && write(mouse_uinput_fd, &event, sizeof(event)) < 0) {
                            LOG_ERROR("Failed to forward mouse input event\n");
                            break;
                        }
                    }
                } else if (sz < 0 && errno != EAGAIN) {
                    LOG_ERROR("Failed to read mouse input event\n");
                    break;
                }
            }
        }
    }

    ioctl(mouse_uinput_fd, UI_DEV_DESTROY);
    close(mouse_uinput_fd);
    if (fd_tablet >= 0) {
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
        close(fd_tablet);
    }
    close(fd_mouse);

    return EXIT_SUCCESS;
}
