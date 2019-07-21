//
// Created by Ygor on 26/12/2017.
//

#include <cstdio>
#include "Cartucho.h"
#include "Mapper.h"
#include "Mapper0.h"
#include "PPU.h"
#include "CPU.h"
#include "APU.h"

namespace Cartucho
{

    Mapper * mapper = nullptr;

    /*
     * PRG ROM ACCESS
     */
    template<bool wr>
    uint8_t access(uint16_t addr, uint8_t v)
    {

        if(!wr)
            return mapper->readPrgROM(addr);
        else
            return mapper->writePrgROM(addr,v);

    }

    /*
     * CHR_ROM/RAM ACCESS
     */
    template<bool wr>
    uint8_t chr_access(uint16_t addr, uint8_t v)
    {
        if(!wr)
            return mapper->readChrROM(addr);
        else
            return mapper->writeChrROM(addr,v);
    }



    void signalScanLine()
    {
        mapper->signalScanLine();
    }

    void load(const char *fileName)
    {

        FILE * file = fopen(fileName,"rb");

        fseek(file, 0, SEEK_END);
        int size = ftell(file);
        fseek(file,0,SEEK_SET);

        uint8_t * rom = new uint8_t[size];

        fread(rom,size,1,file);

        fclose(file);

        int mapperNum = ((rom[7] & 0xF0) | (rom[6] >> 4));

        switch(mapperNum)
        {
            case 0:
                mapper = new Mapper0(rom);
                break;
        }

        CPU::power();
        PPU::reset();
        APU::init();
    }

    bool isLoaded()
    {
        return mapper != nullptr;
    }

    template uint8_t access<0>(uint16_t,uint8_t);
    template uint8_t access<1>(uint16_t,uint8_t);

    template uint8_t chr_access<0>(uint16_t,uint8_t);
    template uint8_t chr_access<1>(uint16_t,uint8_t);


}