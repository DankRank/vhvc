#include "mapper.hh"
#include "bus.hh"
#include "imgui.h"
namespace vhvc {
struct FdsMapper : Mapper {
	FdsFile* nf = nullptr;
	uint8_t ram[32768] = {0};
	uint8_t chrram[8192] = {0};
	uint8_t *chr[16] = {0};
	uint16_t irq_latch = 0;
	uint16_t irq_counter = 0;
	bool irq_enabled = false;
	bool irq_repeat = false;
	uint8_t read_latch = 0;
	uint8_t write_latch = 0;
	uint8_t ext_latch = 0x80;
	uint8_t waveram[64] = {0};
	bool disk_regs = false;
	bool sound_regs = false;
	int side = -1;
	int side_offset = 0;
	bool disk_write = false;
	bool disk_scan_media = false;
	bool disk_stop_motor = false;
	bool disk_end_of_side = false;
	bool disk_scan_inprogress = false;
	bool disk_ready = false;
	bool disk_irq_enabled = false;
	bool disk_ingap = false;
	int scan_delay = 0;
	void set_nt(bool horizontal) {
		chr[13+(int)horizontal] = chr[9+(int)horizontal] = ppu_ram1;
		chr[14-(int)horizontal] = chr[10-(int)horizontal] = ppu_ram0;
	}
	void poweron() {
		for (int i = 0; i < 8; i++)
			chr[i] = &chrram[i*0x0400];
		chr[12] = chr[8] = ppu_ram0;
		chr[15] = chr[11] = ppu_ram1;
		set_nt(0);
		side = -1;
	}
	void irq_tick() {
		if (irq_enabled) {
			if (irq_counter)
				irq_counter--;
			if (irq_counter == 0) {
				irq_raise(IRQ_MAPPER);
				irq_counter = irq_latch;
				if (!irq_repeat)
					irq_enabled = false;
			}
		}
	}
	void disk_tick() {
		// shoutouts to mesen for the logic
		// someone should really document this on the wiki
		if (side == -1 || disk_stop_motor) {
			disk_end_of_side = true;
			disk_scan_inprogress = false;
			return;
		}
		if (!disk_scan_media && !disk_scan_inprogress)
			return;
		if (disk_end_of_side) {
			disk_end_of_side = false;
			side_offset = 0;
			scan_delay = 50000;
		}
		if (scan_delay) {
			scan_delay--;
		} else {
			disk_scan_inprogress = true;
			if (!disk_write) {
				if (!disk_ready)
					disk_ingap = true;
				read_latch = nf->serial_sides[side][side_offset];
				if (!disk_ingap) {
					if (disk_irq_enabled) {
						irq_raise(IRQ_DISK);
					}
				}
				if (disk_ready && disk_ingap && read_latch)
					disk_ingap = false;
			} else {
				// TODO: writing
			}

			if (side_offset++ >= (int)nf->serial_sides[side].size()) {
				disk_stop_motor = true;
			}
			scan_delay = 150;
		}
	}
	uint8_t cpu_read(uint16_t addr) {
		if (!bus_inspect) {
			irq_tick();
			disk_tick();
		}
		if ((addr & 0xE000) == 0xE000)
			return nf->bios[addr&0x1FFF];
		if (addr >= 0x6000)
			return ram[addr - 0x6000];
		if (addr >= 0x4040 && addr < 0x4080) {
			// TODO: fds sound
			return cpu::data_bus&0xC0 | waveram[addr&0x3F];
		}
		switch (addr) {
		case 0x4030: {
			uint8_t r = (int)irq_status(IRQ_DISK)<<1 | (int)irq_status(IRQ_MAPPER);
			if (!bus_inspect) {
				irq_ack(IRQ_MAPPER);
				irq_ack(IRQ_DISK);
			}
			if (disk_end_of_side)
				r |= 0x40;
			//r |= 0x80; // TODO: ???
			// Not implemented: CRC errors (bit 4)
			return cpu::data_bus&0x2C | r;
		}
		case 0x4031:
			if (!bus_inspect) {
				irq_ack(IRQ_DISK);
			}
			return read_latch;
		case 0x4032: {
			// TODO:
			uint8_t r;
			if (side == -1) {
				r = 0x7;
			} else {
				r = 0x6;
				if (disk_scan_inprogress)
					r &= ~2;
			}
			return cpu::data_bus&0xF8 | r;
		}
		case 0x4033:
			// TODO: unset bit 7 (battery ok) when motor is off
			// Not implemented: actual battry low condition
			return ext_latch;
		}

		return cpu::data_bus;
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		if (!bus_inspect) {
			irq_tick();
			disk_tick();
		}
		if (addr >= 0x6000 && addr < 0xE000)
			ram[addr - 0x6000] = data;
		if (addr >= 0x4040 && addr < 0x4080) {
			// TODO: fds sound
			waveram[addr&0x3F] = data&0x3F;
		}
		switch (addr) {
		case 0x4020:
			irq_latch = irq_latch&0xFF00 | data;
			break;
		case 0x4021:
			irq_latch = irq_latch&0xFF | data<<8;
			break;
		case 0x4022:
			if (disk_regs) {
				irq_repeat = data & 1;
				irq_enabled = data & 2;
				if (irq_enabled)
					irq_counter = irq_latch;
				else
					irq_ack(IRQ_MAPPER);
			}
			break;
		case 0x4023:
			disk_regs = data & 1;
			if (!disk_regs) {
				irq_enabled = false;
				irq_ack(IRQ_MAPPER);
				irq_ack(IRQ_DISK);
			}
			sound_regs = data & 2;
			break;
		case 0x4024:
			if (disk_regs) { // TODO: check
				write_latch = data;
				irq_ack(IRQ_DISK);
			}
			break;
		case 0x4025:
			if (disk_regs) { // TODO: check
				// these are active low
				disk_stop_motor = !(data & 0x01);
				disk_scan_media = !(data & 0x02);
				disk_write = !(data & 0x04);
				set_nt(data & 0x08);
				// TODO: other bits
				disk_ready = data & 0x40;
				disk_irq_enabled = data & 0x80;
				irq_ack(IRQ_DISK);
			}
			break;
		case 0x4026:
			// Unused external connector.
			ext_latch = 0x80 | data;
			break;

		}
	}
	uint8_t ppu_read(uint16_t addr) {
		return chr[addr >> 10][addr & 0x3FF];
	}
	void ppu_write(uint16_t addr, uint8_t data) {
		chr[addr >> 10][addr & 0x3FF] = data;
	}
	void debug_gui() {
		if (ImGui::BeginListBox("Disk Side", ImVec2(0, 0))) {
			if (ImGui::Selectable("No Side", side == -1))
				side = -1;
			char buf[5+11+1];
			for (int i = 0; i < (int)nf->sides.size(); i++) {
				sprintf(buf, "Side %d", i);
				if (ImGui::Selectable(buf, side == i)) {
					side = i;
					side_offset = 0;
				}
			}
			ImGui::EndListBox();
		}
	}
	FdsMapper(FdsFile &nf) :nf(&nf) {}
};
void mapper_setup(FdsFile& nf) {
	mapper_cleanup();
	mapper = new FdsMapper(nf);
}
}
