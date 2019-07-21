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

    Mirroring mirroring;       // Mirroring mode.
    uint8_t ciRam[0x800];           // VRAM for nametables.
    uint8_t cgRam[0x20];            // VRAM for palettes.
    uint8_t oamMem[0x100];          // VRAM for sprite properties.
    Sprite oam[8], secOam[8];  // Sprite buffers.
    uint32_t pixels[256 * 240];     // Video buffer.

    Address vAddr, tAddr;  // Loopy V, T.
    uint8_t fX;              // Fine X.
    uint8_t oamAddr;         // OAM address.

    Controle ctrl;      // PPUCTRL   ($2000) register.
    Mask mask;      // PPUMASK   ($2001) register.
    Status status;  // PPUSTATUS ($2002) register.

// Background latches:
    uint8_t nt, at, bgL, bgH;
// Background shift registers:
    uint8_t atShiftL, atShiftH;
    uint16_t bgShiftL, bgShiftH;
    bool atLatchL, atLatchH;

// Rendering counters:
    int scanline, dot;
    bool frameOdd;

    inline bool rendering()
    {
        return mask.background || mask.sprite;
    }

    inline int spr_height()
    {
        return ctrl.spriteSize ? 16 : 8;
    }

    uint16_t nt_mirror(uint16_t addr)
    {
        switch (mirroring)
        {
            case VERTICAL:
                return addr % 0x800;
            case HORIZONTAL:
                return ((addr / 2) & 0x400) + (addr % 0x400);
            default:
                return addr - 0x2000;
        }
    }

    void set_mirroring(Mirroring mode)
    {
        mirroring = mode;
    }

/* Access PPU memory */
    uint8_t rd(uint16_t addr)
    {
        switch (addr)
        {
            case 0x0000 ... 0x1FFF:
                return Cartucho::chr_access<false>(addr); //CHR-ROM/RAM
            case 0x2000 ... 0x3EFF:
                return ciRam[ntMirror(addr)];       //NameTables

            case 0x3F00 ... 0x3FFF:
                if ((addr & 0x13) == 0x10)
                    addr &= ~0x10;
                return cgRam[addr & 0x1F] & (mask.grayScale ? 0x30 : 0xFF);

            default:
                return 0;
        }
    }

    void wr(uint16_t addr, uint8_t v)
    {
        switch (addr)
        {
            case 0x0000 ... 0x1FFF:
                Cartucho::chr_access<1>(addr, v);
                break;  // CHR-ROM/RAM.

            case 0x2000 ... 0x3EFF:
                ciRam[ntMirror(addr)] = value;
                break;

            case 0x3F00 ... 0x3FFF:
                if ((addr & 0x13) == 0x10)
                    addr &= ~0x10;

                cgRam[addr & 0x1F] = value;
                break;
        }
    }

/* Access PPU through registers. */
    template<bool write>
    uint8_t access(uint16_t index, uint8_t v)
    {
        static uint8_t res;      // Result of the operation.
        static uint8_t buffer;   // VRAM read buffer.
        static bool latch;  // Detect second reading.

        /* Write into register */
        if (write)
        {
            res = v;

            switch (index)
            {
                case 0:
                    ctrl.r = v;
                    tAddr.nameTableSelect = ctrl.nametable;
                    break;       // PPUCTRL   ($2000).
                case 1:
                    mask.r = v;
                    break;                           // PPUMASK   ($2001).
                case 3:
                    oamAddr = v;
                    break;                          // OAMADDR   ($2003).
                case 4:
                    oamMem[oamAddr++] = v;
                    break;                // OAMDATA   ($2004).
                case 5:                                               // PPUSCROLL ($2005).
                    if (!latch)
                    {
                        fX = v & 7;
                        tAddr.coarseX = v >> 3;
                    }
                    else
                    {
                        tAddr.fY = v & 7;
                        tAddr.coarseY = v >> 3;
                    }      // Second write.
                    latch = !latch;
                    break;
                case 6:                                               // PPUADDR   ($2006).
                    if (!latch)
                    {
                        tAddr.h = v & 0x3F;
                    }                 // First write.
                    else
                    {
                        tAddr.l = v;
                        vAddr.r = tAddr.r;
                    }     // Second write.
                    latch = !latch;
                    break;
                case 7:
                    wr(vAddr.addr, v);
                    vAddr.addr += ctrl.increment ? 32 : 1;  // PPUDATA ($2007).
            }
        }
            /* Read from register */
        else
            switch (index)
            {
                // PPUSTATUS ($2002):
                case 2:
                    res = (res & 0x1F) | status.r;
                    status.vBlank = 0;
                    latch = 0;
                    break;
                case 4:
                    res = oamMem[oamAddr];
                    break;  // OAMDATA ($2004).

                case 7:

                    if (vAddr.addr <= 0x3EFF)
                    {
                        res = buffer;
                        buffer = rd(vAddr.addr);
                    }
                    else
                        res = buffer = rd(vAddr.addr);

                    vAddr.addr += ctrl.increment ? 32 : 1;
                    break;
            }
        return res;
    }

    template uint8_t access<0>(uint16_t, uint8_t);
    template uint8_t access<1>(uint16_t, uint8_t);

