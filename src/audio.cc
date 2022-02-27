#include "audio.hh"
#include <stdio.h>
namespace vhvc::audio {
// The easy way would be to just use SDL_QueueAudio, but it holds the audio mutex,
// and I want to write samples one at a time. I could write samples to a buffer
// and enqueue a bunch at once, but that'd mean having to manage a buffer. Might
// as well manage my own queue and avoid a copy while I'm at.
template<int Sz>
struct audio_queue {
	uint8_t data[Sz];
	int read_start = 0;
	int read_left = 0;
	int read_pending = 0;
	int write_start = 0;
	int write_left = Sz;
	int write_pending = 0;
	// NOTE: It's almost possible to avoid the _pending fields, but in the case
	// where both sides of the queue are exhausted, it's impossible to tell
	// which side should be expanded. And also the flip function becomes more complicated.
	int read(uint8_t* buf, int size) {
		int nread = size < read_left ? size : read_left;
		size = nread;
		int until_wrap = Sz-read_start;
		if (size >= until_wrap) {
			memcpy(buf, data+read_start, until_wrap);
			buf += until_wrap;
			size -= until_wrap;
			read_start = 0;
		}
		if (size) {
			memcpy(buf, data+read_start, size);
			read_start += size;
		}
		read_left -= nread;
		read_pending += nread;
		return nread;
	}
	int write(uint8_t* buf, int size) {
		int nwritten = size < write_left ? size : write_left;
		size = nwritten;
		int until_wrap = Sz-write_start;
		if (size >= until_wrap) {
			memcpy(data+write_start, buf, until_wrap);
			buf += until_wrap;
			size -= until_wrap;
			write_start = 0;
		}
		if (size) {
			memcpy(data+write_start, buf, size);
			write_start += size;
		}
		write_left -= nwritten;
		write_pending += nwritten;
		return nwritten;
	}
	void flip() {
		write_left += read_pending;
		read_left += write_pending;
		read_pending = write_pending = 0;
	}
};
audio_queue<4096*3*2> queue; // FIXME: figure out an optimal size (probably some multiple of 1600 (48000/60*2))
SDL_AudioDeviceID devid;
uint8_t silence = 0;
extern "C" void SDLCALL callback(void* userdata, Uint8* stream, int len) {
	int nread = queue.read(stream, len);
	if (nread < len)
		memset(stream+nread, silence, len-nread);
}
void flip() {
	SDL_LockAudioDevice(devid);
	queue.flip();
	SDL_UnlockAudioDevice(devid);
}
void enqueue(int16_t sample) {
	queue.write((uint8_t*)&sample, sizeof(sample));
}
bool init() {
	SDL_AudioSpec desired, obtained;
	memset(&desired, 0, sizeof(desired));
	desired.freq = 48000;
	desired.format = AUDIO_S16SYS;
	desired.channels = 1;
	desired.samples = 4096;
	desired.callback = callback;
	devid = SDL_OpenAudioDevice(NULL, false, &desired, &obtained, 0);
	if (!devid) {
		fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
		return false;
	}
	silence = obtained.silence;
	SDL_PauseAudioDevice(devid, 0);
	return true;
}
}
