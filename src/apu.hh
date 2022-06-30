#ifndef VHVC_APU
#define VHVC_APU
#include "common.hh"
#include "io.hh"
namespace vhvc::apu {
	void do_cycle();
	uint8_t read_4015();
	void reg_write(uint16_t addr, uint8_t data);
	// TODO: move to apudebug
	extern File dump_file;
}
#endif
