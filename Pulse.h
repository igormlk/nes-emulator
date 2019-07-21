//
// Created by Ygor on 15/01/2018.
//

#ifndef NESEMULATOR_PULSE_H
#define NESEMULATOR_PULSE_H


#include <cstdint>

class Pulse
{
private:

    struct Divider
    {
        uint16_t reload;
        uint16_t counter;
    };

    static const uint8_t SequenceLength = 8;

    const uint8_t duty12[SequenceLength] = {
            0,1,0,0,0,0,0,0 //12.5%
    };

    const uint8_t duty25[SequenceLength] = {
            0,1,1,0,0,0,0,0 // 25%
    };

    const uint8_t duty50[SequenceLength] = {
            0,1,1,1,1,0,0,0
    };

    const uint8_t dutyMinus25[SequenceLength] = {
            1,0,0,1,1,1,1,1
    };

    const uint8_t * dutyCyles[4] {
            duty12,duty25,duty50,dutyMinus25

    };
    const uint8_t lengthTable[32] =
            {10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
             12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

    Divider envelope;
    Divider sweep;
    Divider timer;

    uint8_t duty; // só vai de 0 a 3
    bool envelopeStartFlag = false;
    bool envelopeLoopFlag = false;
    bool envelopeConstantVolumeFlag = false;
    bool counterHaltFlag = false;
    bool sweepEnableFlag = false;
    bool sweepShiftFlag = false;
    bool sweepNegateFlag = false;
    bool sweepReloadFlag = false;
    bool controlFlag = false;
    uint8_t sweepShift;
    uint8_t constantVolume;
    uint8_t lengthCounter;
    uint8_t sequencer;
    uint8_t envelopeVolume;
    bool enable = false;

public:
    void access(uint8_t value, uint16_t addr);
    void setEnable(bool value);
    void TimerClock();

};


#endif //NESEMULATOR_PULSE_H
