#ifndef VHVC_BUS
#define VHVC_BUS
#include "common.hh"
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

	enum {
		IRQ_FRAMECOUNTER = 1<<0,
		IRQ_DMC = 1<<1,
		IRQ_MAPPER = 1<<2,
		IRQ_DISK = 1<<3, // this exists just so I don't have to mux two interrupts in FdsMapper
	};
	void irq_raise(unsigned source);
	void irq_ack(unsigned source);
	bool irq_status(unsigned source);
	void bus_poweron();
	void bus_reset();

	extern uint8_t cpu_ram[2048];
	void trigger_dmc_dma();
	uint8_t cpu_read(uint16_t addr);
	void cpu_write(uint16_t addr, uint8_t data);
	extern uint8_t ppu_ram[2048];
	extern uint8_t* const ppu_ram0;
	extern uint8_t* const ppu_ram1;
	uint8_t ppu_read(uint16_t addr);
	void ppu_write(uint16_t addr, uint8_t data);

}
#endif
