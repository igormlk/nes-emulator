//
// Created by Ygor on 26/12/2017.
//

#include "Mapper.h"

Mapper::Mapper(uint8_t *rom) : rom(rom)
{
    prgSize = rom[4] * 0x4000;
    chrSize = rom[5] * 0x2000;
    prgRamSize = rom[8] ? rom[8] * 0x2000 : 0x2000;

    //set mirror gpu

    this->prg = rom+16;
    this->prgRam = new uint8_t[prgRamSize];

    if(chrSize)
        this->chr = rom + 16 + prgSize;
    else
    {
        chrRam = true;
        chrSize = 0x2000;
        this->chr = new uint8_t[chrSize];
    }


}

Mapper::~Mapper()
{
    delete rom;
    delete prgRam;
    if(chrRam)
        delete chr;
}

template<int pageKBS>
void Mapper::map_prg(int slot, int bank)
{

    if (bank < 0)
        bank = (prgSize / (0x400*pageKBS)) + bank;

    for (int i = 0; i < (pageKBS/8); i++)
        prgMap[(pageKBS/8) * slot + i] = (pageKBS*0x400*bank + 0x2000*i) % prgSize;

}

template<int pageKBS>
void Mapper::map_chr(int slot, int bank)
{
    for (int i = 0; i < pageKBS; i++)
        chrMap[pageKBS*slot + i] = (pageKBS*0x400*bank + 0x400*i) % chrSize;
}

uint8_t Mapper::readPrgROM(uint16_t addr)
{
    if (addr >= 0x8000)
        return prg[prgMap[(addr - 0x8000) / 0x2000] + ((addr - 0x8000) % 0x2000)];
    else
        return prgRam[addr - 0x6000];
}

uint8_t Mapper::readChrROM(uint16_t addr)
{
    return chr[chrMap[addr / 0x400] + (addr % 0x400)];
}

template void Mapper::map_prg<32>(int, int);
template void Mapper::map_prg<16>(int, int);
template void Mapper::map_prg<8> (int, int);

template void Mapper::map_chr<8>(int, int);
template void Mapper::map_chr<4>(int, int);
template void Mapper::map_chr<2>(int, int);
template void Mapper::map_chr<1>(int, int);
