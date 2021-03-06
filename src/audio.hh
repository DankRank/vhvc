#ifndef VHVC_AUDIO
#define VHVC_AUDIO
#include "common.hh"
namespace vhvc::audio {
	extern bool sync_to_audio;
	bool need_frame();
	void flip();
	void enqueue(int16_t sample);
	bool init();
}
#endif
