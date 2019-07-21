//
// Created by Ygor on 27/12/2017.
//

#ifndef NESEMULATOR_MAPPER0_H
#define NESEMULATOR_MAPPER0_H

#include "Mapper.h"

class Mapper0 : public Mapper
{

public:
    Mapper0(uint8_t * rom) : Mapper(rom)
    {
        map_prg<32>(0,0);
        map_chr<8>(0,0);
    }
};


#endif //NESEMULATOR_MAPPER0_H
