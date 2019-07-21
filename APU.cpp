//
// Created by Ygor on 15/01/2018.
//

#include "APU.h"
#include "Nes_Apu.h"
#include "CPU.h"
#include "Screen.h"

namespace APU
{

    Nes_Apu apu;
    Blip_Buffer buf;


    const int OUT_SIZE = 4096;
    blip_sample_t outBuf[OUT_SIZE];

    void init()
    {
        buf.sample_rate(96000);
        buf.clock_rate(1789773);

        apu.output(&buf);
        apu.dmc_reader(CPU::dmc_read);
    }

    void reset()
    {
        apu.reset();
        buf.clear();
    }

    template <bool write> uint8_t access(int elapsed, uint16_t addr, uint8_t value)
    {
        if(write)
            apu.write_register(elapsed,addr,value);
        else if(addr == apu.status_addr)
            value = apu.read_status(elapsed);

        return value;
    }

    template uint8_t access<false>(int elapsed, uint16_t addr, uint8_t value);
    template uint8_t access<true>(int elapsed, uint16_t addr, uint8_t value);

    void runFrame(int elapsed)
    {
        apu.end_frame(elapsed);
        buf.end_frame(elapsed);

        if(buf.samples_avail() >= OUT_SIZE)
            Screen::new_samples(outBuf,buf.read_samples(outBuf,OUT_SIZE));

    }

}
