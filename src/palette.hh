#ifndef VHVC_PALETTE
#define VHVC_PALETTE
#include "common.hh"
namespace vhvc::palette {
	extern uint32_t colors[8*64];
	void set_colors(const uint8_t* pal);
	void set_default_colors();
}
#endif
