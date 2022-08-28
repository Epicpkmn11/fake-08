#include <maxmod7.h>
#include <nds/ndstypes.h>

#include <math.h>

#include "Audio.h"
#include "fix32.h"
using z8::fix32;

#include <nds.h>

#define SAMPLERATE (22050)
#define SAMPLESPERBUF (800)

Audio *audio;

static mm_word onStreamRequest(mm_word length, mm_addr dest, mm_stream_formats format) {
	audio->FillMonoAudioBuffer(dest, 0, length);
	return length;
}

void audioSetup(void *audioAddr, void *waveBuffer, void *workBuffer) {
	audio = (Audio *)audioAddr;

	mm_stream stream;
	stream.sampling_rate	= SAMPLERATE;
	stream.buffer_length	= SAMPLESPERBUF;
	stream.callback			= onStreamRequest;
	stream.format			= MM_STREAM_16BIT_MONO;
	stream.timer			= MM_TIMER3;
	stream.manual			= true;
	mmStreamOpen(&stream, waveBuffer, workBuffer);
}
