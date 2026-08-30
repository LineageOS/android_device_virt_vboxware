/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "tablet2multitouch_vboxware"

#include "libtablet2multitouch.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>

#include <linux/input.h>
#include <sys/select.h>

#include <cerrno>
#include <cstring>
#include <optional>
#include <string>

#include <fcntl.h>
#include <unistd.h>

namespace {

using android::base::unique_fd;
using tablet2multitouch::AbsInfo;
using tablet2multitouch::MouseUinputDevice;
using tablet2multitouch::MouseUinputDeviceConfig;
using tablet2multitouch::TabletEventTranslator;
using tablet2multitouch::UinputDevice;
using tablet2multitouch::UinputDeviceConfig;

constexpr int kMaxDeviceNodes = 10;
constexpr size_t kBitsPerLong = sizeof(unsigned long) * 8;
constexpr size_t kEvMaxLongs = (EV_MAX + kBitsPerLong - 1) / kBitsPerLong;

constexpr char kVMwareMouseName[] = "VMware VMware Virtual USB Mouse";
constexpr char kVBoxMouseName[] = "ImExPS/2 Generic Explorer Mouse";
constexpr char kVBoxTabletName[] = "VirtualBox mouse integration";

bool TestBit(size_t bit, const unsigned long* array) {
    return (array[bit / kBitsPerLong] & (1UL << (bit % kBitsPerLong))) != 0;
}

struct TabletInfo {
    unique_fd fd;
    AbsInfo abs_x;
    AbsInfo abs_y;
};

struct MouseInfo {
    unique_fd fd;
};

std::optional<TabletInfo> FindVMwareTablet(int fd, const char* name) {
    if (strcmp(name, kVMwareMouseName) != 0) {
        return std::nullopt;
    }

    unsigned long ev_bits[kEvMaxLongs] = {};
    if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
        return std::nullopt;
    }

    if (!TestBit(EV_ABS, ev_bits)) {
        return std::nullopt;
    }

    struct input_absinfo abs_x_info = {};
    struct input_absinfo abs_y_info = {};

    if (ioctl(fd, EVIOCGABS(ABS_X), &abs_x_info) < 0) {
        PLOG(ERROR) << "ioctl EVIOCGABS(ABS_X) failed";
        return std::nullopt;
    }

    if (ioctl(fd, EVIOCGABS(ABS_Y), &abs_y_info) < 0) {
        PLOG(ERROR) << "ioctl EVIOCGABS(ABS_Y) failed";
        return std::nullopt;
    }

    TabletInfo info;
    info.abs_x.minimum = abs_x_info.minimum;
    info.abs_x.maximum = abs_x_info.maximum;
    info.abs_y.minimum = abs_y_info.minimum;
    info.abs_y.maximum = abs_y_info.maximum;

    return info;
}

bool IsVMwareMouse(int fd, const char* name) {
    if (strcmp(name, kVMwareMouseName) != 0) {
        return false;
    }

    unsigned long ev_bits[kEvMaxLongs] = {};
    if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
        return false;
    }

    return TestBit(EV_REL, ev_bits);
}

struct SourceDevices {
    std::optional<TabletInfo> tablet;
    std::optional<MouseInfo> mouse;
    int tablet_fd = -1;
    int mouse_fd = -1;
};

SourceDevices FindSourceDevices() {
    SourceDevices result;
    std::vector<unique_fd> fds_to_close;

    for (int i = 0; i < kMaxDeviceNodes; ++i) {
        std::string path = "/dev/input/event" + std::to_string(i);
        unique_fd fd(open(path.c_str(), O_RDONLY | O_NONBLOCK));
        if (!fd.ok()) {
            continue;
        }

        char name_buf[64] = {};
        if (ioctl(fd.get(), EVIOCGNAME(sizeof(name_buf)), name_buf) < 0) {
            fds_to_close.push_back(std::move(fd));
            continue;
        }

        if (!result.tablet.has_value()) {
            auto tablet = FindVMwareTablet(fd.get(), name_buf);
            if (tablet.has_value()) {
                LOG(INFO) << "Found source VMware tablet input device: " << path;
                tablet->fd = std::move(fd);
                result.tablet_fd = tablet->fd.get();
                result.tablet = std::move(tablet);
                continue;
            }

            if (strcmp(name_buf, kVBoxTabletName) == 0) {
                LOG(INFO) << "Found source VirtualBox tablet input device: " << path;

                struct input_absinfo abs_x_info = {};
                struct input_absinfo abs_y_info = {};

                if (ioctl(fd.get(), EVIOCGABS(ABS_X), &abs_x_info) < 0 ||
                    ioctl(fd.get(), EVIOCGABS(ABS_Y), &abs_y_info) < 0) {
                    fds_to_close.push_back(std::move(fd));
                    continue;
                }

                TabletInfo info;
                info.fd = std::move(fd);
                info.abs_x.minimum = abs_x_info.minimum;
                info.abs_x.maximum = abs_x_info.maximum;
                info.abs_y.minimum = abs_y_info.minimum;
                info.abs_y.maximum = abs_y_info.maximum;
                result.tablet_fd = info.fd.get();
                result.tablet = std::move(info);
                continue;
            }
        }

        if (!result.mouse.has_value()) {
            if (IsVMwareMouse(fd.get(), name_buf)) {
                LOG(INFO) << "Found source VMware mouse input device: " << path;
                MouseInfo info;
                info.fd = std::move(fd);
                result.mouse_fd = info.fd.get();
                result.mouse = std::move(info);
                continue;
            }

            if (strcmp(name_buf, kVBoxMouseName) == 0) {
                LOG(INFO) << "Found source VirtualBox mouse input device: " << path;
                MouseInfo info;
                info.fd = std::move(fd);
                result.mouse_fd = info.fd.get();
                result.mouse = std::move(info);
                continue;
            }
        }

        if (result.tablet.has_value() && result.mouse.has_value()) {
            break;
        }

        fds_to_close.push_back(std::move(fd));
    }

    return result;
}

