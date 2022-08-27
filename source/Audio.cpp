#include "Audio.h"
#include "synth.h"
#include "hostVmShared.h"
#include "mathhelpers.h"

using z8::fix32;


//playback implemenation based on zetpo 8's
//https://github.com/samhocevar/zepto8/blob/master/src/pico8/sfx.cpp

Audio::Audio(PicoRam* memory, int numChannels){
    _numChannels = numChannels;
    _memory = memory;
    
    resetAudioState();
}

void Audio::resetAudioState() {
    for(int i = 0; i < _numChannels; i++) {
        _audioState._sfxChannels[i].sfxId = -1;
        _audioState._sfxChannels[i].offset = 0;
        _audioState._sfxChannels[i].phi = 0;
        _audioState._sfxChannels[i].can_loop = true;
        _audioState._sfxChannels[i].is_music = false;
        _audioState._sfxChannels[i].prev_key = 0;
        _audioState._sfxChannels[i].prev_vol = 0;
    }
    _audioState._musicChannel.count = 0;
    _audioState._musicChannel.pattern = -1;
    _audioState._musicChannel.master = -1;
    _audioState._musicChannel.mask = 0xf;
    _audioState._musicChannel.speed = 0;
    _audioState._musicChannel.volume = 0;
    _audioState._musicChannel.volume_step = 0;
    _audioState._musicChannel.offset = 0;
}

audioState* Audio::getAudioState() {
    return &_audioState;
}

void Audio::api_sfx(int sfx, int channel, int offset){

    if (sfx < -2 || sfx > 63 || channel < -1 || channel > 3 || offset > 31) {
        return;
    }

    if (sfx == -1)
    {
        // Stop playing the current channel
        if (channel != -1) {
            _audioState._sfxChannels[channel].sfxId = -1;
        }
    }
    else if (sfx == -2)
    {
        // Stop looping the current channel
        if (channel != -1) {
            _audioState._sfxChannels[channel].can_loop = false;
        }
    }
    else
    {
        // Find the first available channel: either a channel that plays
        // nothing, or a channel that is already playing this sample (in
        // this case PICO-8 decides to forcibly reuse that channel, which
        // is reasonable)
        if (channel == -1)
        {
            for (int i = 0; i < _numChannels; ++i)
                if (_audioState._sfxChannels[i].sfxId == -1 ||
                    _audioState._sfxChannels[i].sfxId == sfx)
                {
                    channel = i;
                    break;
                }
        }

        // If still no channel found, the PICO-8 strategy seems to be to
        // stop the sample with the lowest ID currently playing
        if (channel == -1)
        {
            for (int i = 0; i < _numChannels; ++i) {
               if (channel == -1 || _audioState._sfxChannels[i].sfxId < _audioState._sfxChannels[channel].sfxId) {
                   channel = i;
               }
            }
        }

        // Stop any channel playing the same sfx
        for (int i = 0; i < _numChannels; ++i) {
            if (_audioState._sfxChannels[i].sfxId == sfx) {
                _audioState._sfxChannels[i].sfxId = -1;
            }
        }

        // Play this sound!
        _audioState._sfxChannels[channel].sfxId = sfx;
        _audioState._sfxChannels[channel].offset = std::max(0, offset);
        _audioState._sfxChannels[channel].phi = 0;
        _audioState._sfxChannels[channel].can_loop = true;
        _audioState._sfxChannels[channel].is_music = false;
        // Playing an instrument starting with the note C-2 and the
        // slide effect causes no noticeable pitch variation in PICO-8,
        // so I assume this is the default value for “previous key”.
        _audioState._sfxChannels[channel].prev_key = 24;
        // There is no default value for “previous volume”.
        _audioState._sfxChannels[channel].prev_vol = 0.f;
    }      
}

