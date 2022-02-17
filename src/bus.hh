#ifndef VHVC_BUS
#define VHVC_BUS
#include <stdint.h>
#include "cpu.hh"
namespace vhvc {
	extern bool bus_inspect;
	class inspect_lock {
		bool save;
	public:
		inspect_lock() { save = bus_inspect; bus_inspect = true; }
		~inspect_lock() { bus_inspect = save; }
		inspect_lock(const inspect_lock&) = delete;
		inspect_lock& operator=(const inspect_lock&) = delete;
	};

	extern uint8_t cpu_ram[2048];
	uint8_t cpu_read(uint16_t addr);
	void cpu_write(uint16_t addr, uint8_t data);
	extern uint8_t ppu_ram[2048];
	extern uint8_t* const ppu_ram0;
	extern uint8_t* const ppu_ram1;
	uint8_t ppu_read(uint16_t addr);
	void ppu_write(uint16_t addr, uint8_t data);

}
#endif
