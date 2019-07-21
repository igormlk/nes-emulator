//
// Created by Ygor on 09/01/2018.
//

#ifndef NESEMULATOR_JOYPAD_H
#define NESEMULATOR_JOYPAD_H


#include <SDL.h>

namespace Joypad
{

    uint8_t readState(int n);
    void writeStrobe(bool v);
}

#endif //NESEMULATOR_JOYPAD_H
