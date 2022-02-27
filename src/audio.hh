#ifndef VHVC_AUDIO
#define VHVC_AUDIO
namespace vhvc::audio {
	void flip();
	void enqueue(int16_t sample);
	bool init();
}
#endif
