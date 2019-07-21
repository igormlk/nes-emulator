//
// Created by Ygor on 26/12/2017.
//

#ifndef NESEMULATOR_CARTUCHO_H
#define NESEMULATOR_CARTUCHO_H

#include <cstdint>

namespace Cartucho
{

    template<bool wr> uint8_t  access(uint16_t addr,uint8_t v = 0);
    template<bool wr> uint8_t chr_access(uint16_t addr,uint8_t v = 0);
    void signalScanLine();
    void load(const char * fileName);
    bool isLoaded();

}

#endif //NESEMULATOR_CARTUCHO_H
