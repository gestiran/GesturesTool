#ifndef GESTURESTOOL_MAIN_H
#define GESTURESTOOL_MAIN_H

#include <string>

struct Finger {
    int x;
    int y;
    int id;
    bool is_updated;
};

std::string find_touchpad();

#endif //GESTURESTOOL_MAIN_H
