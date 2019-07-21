//
// Created by Ygor on 27/12/2017.
//

#include "PPU.h"
#include "Cartucho.h"
#include "pallete.inc"
#include "Screen.h"
#include <cstring>
#include "CPU.h"

#define NTH_BIT(x, n) (((x) >> (n)) & 1)

namespace PPU
{

#define CONTROL_REGISTER 0
#define MASK_REGISTER 1
#define STATUS_REGISTER 2
#define OAMADDRESS_REGISTER 3
#define OAMDATA_REGISTER 4
#define SCROLL_REGISTER 5
#define ADDRESS_REGISTER 6
#define DATA_REGISTER 7

    Mirroring mirroring; //Modo de espelhamento
    uint8_t ciRam[0x800]; //Video ram para nametables
    uint8_t cgRam[0x20];  //Video ram para as palletes
    uint8_t oamMem[0x100]; //Video ram para propriedade das sprites
    Sprite oam[8],secOam[8]; //Sprite Buffer
    uint32_t pixels[256 * 240]; //Video Buffer

    Address vAddr,tAddr; //Loopy V,T
    uint8_t fX;          //Fine X
    uint8_t oamAddr;    //OAM Address

    Controle ctrl;      //0x2000 PPUCTRL REGISTER
    Mask mask;          //0x2001 PPUMASK REGISTER
    Status status;      //0x2002 PPUSTATUS REGISTER

    //Background latches
    uint8_t nt,at,bgL,bgH;
    //background shiftRegisters
    uint8_t atShiftL,atShiftH;uint16_t bgShiftL,bgShiftH;
    bool atLatchL, atLatchH;

    //Rendering counters
    int scanline,dot;
    bool frameOdd;

    inline bool rendering()
    {
        return mask.background || mask.sprite;
    }
    inline int spr_height()
    {
        return ctrl.spriteSize ? 16 : 8;
    }

    uint16_t ntMirror(uint16_t addr)
    {
        switch(mirroring)
        {
            case VERTICAL:
                return addr % 0x800;
            case HORIZONTAL:
                return ((addr / 2) & 0x400) + (addr % 0x400);
            default:
                return addr - 0x2000;
        }
    }

    void setMirroring(Mirroring mode)
    {
        mirroring = mode;
    }

    uint8_t read(uint16_t addr)
    {
        switch(addr)
        {
            case 0x0000 ... 0x1FFF:
                return Cartucho::chr_access<false>(addr); //CHR-ROM/RAM
            case 0x2000 ... 0x3EFF:
                return ciRam[ntMirror(addr)];       //NameTables

            case 0x3F00 ... 0x3FFF:
                if((addr & 0x13) == 0x10)
                    addr &= ~0x10;
                return cgRam[addr & 0x1F] & (mask.grayScale ? 0x30 : 0xFF);

            default:
                return 0;
        }

    }


    void write(uint16_t addr, uint8_t value)
    {
        switch (addr)
        {
            case 0x0000 ... 0x1FFF:
                Cartucho::chr_access<true>(addr,value);
                break;

            case 0x2000 ... 0x3EFF:
                ciRam[ntMirror(addr)] = value;
                break;

            case 0x3F00 ... 0x3FFF:
                if((addr & 0x13) == 0x10)
                    addr &= ~0x10;

                cgRam[addr & 0x1F] = value;
                break;

        }
    }


    template <bool wr> uint8_t access(uint16_t addr, uint8_t v)
    {
        static uint8_t res;      // Result of the operation.
        static uint8_t buffer;   // VRAM read buffer.
        static bool latch;  // Detect second reading.

        if(wr)
        {
            res = v;

            switch(addr)
            {

                case CONTROL_REGISTER:
                    ctrl.r = v;
                    tAddr.nameTableSelect = ctrl.nametable;
                    break;
                case MASK_REGISTER:
                    mask.r = v;
                    break;
                case OAMADDRESS_REGISTER:
                    oamAddr = v;
                    break;
                case OAMDATA_REGISTER:
                    oamMem[oamAddr++] = v;
                    break;
                case SCROLL_REGISTER:
                    if (!latch)
                    {
                        fX = v & 7;
                        tAddr.coarseX = v >> 3;
                    }
                    else
                    {
                        tAddr.fY = v & 7;
                        tAddr.coarseY = v >> 3;
                    }      // Second wr.
                    latch = !latch;
                    break;

                case ADDRESS_REGISTER:
                    if(!latch)
                    {
                        tAddr.h = v & 0x3F;
                    }
                    else
                    {
                        tAddr.l = v;
                        vAddr.r = tAddr.r;
                    }
                    latch = !latch;
                    break;
                case 7:
                    write(vAddr.addr, v);
                    vAddr.addr += ctrl.increment ? 32 : 1;  // PPUDATA ($2007).
            }
        }
        else
        {
            switch(addr)
            {

                case STATUS_REGISTER:
                    res = (res & 0x1F) | status.r;
                    status.vBlank = 0;
                    latch = false;
                    break;

                case OAMDATA_REGISTER:
                    res = oamMem[oamAddr];
                    break;

                case DATA_REGISTER:

                    if(vAddr.addr <= 0x3EFF)
                    {
                        res = buffer;
                        buffer = read(vAddr.addr);
                    }
                    else
                        res = buffer = read(vAddr.addr);

                    vAddr.addr += ctrl.increment ? 32 : 1;
                    break;

            }

        }
        return res;
    }