/* Calculate graphics addresses */
    inline uint16_t nt_addr()
    {
        return 0x2000 | (vAddr.r & 0x0FFF);
    }

    inline uint16_t at_addr()
    {
        return 0x23C0 | (vAddr.nameTableSelect << 10) | ((vAddr.coarseY / 4) << 3) | (vAddr.coarseX / 4);
    }

    inline uint16_t bg_addr()
    {
        return (ctrl.bgPatternTable * 0x1000) + (nt * 16) + vAddr.fY;
    }

/* Increment the scroll by one pixel */
    inline void h_scroll()
    {
        if (!rendering())
            return;
        if (vAddr.coarseX == 31)
            vAddr.r ^= 0x41F;
        else
            vAddr.coarseX++;
    }

    inline void v_scroll()
    {
        if(!rendering())
            return;
        if (vAddr.fY < 7)
            vAddr.fY++;
        else
        {
            vAddr.fY = 0;
            if (vAddr.coarseY == 31)
                vAddr.coarseY = 0;
            else if (vAddr.coarseY == 29)
            {
                vAddr.coarseY = 0;
                vAddr.nameTableSelect ^= 0b10;
            }
            else
                vAddr.coarseY++;
        }
    }

/* Copy scrolling data from loopy T to loopy V */
    inline void h_update()
    {
        if (!rendering()) return;
        vAddr.r = (vAddr.r & ~0x041F) | (tAddr.r & 0x041F);
    }

    inline void v_update()
    {
        if (!rendering()) return;
        vAddr.r = (vAddr.r & ~0x7BE0) | (tAddr.r & 0x7BE0);
    }

/* Put new data into the shift registers */
    inline void reload_shift()
    {
        bgShiftL = (bgShiftL & 0xFF00) | bgL;
        bgShiftH = (bgShiftH & 0xFF00) | bgH;

        atLatchL = (at & 1);
        atLatchH = (at & 2);
    }

/* Clear secondary OAM */
    void clear_oam()
    {
        for (int i = 0; i < 8; i++)
        {
            secOam[i].id = 64;
            secOam[i].y = 0xFF;
            secOam[i].tile = 0xFF;
            secOam[i].attr = 0xFF;
            secOam[i].x = 0xFF;
            secOam[i].dataL = 0;
            secOam[i].dataH = 0;
        }
    }

/* Fill secondary OAM with the sprite infos for the next scanline */
    void eval_sprites()
    {
        int n = 0;
        for (int i = 0; i < 64; i++)
        {
            int line = (scanline == 261 ? -1 : scanline) - oamMem[i * 4 + 0];
            // If the sprite is in the scanline, copy its properties into secondary OAM:
            if (line >= 0 and line < spr_height())
            {
                secOam[n].id = i;
                secOam[n].y = oamMem[i * 4 + 0];
                secOam[n].tile = oamMem[i * 4 + 1];
                secOam[n].attr = oamMem[i * 4 + 2];
                secOam[n].x = oamMem[i * 4 + 3];

                if (++n > 8)
                {
                    status.spriteOverflow = true;
                    break;
                }
            }
        }
    }

/* Load the sprite info into primary OAM and fetch their tile data. */
    void load_sprites()
    {
        uint16_t addr;
        for (int i = 0; i < 8; i++)
        {
            oam[i] = secOam[i];  // Copy secondary OAM into primary.

            // Different address modes depending on the sprite height:
            if (spr_height() == 16)
                addr = ((oam[i].tile & 1) * 0x1000) + ((oam[i].tile & ~1) * 16);
            else
                addr = (ctrl.spritePatternTable * 0x1000) + (oam[i].tile * 16);

            unsigned sprY = (scanline - oam[i].y) % spr_height();  // Line inside the sprite.

            if (oam[i].attr & 0x80)
                sprY ^= spr_height() - 1;

            // Vertical flip.
            addr += sprY + (sprY & 8);  // Select the second tile if on 8x16.

            oam[i].dataL = rd(addr + 0);
            oam[i].dataH = rd(addr + 8);
        }
    }