int RunEventLoop(int tablet_fd, int mouse_fd, bool has_tablet, TabletEventTranslator* translator,
                 MouseUinputDevice& mouse_uinput) {
    bool use_mouse = false;

    int max_fd = (has_tablet && tablet_fd > mouse_fd) ? tablet_fd : mouse_fd;
    int nfds = max_fd + 1;

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(mouse_fd, &read_fds);
        if (has_tablet) {
            FD_SET(tablet_fd, &read_fds);
        }

        int ret = select(nfds, &read_fds, nullptr, nullptr, nullptr);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            PLOG(ERROR) << "select() failed";
            return EXIT_FAILURE;
        }

        if (has_tablet && FD_ISSET(tablet_fd, &read_fds)) {
            struct input_event ev;
            ssize_t sz = read(tablet_fd, &ev, sizeof(ev));
            if (sz == sizeof(ev)) {
                if (ev.type == EV_ABS || ev.type == EV_SYN) {
                    use_mouse = false;
                    translator->HandleEvent(ev);
                }
            } else if (sz < 0 && errno != EAGAIN && errno != EINTR) {
                PLOG(ERROR) << "Failed to read tablet input event";
                return EXIT_FAILURE;
            }
        }

        if (FD_ISSET(mouse_fd, &read_fds)) {
            struct input_event ev;
            ssize_t sz = read(mouse_fd, &ev, sizeof(ev));
            if (sz == sizeof(ev)) {
                if (has_tablet && !use_mouse && (ev.type == EV_KEY || ev.type == EV_SYN)) {
                    translator->HandleEvent(ev);
                } else {
                    if (ev.type == EV_REL) {
                        use_mouse = true;
                    }
                    if (use_mouse) {
                        mouse_uinput.ForwardEvent(ev);
                    }
                }
            } else if (sz < 0 && errno != EAGAIN && errno != EINTR) {
                PLOG(ERROR) << "Failed to read mouse input event";
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char** argv) {
#if defined(__ANDROID_RECOVERY__)
    android::base::InitLogging(argv, &android::base::KernelLogger);
#endif

    auto devices = FindSourceDevices();

    if (!devices.mouse.has_value()) {
        LOG(ERROR) << "Missing source mouse input device";
        return EXIT_SUCCESS;
    }

    bool has_tablet = devices.tablet.has_value();
    int tablet_fd = has_tablet ? devices.tablet->fd.get() : -1;
    int mouse_fd = devices.mouse->fd.get();

    UinputDevice uinput;
    TabletEventTranslator* translator = nullptr;

    if (has_tablet) {
#if defined(__ANDROID_RECOVERY__)
        constexpr bool kReportHover = false;
#else
        constexpr bool kReportHover = true;
#endif

        UinputDeviceConfig config;
        config.name = "vboxware-tablet2multitouch";
        config.bustype = BUS_VIRTUAL;
        config.vendor = 0xCAFE;
        config.product = 0x7110;
        config.abs_x = devices.tablet->abs_x;
        config.abs_y = devices.tablet->abs_y;
        config.report_hover = kReportHover;

        if (!uinput.Create(config)) {
            LOG(ERROR) << "Failed to setup uinput device";
            return EXIT_FAILURE;
        }

        translator = new TabletEventTranslator(uinput, kReportHover);
    }

    MouseUinputDeviceConfig mouse_config;
    mouse_config.name = "vboxware-tablet2multitouch-mouse";
    mouse_config.bustype = BUS_VIRTUAL;
    mouse_config.vendor = 0xCAFE;
    mouse_config.product = 0x7111;

    MouseUinputDevice mouse_uinput;
    if (!mouse_uinput.Create(mouse_config)) {
        LOG(ERROR) << "Failed to setup mouse uinput device";
        delete translator;
        return EXIT_FAILURE;
    }

    int result = RunEventLoop(tablet_fd, mouse_fd, has_tablet, translator, mouse_uinput);

    delete translator;
    return result;
}