void Audio::api_music(int pattern, int16_t fade_len, int16_t mask){
    if (pattern < -1 || pattern > 63) {
        return;
    }

    if (pattern == -1)
    {
        // Music will stop when fade out is finished
        _audioState._musicChannel.volume_step = fade_len <= 0 ? fix32(-10000) // TODO: min number
                                  : -_audioState._musicChannel.volume * (fix32(1000) / fix32(fade_len));
        return;
    }

    _audioState._musicChannel.count = 0;
    _audioState._musicChannel.mask = mask ? mask & 0xf : 0xf;

    _audioState._musicChannel.volume = 1;
    _audioState._musicChannel.volume_step = 0;
    if (fade_len > 0)
    {
        _audioState._musicChannel.volume = 0;
        _audioState._musicChannel.volume_step = 1000 / fade_len;
    }

    set_music_pattern(pattern);
}

void Audio::set_music_pattern(int pattern) {
    _audioState._musicChannel.pattern = pattern;
    _audioState._musicChannel.offset = 0;

    //array to access song's channels. may be better to have this part of the struct?
    uint8_t channels[] = {
        _memory->songs[pattern].getSfx0(),
        _memory->songs[pattern].getSfx1(),
        _memory->songs[pattern].getSfx2(),
        _memory->songs[pattern].getSfx3(),
    };

    // Find music speed; it’s the speed of the fastest sfx
	// While we are looping through this, find the lowest *valid* sfx length
    _audioState._musicChannel.master = _audioState._musicChannel.speed = -1;
	_audioState._musicChannel.length = 32; //as far as i can tell, there is no way to make an sfx longer than 32.
    for (int i = 0; i < _numChannels; ++i)
    {
        uint8_t n = channels[i];

        if (n & 0x40)
            continue;

        auto &sfx = _memory->sfx[n & 0x3f];
        if (_audioState._musicChannel.master == -1 || _audioState._musicChannel.speed > sfx.speed)
        {
            _audioState._musicChannel.master = i;
            _audioState._musicChannel.speed = std::max(1, (int)sfx.speed);
        }
		
		if(sfx.loopRangeStart != 0 && sfx.loopRangeEnd == 0){
			_audioState._musicChannel.length = std::min(_audioState._musicChannel.length, fix32(sfx.loopRangeStart));
		}
    }

    // Play music sfx on active channels
    for (int i = 0; i < _numChannels; ++i)
    {
        if (((1 << i) & _audioState._musicChannel.mask) == 0)
            continue;

        uint8_t n = channels[i];
        if (n & 0x40)
            continue;

        _audioState._sfxChannels[i].sfxId = n;
        _audioState._sfxChannels[i].offset = 0;
        _audioState._sfxChannels[i].phi = 0;
        _audioState._sfxChannels[i].can_loop = false;
        _audioState._sfxChannels[i].is_music = true;
        _audioState._sfxChannels[i].prev_key = 24;
        _audioState._sfxChannels[i].prev_vol = 0;
    }
}

void Audio::FillAudioBuffer(void *audioBuffer, size_t offset, size_t size){
    if (audioBuffer == nullptr) {
        return;
    }

    uint32_t *buffer = (uint32_t *)audioBuffer;

    for (size_t i = 0; i < size; ++i){
        int_fast16_t sample = 0;

        for (int c = 0; c < _numChannels; ++c) {
            sample += this->getSampleForChannel(c);
        }

        //buffer is stereo, so just send the mono sample to both channels
        buffer[i] = (sample<<16) | (sample & 0xffff);
    }
}

void Audio::FillMonoAudioBuffer(void *audioBuffer, size_t offset, size_t size){
    if (audioBuffer == nullptr) {
        return;
    }

    int16_t *buffer = (int16_t *)audioBuffer;

    for (size_t i = 0; i < size; ++i){
        int_fast16_t sample = 0;

        for (int c = 0; c < _numChannels; ++c) {
            sample += this->getSampleForChannel(c);
        }

        buffer[i] = sample;
    }
}

static fix32 key_to_freq(uint_fast8_t key)
{
    return fix32(440) * (fix32(1) << (((int)key - 33) / 12));
}

int16_t Audio::getCurrentSfxId(int channel){
    return _audioState._sfxChannels[channel].sfxId;
}

