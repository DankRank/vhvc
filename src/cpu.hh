#ifndef VHVC_CPU
#define VHVC_CPU
#include "common.hh"
namespace vhvc::cpu {
	extern uint8_t a;
	extern uint8_t x;
	extern uint8_t y;
	extern uint8_t s;
	extern uint16_t pc;
	extern uint8_t data_bus;
	extern uint8_t ane_magic;
	extern uint8_t lax_magic;
	extern bool rdy_happened;
	extern bool nmi_line;
	extern bool nmi;
	extern bool irq;
	extern bool jammed;
	extern bool C;
	extern bool Z;
	extern bool I;
	extern bool D;
	extern bool V;
	extern bool N;
	extern bool exit_requested;
	static constexpr uint8_t B_FLAG = 1 << 4;
	uint8_t get_flags();
	void set_flags(uint8_t f);
	void set_nmi(bool state);

	void poweron();
	void reset();
	void step(int steps);
}
#endif
