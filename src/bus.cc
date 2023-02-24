#include "bus.hh"
#include "cpudebug.hh"
#include "ppudebug.hh"
#include "input.hh"
#include "ppu.hh"
#include "mapper.hh"
#include "apu.hh"
namespace vhvc {
bool bus_inspect = false;
void irq_raise(unsigned source) {
	if (ppudebug::show_events) {
		switch (source) {
			case IRQ_MAPPER:
				ppudebug::put_event(0xFF0000); break;
			case IRQ_DMC:
				ppudebug::put_event(0xCC8888); break;
			case IRQ_FRAMECOUNTER:
				ppudebug::put_event(0x882222); break;
		}
	}
	cpu::irq |= source;
}
void irq_ack(unsigned source) {
	cpu::irq &= ~source;
}
bool irq_status(unsigned source) {
	return cpu::irq & source;
}
void bus_poweron() {
	cpu::poweron();
	ppu::poweron();
	irq_ack(-1);
	mapper->poweron();
	memset(cpu_ram, 0xFF, 2048);
	memset(ppu_ram, 0xFF, 2048);
}
void bus_reset() {
	cpu::reset();
	ppu::reset();
	mapper->reset();
}
uint8_t cpu_ram[2048] = {};
uint32_t cpu_cycle = 0; // TODO: probably should be a part of APU
bool in_dma = false;
enum {
	OAM_STATE_NONE = 0,
	OAM_STATE_WAIT = 1,
	OAM_STATE_READ = 2,
	OAM_STATE_WRITE = 3,
	DMC_STATE_NONE = 0,
	DMC_STATE_WAIT = 1,
	DMC_STATE_DUMMY = 2,
	DMC_STATE_READ = 3,
};
int dmc_dma = DMC_STATE_NONE;
int oam_dma = OAM_STATE_NONE;
uint8_t oam_dma_page = 0;
void trigger_dmc_dma() {
	if (dmc_dma == DMC_STATE_NONE)
		dmc_dma = DMC_STATE_WAIT;
}
uint8_t cpu_read_basic(uint16_t addr) {
	// FIXME: perhaps it's better to add a separate cpu_tick mapper callback?
	uint8_t mread = mapper->cpu_read(addr);
	if ((addr & 0xE000) == 0x0000) {
		return cpu_ram[addr & 0x7FF];
	} else if ((addr & 0xE000) == 0x2000) {
		return ppu::reg_read(addr & 7);
	} else if (addr == 0x4015) {
		return apu::read_4015();
	} else if (addr == 0x4016) {
		return bus_inspect ? cpu::data_bus : read_4016() & 0x1F | cpu::data_bus & 0xE0;
	} else if (addr == 0x4017) {
		return bus_inspect ? cpu::data_bus : read_4017() & 0x1F | cpu::data_bus & 0xE0;
	}
	return mread;
}
#define IS_GET() (!(cpu_cycle & 1))
#define IS_PUT() (cpu_cycle & 1)
uint8_t cpu_read(uint16_t addr) {
	uint8_t rv = cpu_read_basic(addr);
	if (!bus_inspect) {
		if (cpudebug::is_debugging)
			cpudebug::on_cycle();
		ppu::do_cycle();
		ppu::do_cycle();
		ppu::do_cycle();
		apu::do_cycle();
		cpu_cycle++;
		if (!in_dma && (oam_dma || dmc_dma)) {
			in_dma = true;
			cpu::rdy_happened = true;
			uint8_t oam_lo = 0;
			uint8_t oam_buf = 0;
			if (dmc_dma == DMC_STATE_WAIT)
				dmc_dma = DMC_STATE_DUMMY;
			if (oam_dma == OAM_STATE_WAIT)
				oam_dma = OAM_STATE_READ;
			while (oam_dma || dmc_dma) {
				bool cycle_used_up = false;
				switch (dmc_dma) {
				case DMC_STATE_WAIT:
					dmc_dma = DMC_STATE_DUMMY;
					break;
				case DMC_STATE_DUMMY:
					dmc_dma = DMC_STATE_READ;
					break;
				case DMC_STATE_READ:
					if (IS_GET()) {
						apu::dma_finish(cpu_read(apu::dma_addr()));
						cycle_used_up = true;
						dmc_dma = DMC_STATE_NONE;
					}
					break;
				}
				switch (oam_dma) {
				case OAM_STATE_WAIT:
					// OAM can't happen during DMC, so this is dead code.
					oam_lo = 0;
					oam_dma = OAM_STATE_READ;
					break;
				case OAM_STATE_READ:
					if (!cycle_used_up && IS_GET()) {
						oam_buf = cpu_read(oam_dma_page << 8 | oam_lo);
						cycle_used_up = true;
						oam_dma = OAM_STATE_WRITE;
					}
					break;
				case OAM_STATE_WRITE:
					if (!cycle_used_up && IS_PUT()) {
						cpu_write(0x2004, oam_buf);
						cycle_used_up = true;
						if (++oam_lo == 0) {
							oam_dma = OAM_STATE_NONE;
						} else {
							oam_dma = OAM_STATE_READ;
						}
					}
					break;
				}
				if (!cycle_used_up)
					cpu_read(addr);
			}
			rv = cpu_read(addr);
			in_dma = false;
		}
	}
	return rv;
}
void cpu_write(uint16_t addr, uint8_t data) {
	if ((addr & 0xE000) == 0x0000) {
		cpu_ram[addr & 0x7FF] = data;
	} else if ((addr & 0xE000) == 0x2000) {
		ppu::reg_write(addr & 7, data);
	} else if ((addr & 0xFFE0) == 0x4000) {
		apu::reg_write(addr, data);
		if (addr == 0x4014) {
			oam_dma = OAM_STATE_WAIT;
			oam_dma_page = data;
		}
		if (addr == 0x4016) {
			write_4016(data);
		}
	}
	mapper->cpu_write(addr, data);
	if (!bus_inspect) {
		if (cpudebug::is_debugging)
			cpudebug::on_cycle();
		ppu::do_cycle();
		ppu::do_cycle();
		ppu::do_cycle();
		apu::do_cycle();
		cpu_cycle++;
	}
}


uint8_t ppu_ram[2048] = {};
uint8_t* const ppu_ram0 = &ppu_ram[0];
uint8_t* const ppu_ram1 = &ppu_ram[1024];
uint8_t ppu_read(uint16_t addr) {
	assert(addr < 0x4000);
	return mapper->ppu_read(addr);
}
void ppu_write(uint16_t addr, uint8_t data) {
	assert(addr < 0x4000);
	mapper->ppu_write(addr, data);
}

}
