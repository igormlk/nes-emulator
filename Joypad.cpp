//
// Created by Ygor on 09/01/2018.
//

#include <cstdint>
#include "Joypad.h"
#include "Screen.h"


namespace Joypad
{

    uint8_t joypad_bits[2];
    bool strobe;


    uint8_t readState(int n)
    {
        if(strobe)
            return 0x40 | (Screen::getJoypadState(n) & 1);

        uint8_t j = 0x40 | (joypad_bits[n] & 1);
        joypad_bits[n] = 0x80 | (joypad_bits[n] >> 1);

        return j;
    }


    void writeStrobe(bool v)
    {
        if(strobe and !v)
            for(int i = 0; i < 2; i++)
                joypad_bits[i] = Screen::getJoypadState(i);

        strobe = v;
    }



}