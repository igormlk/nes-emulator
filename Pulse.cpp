//
// Created by Ygor on 15/01/2018.
//

#include "Pulse.h"



void Pulse::access(uint8_t value, uint16_t addr)
{

    switch(addr)
    {
        case 0x4000:
        case 0x4004:
            duty = (value >> 6);
            envelopeStartFlag = (value >> 5) & 1;
            envelopeConstantVolumeFlag = (value >> 4) & 1;
            envelope.reload =  value & 0x0F;
            constantVolume = value & 0x0F;
            break;

        case 0x4001:
        case 0x4005:
            sweepEnableFlag = value >> 7;
            sweep.reload = ((value >> 4) & 0x07);
            sweepNegateFlag = (value >> 3) & 0x01;
            sweepShiftFlag = (value & 0x07);
            sweepReloadFlag = true;
            break;

        case 0x4002:
        case 0x4006:
            timer.reload = (timer.reload & 0x700) | value;
            break;

        case 0x4003:
        case 0x4007:
            timer.reload = (timer.reload & 0x0FF) | ((value & 0x07) << 8);
            if(sweepEnableFlag)
                lengthCounter = lengthTable[value >> 3];
            sequencer = 0;
            envelopeStartFlag = true;
            break;
    }


}

void Pulse::setEnable(bool value)
{
    this->enable = value;
    if(!enable)
        lengthCounter = 0;
}

void Pulse::TimerClock()
{

    if(!timer.counter)
    {
        timer.counter = timer.reload;
        sequencer++;
        if(sequencer++)
            if(sequencer == SequenceLength)
                sequencer = 0;
    }
    else
    {
        if(!envelopeStartFlag)
            if(!envelope.counter)
            {
                if(envelopeVolume)
                    envelopeVolume--;
                else if(controlFlag)
                    envelopeVolume = 15;

                envelope.counter = envelope.reload;
            }
            else
                envelope.counter--;
        else
        {
            envelopeVolume = 15;
            envelope.counter = envelope.reload;
            envelopeStartFlag = false;
        }

    }



}
