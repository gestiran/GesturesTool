#include <cerrno>
#include <libinput.h>
#include <libudev.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstdio>

static int open_restricted(const char *path, int flags, void *) {
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *) {
    close(fd);
}

static const libinput_interface interface = {
    open_restricted,
    close_restricted,
};

int main() {
    struct udev *udev = udev_new();

    struct libinput *li = libinput_udev_create_context(&interface, nullptr, udev);
    libinput_udev_assign_seat(li, "seat0");

    int fd = libinput_get_fd(li); // fd for poll/epoll

    while (true) {
        struct pollfd pfd = {fd, POLLIN, 0};
        poll(&pfd, 1, -1);

        libinput_dispatch(li);

        struct libinput_event *ev;
        while ((ev = libinput_get_event(li)) != nullptr) {
            switch (libinput_event_get_type(ev)) {
                case LIBINPUT_EVENT_TOUCH_DOWN: {
                    auto *t = libinput_event_get_touch_event(ev);
                    int slot = libinput_event_touch_get_slot(t); // Finger ID
                    double x = libinput_event_touch_get_x(t); // Offsets
                    double y = libinput_event_touch_get_y(t);
                    double nx = libinput_event_touch_get_x_transformed(t, 1920);
                    double ny = libinput_event_touch_get_y_transformed(t, 1080);
                    printf("TOUCH_DOWN  slot=%d  x=%.1f y=%.1f\n", slot, x, y);
                    break;
                }
                case LIBINPUT_EVENT_TOUCH_MOTION: {
                    auto *t = libinput_event_get_touch_event(ev);
                    int slot = libinput_event_touch_get_slot(t);
                    double x = libinput_event_touch_get_x(t);
                    double y = libinput_event_touch_get_y(t);
                    printf("TOUCH_MOVE  slot=%d  x=%.1f y=%.1f\n", slot, x, y);
                    break;
                }
                case LIBINPUT_EVENT_TOUCH_UP: {
                    auto *t = libinput_event_get_touch_event(ev);
                    int slot = libinput_event_touch_get_slot(t);
                    printf("TOUCH_UP    slot=%d\n", slot);
                    break;
                }
                case LIBINPUT_EVENT_TOUCH_FRAME:
                    printf("TOUCH_FRAME\n");
                    break;

                case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN: {
                    auto *g = libinput_event_get_gesture_event(ev);
                    int fingers = libinput_event_gesture_get_finger_count(g);
                    printf("SWIPE_BEGIN  fingers=%d\n", fingers);
                    break;
                }
                case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE: {
                    auto *g = libinput_event_get_gesture_event(ev);
                    double dx = libinput_event_gesture_get_dx(g);
                    double dy = libinput_event_gesture_get_dy(g);
                    double dx_unaccel = libinput_event_gesture_get_dx_unaccelerated(g);
                    double dy_unaccel = libinput_event_gesture_get_dy_unaccelerated(g);
                    printf("SWIPE_UPDATE dx=%.2f dy=%.2f\n", dx, dy);
                    break;
                }
                case LIBINPUT_EVENT_GESTURE_SWIPE_END: {
                    auto *g = libinput_event_get_gesture_event(ev);
                    bool cancelled = libinput_event_gesture_get_cancelled(g);
                    printf("SWIPE_END  cancelled=%d\n", cancelled);
                    break;
                }
                case LIBINPUT_EVENT_GESTURE_PINCH_UPDATE: {
                    auto *g = libinput_event_get_gesture_event(ev);
                    double scale = libinput_event_gesture_get_scale(g); // 1.0
                    double angle = libinput_event_gesture_get_angle_delta(g);
                    printf("PINCH scale=%.3f angle=%.1f\n", scale, angle);
                    break;
                }

                default:
                    break;
            }

            libinput_event_destroy(ev);
        }
    }

    libinput_unref(li);
    udev_unref(udev);
}
