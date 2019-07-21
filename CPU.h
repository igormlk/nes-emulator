#pragma once
#include <cstdint>
#include "Nes_Apu.h"

namespace CPU {

#define NTH_BIT(x, n) (((x) >> (n)) & 1)

    enum IntType { NMI, RESET, IRQ, BRK };  // Interrupt type.
    typedef uint16_t (*Mode)(void);              // Addressing mode.

/* Processor flags */
    enum Flag {C, Z, I, D, V, N};
    class Flags
    {
        bool f[6];

    public:
        bool& operator[] (const int i) { return f[i]; }

        uint8_t get() { return f[C] | f[Z] << 1 | f[I] << 2 | f[D] << 3 | 1 << 5 | f[V] << 6 | f[N] << 7; }
        void set(uint8_t p) { f[C] = NTH_BIT(p, 0); f[Z] = NTH_BIT(p, 1); f[I] = NTH_BIT(p, 2);
            f[D] = NTH_BIT(p, 3); f[V] = NTH_BIT(p, 6); f[N] = NTH_BIT(p, 7); }
    };

    void set_nmi(bool v = true);
    void set_irq(bool v = true);
    int dmc_read(void*, cpu_addr_t addr);
    void power();
    void run_frame();


}