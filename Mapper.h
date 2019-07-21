//
// Created by Ygor on 26/12/2017.
//

#ifndef NESEMULATOR_MAPPER_H
#define NESEMULATOR_MAPPER_H

#include <cstring>
#include <cstdint>

class Mapper
{
private:
    uint8_t * rom;
    bool chrRam = false;

protected:
    uint32_t prgMap[4];
    uint32_t chrMap[8];

    uint8_t *prg,*chr,*prgRam;
    uint32_t prgSize,chrSize,prgRamSize;

    template<int pageKBS> void map_prg(int slot, int bank);
    template<int pageKBS> void map_chr(int slot, int bank);

public:

    Mapper(uint8_t * rom);
    ~Mapper();

    uint8_t readPrgROM(uint16_t addr);
    virtual uint8_t writePrgROM(uint16_t addr,uint8_t v){return v;}

    uint8_t readChrROM(uint16_t addr);
    virtual uint8_t writeChrROM(uint16_t addr,uint8_t v){return v;}

    virtual void signalScanLine(){}

};


#endif //NESEMULATOR_MAPPER_H