    template uint8_t access<false>(uint16_t addr,uint8_t v);
    template uint8_t access<true>(uint16_t addr,uint8_t v);

    inline uint16_t getNameTableAddr()
    {
        return 0x2000 | (vAddr.r & 0x0FFF);
    }

    inline uint16_t getAttributeAddr()
    {
        return 0x23C0 | (vAddr.nameTableSelect << 10) | ((vAddr.coarseY / 4) << 3) | (vAddr.coarseX / 4);
    }

    inline uint16_t getBackgroundAddr()
    {
        return (ctrl.bgPatternTable * 0x1000) + (nt * 16) + vAddr.fY;
    }

    inline void scrollH()
    {
        if (!rendering())
            return;
        if(vAddr.coarseX == 31)
            vAddr.r ^= 0x41F;
        else
            vAddr.coarseX++;
    }

    inline void scrollV()
    {
        if(!rendering())
            return;
        if(vAddr.fY < 7)
            vAddr.fY++;
        else
        {
            vAddr.fY = 0;
            if(vAddr.coarseY == 31)
                vAddr.coarseY = 0;
            else if(vAddr.coarseY == 29)
            {
                vAddr.coarseY = 0;
                vAddr.nameTableSelect ^= 0b10;
            }
            else
                vAddr.coarseY++;
        }
    }

    inline void updateHScroll()
    {
        if(!rendering())
            return;

        vAddr.r = (vAddr.r & ~0x041F) | (tAddr.r & 0x041F);

    }

    inline void updateVScroll()
    {
        if(!rendering())
            return;

        vAddr.r = (vAddr.r & ~0x7BE0) | (tAddr.r & 0x7BE0);

    }

    inline void reload_shift()
    {
        bgShiftL = (bgShiftL & 0xFF00) | bgL;
        bgShiftH = (bgShiftH & 0xFF00) | bgH;

        atLatchL = (at & 1);
        atLatchH = (at & 2);
    }

    void clearOAM()
    {
        for(int i = 0; i < 8; i++)
        {
            secOam[i].id    = 64;
            secOam[i].y     = 0xFF;
            secOam[i].tile  = 0xFF;
            secOam[i].attr  = 0xFF;
            secOam[i].x     = 0xFF;
            secOam[i].dataL = 0;
            secOam[i].dataH = 0;
        }
    }

    void evaluateSprites()
    {
        int n = 0;
        for(int i = 0; i < 64; i++)
        {
            int line = (scanline == 261 ?  -1 : scanline ) - oamMem[i * 4 + 0];

            if (line >= 0 and line < spr_height())
            {
                secOam[n].id = i;
                secOam[n].y = oamMem[i * 4 + 0];
                secOam[n].tile = oamMem[i * 4 + 1];
                secOam[n].attr = oamMem[i * 4 + 2];
                secOam[n].x = oamMem[i * 4 + 3];


                if(++n > 8)
                {
                    status.spriteOverflow = true;
                    break;
                }

            }

        }
    }

    void loadSprites()
    {
        uint16_t addr;
        for(int i = 0; i < 8; i++)
        {
            oam[i] = secOam[i];  // Copy secondary OAM into primary.

            // Different address modes depending on the sprite height:
            if(spr_height() == 16)
                addr = ((oam[i].tile & 1) * 0x1000) + ((oam[i].tile & ~1) * 16);
            else
                addr = (ctrl.spritePatternTable * 0x1000) + (oam[i].tile * 16);

            unsigned sprY = (scanline - oam[i].y) % spr_height();  // Line inside the sprite.

            if(oam[i].attr  & 0x80)
                sprY ^= spr_height() - 1;

            // Vertical flip.
            addr += sprY + (sprY & 8);  // Select the second tile if on 8x16.

            oam[i].dataL = read(addr + 0);
            oam[i].dataH = read(addr + 8);
        }
    }


