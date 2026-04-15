#include <libevdev/libevdev.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <stdexcept>
#include "main.h"

int main() {
    std::string path_to_touchpad = find_touchpad();
    printf("Device: %s\n", path_to_touchpad.c_str());

    int file_descriptor = open(path_to_touchpad.c_str(), O_RDONLY);

    if (file_descriptor < 0) {
        perror("open");
        return 1;
    }

    struct libevdev *dev = nullptr;

    if (libevdev_new_from_fd(file_descriptor, &dev) < 0) {
        perror("libevdev_new_from_fd");
        return 1;
    }

    printf("Model: %s\n", libevdev_get_name(dev));

    const struct input_absinfo *ax = libevdev_get_abs_info(dev, ABS_MT_POSITION_X);
    const struct input_absinfo *ay = libevdev_get_abs_info(dev, ABS_MT_POSITION_Y);

    int size_x = ax ? ax->resolution : 0;
    int size_y = ay ? ay->resolution : 0;

    int fingers_count = libevdev_get_num_slots(dev);

    if (fingers_count <= 0) {
        fingers_count = 10;
    }

    printf("Fingers: %d\n\n", fingers_count);

    std::unordered_map<int, Finger> fingers;

    for (int finger_id = 0; finger_id < fingers_count; finger_id++) {
        fingers[finger_id] = {};
    }

    int current_slot = 0;

    struct input_event input;

    while (true) {
        int return_code = libevdev_next_event(dev,LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &input);

        if (return_code == LIBEVDEV_READ_STATUS_SYNC) {
            while (libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &input) == LIBEVDEV_READ_STATUS_SYNC);
            continue;
        }

        if (return_code < 0) {
            break;
        }

        switch (input.type) {
            case EV_ABS:
                switch (input.code) {
                    case ABS_MT_SLOT:
                        current_slot = input.value;
                        break;

                    case ABS_MT_TRACKING_ID:
                        if (input.value == -1) {
                            printf("  [slot %d] FINGER UP (id=%d)\n", current_slot, fingers[current_slot].id);
                            fingers[current_slot].id = -1;
                        } else {
                            fingers[current_slot].id = input.value;
                            printf("  [slot %d] FINGER DOWN (id=%d)\n", current_slot, input.value);
                        }
                        break;

                    case ABS_MT_POSITION_X:
                        fingers[current_slot].x = input.value;
                        fingers[current_slot].is_updated = true;
                        break;

                    case ABS_MT_POSITION_Y:
                        fingers[current_slot].y = input.value;
                        fingers[current_slot].is_updated = true;
                        break;
                }
                break;

            case EV_SYN:
                if (input.code == SYN_REPORT) {
                    bool is_any_changed = false;

                    for (auto &[slot, finger]: fingers) {
                        if (!finger.is_updated || finger.id == -1) {
                            continue;
                        }

                        is_any_changed = true;

                        if (size_x > 0 && size_y > 0) {
                            printf("  [slot %d] MOVE  x=%.1fmm  y=%.1fmm\n", slot, static_cast<double>(finger.x) / size_x, static_cast<double>(finger.y) / size_y);
                        } else {
                            printf("  [slot %d] MOVE  x=%d  y=%d  (raw, unknown resolution)\n", slot, finger.x, finger.y);
                        }

                        finger.is_updated = false;
                    }

                    if (is_any_changed) {
                        int active_fingers_count = 0;

                        for (auto &[slot, finger]: fingers) {
                            if (finger.id != -1) {
                                active_fingers_count++;
                            }
                        }

                        printf("  --- FRAME  active_fingers=%d ---\n", active_fingers_count);
                    }
                }
                break;
        }
    }

    libevdev_free(dev);
    close(file_descriptor);
}

std::string find_touchpad() {
    for (auto &current_input_device: std::filesystem::directory_iterator("/dev/input")) {
        if (!current_input_device.is_character_file()) {
            continue;
        }

        std::string path_to_input_device = current_input_device.path().string();

        if (path_to_input_device.find("event") == std::string::npos) {
            continue;
        }

        int file_descriptor = open(path_to_input_device.c_str(), O_RDONLY | O_NONBLOCK);

        if (file_descriptor < 0) {
            continue;
        }

        struct libevdev *device = nullptr;

        if (libevdev_new_from_fd(file_descriptor, &device) == 0) {
            bool is_touchpad = libevdev_has_event_code(device, EV_KEY, BTN_TOOL_FINGER) && libevdev_has_event_code(device, EV_ABS, ABS_MT_POSITION_X);
            libevdev_free(device);

            if (is_touchpad) {
                close(file_descriptor);
                return path_to_input_device;
            }
        }

        close(file_descriptor);
    }

    throw std::runtime_error("Touchpad not found in /dev/input/");
}