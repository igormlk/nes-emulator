//
// Created by Ygor on 15/01/2018.
//

#ifndef NESEMULATOR_APU_H
#define NESEMULATOR_APU_H

#include <cstdint>

namespace APU
{
    template <bool write> uint8_t access(int elapsed, uint16_t addr, uint8_t value);

    void runFrame(int elapsed);
    void reset();
    void init();


}


#endif //NESEMULATOR_APU_H