    void pixel()
    {
        uint8_t palette = 0, objPalette = 0;
        bool objPriority = 0;
        int x = dot - 2;

        if(scanline < 240 and x >= 0 and x < 256)
        {
            if(mask.background and not (!mask.bgLeft && x < 8))
            {
                // Background:
                palette = (NTH_BIT(bgShiftH, 15 - fX) << 1) | NTH_BIT(bgShiftL, 15 - fX);
                if (palette)
                    palette |= ((NTH_BIT(atShiftH, 7 - fX) << 1) | NTH_BIT(atShiftL, 7 - fX)) << 2;
            }

            // Sprites

            if(mask.sprite and not (!mask.spriteLeft && x < 8))
                for(int i = 7; i >= 0; i--)
                {
                    if (oam[i].id == 64)
                        continue;  // Void entry.

                    unsigned sprX = x - oam[i].x;

                    if (sprX >= 8)
                        continue;

                    if(oam[i].attr & 0x40)
                        sprX ^= 7;  // Horizontal flip.

                    uint8_t sprPalette = (NTH_BIT(oam[i].dataH,7 - sprX) << 1) | NTH_BIT(oam[i].dataL,7 - sprX);

                    if (sprPalette == 0)
                        continue;  // Transparent pixel.

                    if (oam[i].id == 0 && palette && x != 255)
                        status.spriteHit = true;

                    sprPalette |= (oam[i].attr & 3) << 2;
                    objPalette= sprPalette + 16;
                    objPriority = oam[i].attr & 0x20;
                }

                //evaluate priority
                if (objPalette && (palette == 0 || objPriority == 0))
                    palette = objPalette;

                pixels[scanline * 256 + x] = nesRgb[read(0x3F00 + (rendering() ? palette : 0))];

        }
        // Perform background shifts:
            bgShiftL <<= 1;
            bgShiftH <<= 1;
            atShiftL = (atShiftL << 1) | atLatchL;
            atShiftH = (atShiftH << 1) | atLatchH;


    }


    template<Scanline s> void scanLineCycle()
    {
        static uint16_t addr;

        if(s == NMI and dot == 1)
        {
            status.vBlank = true;
            if(ctrl.nmiIntEnable)
                CPU::set_nmi(true);
        }
        else if(s == POST and dot == 0)
           Screen::newFrame(pixels);
        else if(s == VISIBLE or s == PRE)
        {
            //Sprites
            switch(dot)
            {
                case 1:
                    clearOAM();
                    if(s == PRE)
                    {
                        status.spriteOverflow = status.spriteHit = false;
                    }
                    break;
                case 257:
                    evaluateSprites();
                    break;
                case 321:
                    loadSprites();
                    break;
            }

            //Background
            switch(dot)
            {
                case 2 ... 255:
                case 322 ... 337:
                    pixel();
                    switch(dot % 8)
                    {
                        //NameTable
                        case 1:
                            addr = getNameTableAddr();
                            reload_shift();
                            break;
                        case 2:
                            nt = read(addr);
                            break;
                            //Attribute
                        case 3:
                            addr = getAttributeAddr();
                            break;
                        case 4:
                            at = read(addr);
                            if(vAddr.coarseY & 2)
                                at >>= 4;
                            if(vAddr.coarseX & 2)
                                at >>= 2;
                            break;
                            //Background low bits
                        case 5:
                            addr = getBackgroundAddr();
                            break;
                        case 6:
                            bgL = read(addr);
                            break;
                            //Background high bits
                        case 7:
                            addr += 8;
                            break;
                        case 0:
                            bgH = read(addr);
                            scrollH();
                            break;
                    }
                    break;

                case 256:
                    pixel();
                    bgH = read(addr);
                    scrollV();
                    break;
                case 257:
                    pixel();
                    reload_shift();
                    updateHScroll();
                    break;
                case 280 ... 304:
                    if(s == PRE)
                        updateVScroll();
                    break;

                    // nao ha shift reloading
                case 1:
                    addr = getNameTableAddr();
                    if(s == PRE)
                        status.vBlank = false;
                    break;
                case 321:
                case 339:
                    addr = getNameTableAddr();
                    break;
                    //nametable fetch instead of attribute
                case 338:
                    nt = read(addr);
                    break;
                case 340:
                    nt = read(addr);
                    if(s == PRE && rendering() && frameOdd)
                        dot++;
            }

            if(dot == 260 && rendering())
                Cartucho::signalScanLine();

        }



    }


    void step()
    {
        switch(scanline)
        {
            case 0 ... 239:
                scanLineCycle<VISIBLE>();
                break;
            case 240:
                scanLineCycle<POST>();
                break;
            case 241:
                scanLineCycle<NMI>();
                break;
            case 261:
                scanLineCycle<PRE>();
                break;
        }


        if(++dot > 340)
        {
            dot %= 341;

            if(++scanline > 261)
            {
                scanline = 0;
                frameOdd ^= 1;
            }

        }

    }


    void reset()
    {
        frameOdd = false;
        scanline = dot = 0;
        ctrl.r = mask.r = status.r = 0;

        memset(pixels,0x00,sizeof(pixels));
        memset(ciRam,0xFF,sizeof(ciRam));
        memset(oamMem,0x00,sizeof(oamMem));

    }


}
