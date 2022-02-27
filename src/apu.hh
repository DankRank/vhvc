#ifndef VHVC_APU
#define VHVC_APU
namespace vhvc::apu {
	void do_cycle();
	uint8_t read_4015();
	void reg_write(uint16_t addr, uint8_t data);
}
#endif
