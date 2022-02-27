#ifndef VHVC_CPUDEBUG
#define VHVC_CPUDEBUG
#include "common.hh"
namespace vhvc::cpudebug {
	extern bool is_debugging;
	extern bool nestest;
	void on_insn();
	void on_cycle();
	extern bool show_cpu_state;
	extern bool show_cpu_memory;
	void gui();
}
#endif