/* Process a pixel, draw it if it's on screen */
    void pixel()
    {
        uint8_t palette = 0, objPalette = 0;
        bool objPriority = 0;
        int x = dot - 2;

        if (scanline < 240 and x >= 0 and x < 256)
        {
            if (mask.background and not(!mask.bgLeft && x < 8))
            {
                // Background:
                palette = (NTH_BIT(bgShiftH, 15 - fX) << 1) | NTH_BIT(bgShiftL, 15 - fX);
                if (palette)
                    palette |= ((NTH_BIT(atShiftH, 7 - fX) << 1) | NTH_BIT(atShiftL, 7 - fX)) << 2;
            }
            // Sprites:
            if (mask.sprite and not(!mask.spriteLeft && x < 8))
                for (int i = 7; i >= 0; i--)
                {
                    if (oam[i].id == 64)
                        continue;  // Void entry.

                    unsigned sprX = x - oam[i].x;

                    if (sprX >= 8)
                        continue;

                    if (oam[i].attr & 0x40)
                        sprX ^= 7;  // Horizontal flip.

                    uint8_t sprPalette = (NTH_BIT(oam[i].dataH, 7 - sprX) << 1) | NTH_BIT(oam[i].dataL, 7 - sprX);

                    if (sprPalette == 0)
                        continue;  // Transparent pixel.

                    if (oam[i].id == 0 && palette && x != 255)
                        status.spriteHit = true;

                    sprPalette |= (oam[i].attr & 3) << 2;
                    objPalette = sprPalette + 16;
                    objPriority = oam[i].attr & 0x20;
                }

            // Evaluate priority:
            if (objPalette && (palette == 0 || objPriority == 0))
                palette = objPalette;

            pixels[scanline * 256 + x] = nesRgb[rd(0x3F00 + (rendering() ? palette : 0))];
        }
        // Perform background shifts:
        bgShiftL <<= 1;
        bgShiftH <<= 1;
        atShiftL = (atShiftL << 1) | atLatchL;
        atShiftH = (atShiftH << 1) | atLatchH;
    }

/* Execute a cycle of a scanline */
    template<Scanline s>
    void scanline_cycle()
    {
        static uint16_t addr;

        if (s == NMI and dot == 1)
        {
            status.vBlank = true;
            if (ctrl.nmiIntEnable)
                CPU::set_nmi(true);
        }
        else if (s == POST and dot == 0)
            Screen::newFrame(pixels);
        else if (s == VISIBLE or s == PRE)
        {
            // Sprites:
            switch (dot)
            {
                case 1:
                    clear_oam();
                    if (s == PRE)
                    {
                        status.spriteOverflow = status.spriteHit = false;
                    }
                    break;
                case 257:
                    eval_sprites();
                    break;
                case 321:
                    load_sprites();
                    break;
            }
            // Background:
            switch (dot)
            {
                case 2 ... 255:
                case 322 ... 337:
                    pixel();
                    switch (dot % 8)
                    {
                        // Nametable:
                        case 1:
                            addr = nt_addr();
                            reload_shift();
                            break;
                        case 2:
                            nt = rd(addr);
                            break;
                            // Attribute:
                        case 3:
                            addr = at_addr();
                            break;
                        case 4:
                            at = rd(addr);
                            if (vAddr.coarseY & 2)
                                at >>= 4;
                            if (vAddr.coarseX & 2)
                                at >>= 2;
                            break;
                            // Background (low bits):
                        case 5:
                            addr = bg_addr();
                            break;
                        case 6:
                            bgL = rd(addr);
                            break;
                            // Background (high bits):
                        case 7:
                            addr += 8;
                            break;
                        case 0:
                            bgH = rd(addr);
                            h_scroll();
                            break;
                    }
                    break;
                case 256:
                    pixel();
                    bgH = rd(addr);
                    v_scroll();
                    break;  // Vertical bump.
                case 257:
                    pixel();
                    reload_shift();
                    h_update();
                    break;  // Update horizontal position.
                case 280 ... 304:
                    if (s == PRE)
                        v_update();
                    break;  // Update vertical position.

                    // No shift reloading:
                case 1:
                    addr = nt_addr();
                    if (s == PRE)
                        status.vBlank = false;
                    break;
                case 321:
                case 339:
                    addr = nt_addr();
                    break;
                    // Nametable fetch instead of attribute:
                case 338:
                    nt = rd(addr);
                    break;
                case 340:
                    nt = rd(addr);
                    if (s == PRE && rendering() && frameOdd)
                        dot++;
            }
            // Signal scanline to mapper:
            if (dot == 260 && rendering()) Cartucho::signalScanLine();
        }
    }

/* Execute a PPU cycle. */
    void step()
    {
        switch (scanline)
        {
            case 0 ... 239:
                scanline_cycle<VISIBLE>();
                break;
            case 240:
                scanline_cycle<POST>();
                break;
            case 241:
                scanline_cycle<NMI>();
                break;
            case 261:
                scanline_cycle<PRE>();
                break;
        }
        // Update dot and scanline counters:
        if (++dot > 340)
        {
            dot %= 341;
            if (++scanline > 261)
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

        memset(pixels, 0x00, sizeof(pixels));
        memset(ciRam, 0xFF, sizeof(ciRam));
        memset(oamMem, 0x00, sizeof(oamMem));
    }


}