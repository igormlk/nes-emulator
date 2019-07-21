//
// Created by Ygor on 27/12/2017.
//

#ifndef NESEMULATOR_PPU_H
#define NESEMULATOR_PPU_H

#include <cstdint>


namespace PPU
{

    enum Scanline
    {
        VISIBLE,
        POST,
        NMI,
        PRE
    };

    enum Mirroring
    {
        VERTICAL,
        HORIZONTAL
    };


    struct Sprite
    {
        uint8_t id;     // numero da sprite na pattern table
        uint8_t x;     // posição x
        uint8_t y;     // posição y
        uint8_t tile;  // indice da tile
        uint8_t attr;  //Atributos da sprite
        uint8_t dataL; //Data Tile Low
        uint8_t dataH; //Data Tile High
    };


    union Controle
    {
        struct
        {
            unsigned nametable : 2; // 0x2000 / 0x2400 / 0x2800 / 0x2C00
            unsigned increment : 1; // incrementar 1 ou 32 no address
            unsigned spritePatternTable : 1; //tabela de sprite padrão para 0x8 sprites =  0x0000 ou 0x1000
            unsigned bgPatternTable : 1; // background tabela padrao de endreços 0x0000 ou 0x1000
            unsigned spriteSize : 1; // tamanho da sprite 0 : 8x8 1 : 8x16
            unsigned ppuMasterSlave : 1; // 0: ler backdrop de pinos externos ; 1 : saida de cores em pinos externos
            unsigned nmiIntEnable : 1; // gerar uma interrupção nmi no inicio de um vblank interval 0 : off 1 : on
        };
        uint8_t r;
    };

    union Mask
    {
        struct
        {
            unsigned grayScale : 1;
            unsigned bgLeft : 1;
            unsigned spriteLeft : 1;
            unsigned background : 1;
            unsigned sprite : 1;
            unsigned red : 1;
            unsigned green : 1;
            unsigned blue : 1;
        };
        uint8_t r;
    };


    union Status
    {
        struct
        {
            unsigned bus : 5; // 5 bits em bus , nao utilizado
            unsigned spriteOverflow : 1; // sprite sobrescreveu
            unsigned spriteHit : 1; // sprite 0 colidiu
            unsigned vBlank : 1; // esta em vblank ?
        };
        uint8_t  r;
    };


    union Address
    {
        struct
        {
            unsigned coarseX : 5;
            unsigned coarseY : 5;
            unsigned nameTableSelect : 2;
            unsigned fY : 3;
        };
        struct
        {
            unsigned l : 8;
            unsigned h : 7;
        };

        unsigned addr : 14;
        unsigned r : 15;

    };


    template<bool wr> uint8_t access(uint16_t index, uint8_t v = 0);
    void setMirroring(Mirroring mode);
    void step();
    void reset();

}


#endif //NESEMULATOR_PPU_H
