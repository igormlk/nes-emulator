//
// Created by Ygor on 08/01/2018.
//

#ifndef NESEMULATOR_SCREEN_H
#define NESEMULATOR_SCREEN_H


#include "Blip_Buffer.h"

namespace Screen
{

    void initScreen();
    void run();
    void newFrame(uint32_t * frame);
    uint8_t getJoypadState(int n);
    void new_samples(const blip_sample_t * samples, size_t count);

}


#endif //NESEMULATOR_SCREEN_H
