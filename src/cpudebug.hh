#ifndef VHVC_CPUDEBUG
#define VHVC_CPUDEBUG
#include "common.hh"
namespace vhvc::cpudebug {
	extern bool is_debugging;
	extern bool nestest;
	extern bool log_intr;
	void on_insn();
	void on_cycle();
	void on_intr();
	extern bool show_cpu_state;
	extern bool show_cpu_memory;
	void gui();
}
#endif