int Audio::getCurrentNoteNumber(int channel){
    return _audioState._sfxChannels[channel].sfxId < 0
        ? -1
        : (int)_audioState._sfxChannels[channel].offset;
}

int16_t Audio::getCurrentMusic(){
    return _audioState._musicChannel.pattern;
}

int16_t Audio::getMusicPatternCount(){
    return _audioState._musicChannel.count;
}

int16_t Audio::getMusicTickCount(){
    return _audioState._musicChannel.offset * fix32(_audioState._musicChannel.speed);
}

//adapted from zepto8 sfx.cpp (wtfpl license)
#ifdef _NDS
__attribute__((section(".itcm"), long_call))
#endif
int16_t Audio::getSampleForChannel(int channel){
    if (_audioState._sfxChannels[channel].sfxId == -1) return 0;

    constexpr fix32 samples_per_second = 22050;

    int16_t sample = 0;

    const int_fast16_t index = _audioState._sfxChannels[channel].sfxId;
 
    // Advance music using the master channel
    if (channel == _audioState._musicChannel.master && _audioState._musicChannel.pattern != -1)
    {
        fix32 const offset_per_second = samples_per_second / fix32(183 * _audioState._musicChannel.speed);
        fix32 const offset_per_sample = offset_per_second / samples_per_second;
        _audioState._musicChannel.offset += offset_per_sample;
        _audioState._musicChannel.volume += _audioState._musicChannel.volume_step / samples_per_second;
        _audioState._musicChannel.volume = fix32::min(fix32::max(_audioState._musicChannel.volume, fix32(0)), fix32(1));

        if (_audioState._musicChannel.volume_step < fix32(0) && _audioState._musicChannel.volume <= fix32(0))
        {
            // Fade out is finished, stop playing the current song
            for (int i = 0; i < _numChannels; ++i) {
                if (_audioState._sfxChannels[i].is_music) {
                    _audioState._sfxChannels[i].sfxId = -1;
                }
            }
            _audioState._musicChannel.pattern = -1;
        }
        else if (_audioState._musicChannel.offset >= _audioState._musicChannel.length)
        {
            int16_t next_pattern = _audioState._musicChannel.pattern + 1;
            int16_t next_count = _audioState._musicChannel.count + 1;
            //todo: pull out these flags, get memory storage correct as well
            if (_memory->songs[_audioState._musicChannel.pattern].getStop()) //stop part of the loop flag
            {
                next_pattern = -1;
                next_count = _audioState._musicChannel.count;
            }
            else if (_memory->songs[_audioState._musicChannel.pattern].getLoop()){
                while (--next_pattern > 0 && !_memory->songs[next_pattern].getStart())
                    ;
            }

            _audioState._musicChannel.count = next_count;
            set_music_pattern(next_pattern);
        }
    }

    if (index < 0 || index > 63) {
        //no (valid) sfx here. return silence
        return 0;
    }

    struct sfx const &sfx = _memory->sfx[index];

    // Speed must be 1—255 otherwise the SFX is invalid
    int const speed = std::max(1, (int)sfx.speed);

    fix32 const &offset = _audioState._sfxChannels[channel].offset;
    fix32 const &phi = _audioState._sfxChannels[channel].phi;

    // PICO-8 exports instruments as 22050 Hz WAV files with 183 samples
    // per speed unit per note, so this is how much we should advance
    fix32 const offset_per_second = samples_per_second / fix32(183 * speed);
    fix32 const offset_per_sample = offset_per_second / samples_per_second;
    fix32 next_offset = offset + offset_per_sample;

    // Handle SFX loops. From the documentation: “Looping is turned
    // off when the start index >= end index”.
    fix32 const loop_range = fix32(sfx.loopRangeEnd - sfx.loopRangeStart);
    if (loop_range > fix32(0) && next_offset >= fix32(sfx.loopRangeStart) && _audioState._sfxChannels[channel].can_loop) {
        next_offset = fix32::modf(next_offset - fix32(sfx.loopRangeStart) / loop_range) + fix32(sfx.loopRangeStart);
    }

    int const note_idx = offset;
    int const next_note_idx = next_offset;

    uint_fast8_t key = sfx.notes[note_idx].getKey();
    fix32 volume = sfx.notes[note_idx].getVolume() / fix32(7);
    fix32 freq = key_to_freq(key);

    if (volume == fix32(0)) {
        //volume all the way off. return silence, but make sure to set stuff
        _audioState._sfxChannels[channel].offset = next_offset;

        if (next_offset >= fix32(32)){
            _audioState._sfxChannels[channel].sfxId = -1;
        }
        else if (next_note_idx != note_idx){
            _audioState._sfxChannels[channel].prev_key = sfx.notes[note_idx].getKey();
            _audioState._sfxChannels[channel].prev_vol = fix32(sfx.notes[note_idx].getVolume()) / fix32(7);
        }

        return 0;
    }

    //TODO: apply effects
    int const fx = sfx.notes[note_idx].getEffect();

    // Apply effect, if any
    switch (fx)
    {
        case FX_NO_EFFECT:
            break;
        case FX_SLIDE:
        {
            fix32 t = fix32::modf(offset);
            // From the documentation: “Slide to the next note and volume”,
            // but it’s actually _from_ the _prev_ note and volume.
            freq = lerp(key_to_freq(_audioState._sfxChannels[channel].prev_key), freq, t);
            if (_audioState._sfxChannels[channel].prev_vol > fix32(0))
                volume = lerp(_audioState._sfxChannels[channel].prev_vol, fix32(volume), t);
            break;
        }
        case FX_VIBRATO:
        {
            // 7.5f and 0.25f were found empirically by matching
            // frequency graphs of PICO-8 instruments.
            fix32 t = fix32::abs(fix32::modf(fix32(7.5) * offset / offset_per_second) - fix32(0.5)) - fix32(0.25);
            // Vibrato half a semi-tone, so multiply by pow(2,1/12)
            freq = lerp(freq, freq * fix32(1.059463094359), t);
            break;
        }
        case FX_DROP:
            freq *= fix32(1) - fix32::modf(offset);
            break;
        case FX_FADE_IN:
            volume *= fix32::modf(offset);
            break;
        case FX_FADE_OUT:
            volume *= fix32(1) - fix32::modf(offset);
            break;
        case FX_ARP_FAST:
        case FX_ARP_SLOW:
        {
            // From the documentation:
            // “6 arpeggio fast  //  Iterate over groups of 4 notes at speed of 4
            //  7 arpeggio slow  //  Iterate over groups of 4 notes at speed of 8”
            // “If the SFX speed is <= 8, arpeggio speeds are halved to 2, 4”
            int const m = (speed <= 8 ? 32 : 16) / (fx == FX_ARP_FAST ? 4 : 8);
            int const n = (int)(m * 7.5f * offset / offset_per_second);
            int const arp_note = (note_idx & ~3) | (n & 3);
            freq = key_to_freq(sfx.notes[arp_note].getKey());
            break;
        }
    }


    // Play note
    fix32 waveform = z8::synth::waveform(sfx.notes[note_idx].getWaveform(), phi);

    // Apply master music volume from fade in/out
    // FIXME: check whether this should be done after distortion
    if (_audioState._sfxChannels[channel].is_music) {
        volume *= _audioState._musicChannel.volume;
    }

    sample = (fix32(32767.99) * volume * waveform);

    // TODO: Apply hardware effects
    if (_memory->hwState.distort & (1 << channel)) {
        sample = sample / 0x1000 * 0x1249;
    }

    _audioState._sfxChannels[channel].phi = phi + freq / samples_per_second;

    _audioState._sfxChannels[channel].offset = next_offset;

    if (next_offset >= fix32(32)){
        _audioState._sfxChannels[channel].sfxId = -1;
    }
    else if (next_note_idx != note_idx){
        _audioState._sfxChannels[channel].prev_key = sfx.notes[note_idx].getKey();
        _audioState._sfxChannels[channel].prev_vol = fix32(sfx.notes[note_idx].getVolume()) / fix32(7);
    }

    return sample;
}