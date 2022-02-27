#ifndef VHVC_INPUT
#define VHVC_INPUT
#include "common.hh"
namespace vhvc {
	struct Joy {
		uint8_t state = 0;
		int joyid = -1;
	};
	extern struct Joy joy1;
	extern struct Joy joy2;
	void handle_input(SDL_Event* ev);
	void input_debug(bool* p_open);
	uint8_t read_4016();
	uint8_t read_4017();
	void write_4016(uint8_t data);
}
#endif
