//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2017—2020 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include "synth.h"

//#include <lol/noise> // lol::perlin_noise
#include <algorithm> //std::min, std::max

namespace z8
{

#ifdef _NDS
__attribute__((section(".itcm"), long_call))
#endif
fix32 synth::waveform(int instrument, fix32 advance)
{
    //const from picolove:
    //local function note_to_hz(note)
	//  return 440 * 2 ^ ((note - 33) / 12)
    //end
    //local tscale = note_to_hz(63) / __sample_rate
    constexpr fix32 tscale = 0.11288053831187;

    fix32 t = fix32::modf(advance);
    fix32 ret = 0;

    // Multipliers were measured from PICO-8 WAV exports. Waveforms are
    // inferred from those exports by guessing what the original formulas
    // could be.
    switch (instrument)
    {
        case INST_TRIANGLE:
            return fix32(0.5) * (fix32::abs(fix32(4) * t - fix32(2)) - fix32(1));
        case INST_TILTED_SAW:
        {
            constexpr fix32 a = 0.9;
            ret = t < a ? fix32(2) * t / a - fix32(1)
                        : fix32(2) * (fix32(1) - t) / (fix32(1) - a) - fix32(1);
            return ret * fix32(0.5);
        }
        case INST_SAW:
            return fix32(0.653) * (t < fix32(0.5) ? t : t - fix32(1));
        case INST_SQUARE:
            return t < fix32(0.5) ? fix32(0.25) : fix32(-0.25);
        case INST_PULSE:
            return t < fix32(1) / fix32(3) ? fix32(0.25) : fix32(-0.25);
        case INST_ORGAN:
            ret = t < fix32(0.5) ? fix32(3) - fix32::abs(fix32(24) * t - fix32(6))
                                 : fix32(1) - fix32::abs(fix32(16) * t - fix32(12));
            return ret / fix32(9);
        case INST_NOISE:
        {
            // Spectral analysis indicates this is some kind of brown noise,
            // but losing almost 10dB per octave. I thought using Perlin noise
            // would be fun, but it’s definitely not accurate.
            //
            // This may help us create a correct filter:
            // http://www.firstpr.com.au/dsp/pink-noise/

            //TODO: not even doing zepto 8 noise here

            //static lol::perlin_noise<1> noise;
            //for (fix32 m = fix32(1.75), d = fix32(1); m <= 128; m *= fix32(2.25), d *= fix32(0.75))
            //    ret += d * noise.eval(lol::vec_t<fix32, 1>(m * advance));

            //ret = (fix32(rand()) / fix32(RAND_MAX));

            //return ret * fix32(0.4);

            //picolove noise function in lua
            //zepto8 phi == picolove oscpos (x parameter in picolove generator func, advance in synth.cpp waveform function)
            //-- noise
            //osc[6] = function()
            //    local lastx = 0
            //    local sample = 0
            //    local lsample = 0
            //    local tscale = note_to_hz(63) / __sample_rate
            //1,041.8329
            //    return function(x)
            //        local scale = (x - lastx) / tscale
            //        lsample = sample
            //        sample = (lsample + scale * (math.random() * 2 - 1)) / (1 + scale)
            //        lastx = x
            //        return math.min(math.max((lsample + sample) * 4 / 3 * (1.75 - scale), -1), 1) *
            //            0.7
            //    end
            //end
            //printf("tscale: %f\n", tscale);
            //printf("advance: %f\n", advance);

            fix32 scale = (advance - lastadvance) / tscale;
            //printf("scale: %f\n", scale);
            lsample = sample;
            sample = (lsample + scale * ((fix32(rand()) / fix32(RAND_MAX)) * fix32(2) - fix32(1))) / (fix32(1) + scale);
            //printf("sample: %f\n", sample);
            lastadvance = advance;
            fix32 endval = fix32::min(fix32::max((lsample + sample) * fix32(4) / fix32(3) * (fix32(1.75) - scale), fix32(-1)), fix32(1)) * fix32(0.2);
            //printf("endval: %f\n", endval);
            return endval;
        }
        case INST_PHASER:
        {   // This one has a subfrequency of freq/128 that appears
            // to modulate two signals using a triangle wave
            // FIXME: amplitude seems to be affected, too
            fix32 k = fix32::abs(fix32(2) * fix32::modf(advance / fix32(128)) - fix32(1));
            fix32 u = fix32::modf(t + fix32(0.5) * k);
            ret = fix32::abs(fix32(4) * u - fix32(2)) - fix32::abs(fix32(8) * t - fix32(4));
            return ret / fix32(6);
        }
    }

    return fix32(0);
}

} // namespace z8
