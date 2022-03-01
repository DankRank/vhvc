#include "bus.hh"
#include "cpudebug.hh"
#include "input.hh"
#include "ppu.hh"
#include "mapper.hh"
#include "apu.hh"
namespace vhvc {
bool bus_inspect = false;
void irq_raise(unsigned source) {
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
bool oam_dma = false;
uint8_t oam_dma_page = 0;
uint8_t cpu_read_basic(uint16_t addr) {
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
	return mapper->cpu_read(addr);
}
uint8_t cpu_read(uint16_t addr) {
	uint8_t rv = cpu_read_basic(addr);
	if (!bus_inspect) {
		if (cpudebug::is_debugging)
			cpudebug::on_cycle();
		ppu::do_cycle();
		ppu::do_cycle();
		ppu::do_cycle();
		apu::do_cycle();
		if (oam_dma) {
			// TODO: this will need to be adjusted for DMC DMA
			oam_dma = false;
			cpu::rdy_happened = true;
			cpu_read(addr);
			for (int i = 0; i < 256; i++) {
				if (cpu_cycle & 1)
					cpu_read(addr); // TODO: check what address is used for those reads
				cpu_write(0x2004, cpu_read(oam_dma_page << 8 | i));
			}
		}
		cpu_cycle++;
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
			oam_dma = true;
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
