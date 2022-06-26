#include "mapper.hh"
#include "bus.hh"
#include "ppu.hh"
#include "ppudebug.hh"
#include "imgui.h"
namespace vhvc {
struct MMC1 : BasicMapper {
	int reg = 0;
	int bitn = 0;
	int reg_ctl = 0;
	int reg_chr0 = 0;
	int reg_chr1 = 0;
	int reg_prg = 0;
	void poweron() {
		has_prgram = true;
		reg_ctl = 0xC;
		update_mapping();
	}
	bool consecutive_write = false;
	void update_mapping() {
		int mirroring_modes[4] = { MIRRORING_SCREENA, MIRRORING_SCREENB, MIRRORING_VERTICAL, MIRRORING_HORIZONTAL };
		set_mirroring(mirroring_modes[reg_ctl&3]);
		if (reg_ctl & 0x8) {
			if (reg_ctl & 0x4) {
				set_prg16k(0, nf->get_prg16k(reg_prg & 15));
				set_prg16k(1, nf->get_prg16k(-1));
			} else {
				set_prg16k(0, nf->get_prg16k(0));
				set_prg16k(1, nf->get_prg16k(reg_prg & 15));
			}
		} else {
			set_prg32k(nf->get_prg32k(reg_prg/2 & 7));
		}
		chrram_check();
		if (reg_ctl & 0x10) {
			set_chr4k(0, nf->get_chr4k(reg_chr0));
			set_chr4k(1, nf->get_chr4k(reg_chr1));
		} else {
			set_chr8k(nf->get_chr8k(reg_chr0/2));
		}
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if (consecutive_write)
			return;
		consecutive_write = true;
		if (addr & 0x8000) {
			if (data & 0x80) {
				reg_ctl |= 0x0C;
				reg = 0;
				bitn = 0;
				update_mapping();
				return;
			}
			if (data & 1)
				reg |= 1 << bitn;
			bitn++;
			if (bitn == 5) {
				switch (addr & 0xE000) {
				case 0x8000: reg_ctl = reg;  break; // 8000-9FFF
				case 0xA000: reg_chr0 = reg; break; // A000-BFFF
				case 0xC000: reg_chr1 = reg; break; // C000-DFFF
				case 0xE000: reg_prg = reg;  break; // E000-FFFF
				}
				reg = 0;
				bitn = 0;
				update_mapping();
			}
		}
	}
	uint8_t cpu_read(uint16_t addr) {
		consecutive_write = false;
		return BasicMapper::cpu_read(addr);
	}
	void debug_gui() {
		BasicMapper::debug_gui();
		ImGui::Text("reg_ctl  %02X", reg_ctl);
		ImGui::Text("reg_chr0 %02X", reg_chr0);
		ImGui::Text("reg_chr1 %02X", reg_chr1);
		ImGui::Text("reg_prg  %02X", reg_prg);
	}
	MMC1(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(MMC1)
struct MMC2 : BasicMapper {
	bool isMMC4;
	bool latch0 = false;
	bool latch1 = false;
	int bank0FD = 0;
	int bank0FE = 0;
	int bank1FD = 0;
	int bank1FE = 0;

	void poweron() {
		set_mirroring(MIRRORING_VERTICAL);
		if (isMMC4) {
			set_prg16k(0, nf->get_prg16k(0));
			set_prg16k(1, nf->get_prg16k(-1));
		} else {
			set_prg8k(0, nf->get_prg8k(0));
			set_prg8k(1, nf->get_prg8k(-3));
			set_prg8k(2, nf->get_prg8k(-2));
			set_prg8k(3, nf->get_prg8k(-1));
		}
		set_chr4k(0, nf->get_chr4k(0));
		set_chr4k(1, nf->get_chr4k(0));
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		switch (addr & 0xF000) {
		case 0xA000:
			if (isMMC4)
				set_prg16k(0, nf->get_prg16k(data&7));
			else
				set_prg8k(0, nf->get_prg8k(data&15));
			break;
		case 0xB000:
			bank0FD = data & 0x1F;
			if (!latch0) // FIXME: do those switches happen automatically?
				set_chr4k(0, nf->get_chr4k(bank0FD));
			break;
		case 0xC000:
			bank0FE = data & 0x1F;
			if (latch0)
				set_chr4k(0, nf->get_chr4k(bank0FE));
			break;
		case 0xD000:
			bank1FD = data & 0x1F;
			if (!latch1)
				set_chr4k(1, nf->get_chr4k(bank1FD));
			break;
		case 0xE000:
			bank1FE = data & 0x1F;
			if (latch1)
				set_chr4k(1, nf->get_chr4k(bank1FE));
			break;
		case 0xF000:
			set_mirroring(data&1 ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
			break;
		}
	}
	uint8_t ppu_read(uint16_t addr) {
		uint8_t rv = BasicMapper::ppu_read(addr);
		if (bus_inspect)
			return rv;
		if (isMMC4)
			addr &= 0x3FF8;
		if (addr == 0x0FD8) {
			latch0 = false;
			set_chr4k(0, nf->get_chr4k(bank0FD));
		} else if (addr == 0x0FE8) {
			latch0 = true;
			set_chr4k(0, nf->get_chr4k(bank0FE));
		}
		addr &= 0x3FF8;
		if (addr == 0x1FD8) {
			latch1 = false;
			set_chr4k(1, nf->get_chr4k(bank1FD));
		} else if (addr == 0x1FE8) {
			latch1 = true;
			set_chr4k(1, nf->get_chr4k(bank1FE));
		}
		return rv;
	}
	MMC2(NesFile& nf, int isMMC4) :BasicMapper(nf), isMMC4(isMMC4) {}
};
DECLARE_MAPPER_INT(MMC2)
struct MMC3 : BasicMapper {
	int cur_reg = 0;
	bool prg_swap = false;
	bool chr_swap = false;
	int irq_latch = 0;
	int irq_counter = 0;
	bool irq_reload = false;
	bool irq_enabled = false;
	bool ppu_a12 = false;
	bool ppu_a12_d1 = false;
	bool ppu_a12_d2 = false;
	bool ppu_a12_d3 = false;
	uint8_t dbg_reg[8] = {0};
	void poweron() {
		set_prg8k(0, nf->get_prg16k(0));
		set_prg8k(1, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(-1));
		set_chr2k(0, nf->get_chr8k(0));
		set_chr2k(1, nf->get_chr8k(0));
		set_chr1k(4, nf->get_chr8k(0));
		set_chr1k(5, nf->get_chr8k(0));
		set_chr1k(6, nf->get_chr8k(0));
		set_chr1k(7, nf->get_chr8k(0));
		chrram_check();
		set_mirroring(MIRRORING_VERTICAL);
	}
	void irq_tick() {
		if (!ppu_a12_d3 && !ppu_a12_d2 && !ppu_a12_d1 && ppu_a12) {
			if (ppudebug::show_events)
				ppudebug::put_event(0x00FFFF);
			if (irq_reload || irq_counter == 0) {
				if (!irq_reload && irq_enabled)
					irq_raise(IRQ_MAPPER);
				irq_counter = irq_latch;
				irq_reload = false;
			} else {
				irq_counter--;
			}
		}
		ppu_a12_d3 = ppu_a12_d2; ppu_a12_d2 = ppu_a12_d1; ppu_a12_d1 = ppu_a12; ppu_a12 = false;
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if (!bus_inspect)
			irq_tick();
		switch (addr & 0xE001) {
		case 0x8000:
			cur_reg = data & 7;
			prg_swap = data & 0x40;
			chr_swap = data & 0x80;
			break;
		case 0x8001:
			dbg_reg[cur_reg] = data;
			switch (cur_reg) {
			case 0: set_chr2k(0, nf->get_chr1k(data&0xFE)); break;
			case 1: set_chr2k(1, nf->get_chr1k(data&0xFE)); break;
			case 2: set_chr1k(4, nf->get_chr1k(data)); break;
			case 3: set_chr1k(5, nf->get_chr1k(data)); break;
			case 4: set_chr1k(6, nf->get_chr1k(data)); break;
			case 5: set_chr1k(7, nf->get_chr1k(data)); break;
			case 6: set_prg8k(0, nf->get_prg8k(data&0x3F)); break;
			case 7: set_prg8k(1, nf->get_prg8k(data&0x3F)); break;
			}
			break;
		case 0xA000:
			set_mirroring(data & 1 ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
			break;
		case 0xA001:
			// FIXME:
			break;
		case 0xC000: irq_latch = data; break;
		case 0xC001: irq_reload = true; break;
		case 0xE000: irq_enabled = false; irq_ack(IRQ_MAPPER); break;
		case 0xE001: irq_enabled = true; break;
		}
	}
	uint8_t cpu_read(uint16_t addr) {
		if (!bus_inspect)
			irq_tick();
		if (prg_swap && (addr & 0xA000) == 0x8000)
			addr ^= 0x4000;
		return BasicMapper::cpu_read(addr);
	}
	void ppu_write(uint16_t addr, uint8_t data) {
		if (!bus_inspect && addr & 0x1000) // FIXME: timing of this might be slightly off
			ppu_a12 = true;
		if (chr_swap && !(addr & 0x2000))
			addr ^= 0x1000;
		BasicMapper::ppu_write(addr, data);
	}
	uint8_t ppu_read(uint16_t addr) {
		if (!bus_inspect && addr & 0x1000)
			ppu_a12 = true;
		if (chr_swap && !(addr & 0x2000))
			addr ^= 0x1000;
		return BasicMapper::ppu_read(addr);
	}
	void debug_gui() {
		BasicMapper::debug_gui();
		ImGui::Text("cur_reg %d", cur_reg);
		ImGui::Text("prg_swap %c, chr_swap %c",
			prg_swap ? '+' : '-',
			chr_swap ? '+' : '-');
		ImGui::Text("R0 %02X, R1 %02X, R2 %02X, R3 %02X", dbg_reg[0], dbg_reg[1], dbg_reg[2], dbg_reg[3]);
		ImGui::Text("R4 %02X, R5 %02X, R6 %02X, R7 %02X", dbg_reg[4], dbg_reg[5], dbg_reg[6], dbg_reg[7]);
		ImGui::Text("ppu_a12 %c%c%c%c",
			ppu_a12_d3 ? '+' : '-',
			ppu_a12_d2 ? '+' : '-',
			ppu_a12_d1 ? '+' : '-',
			ppu_a12 ? '+' : '-');
		ImGui::Text("irq_latch %d", irq_latch);
		ImGui::Text("irq_counter %d", irq_counter);
		ImGui::Text("irq_enabled %c | irq_reload %c", irq_enabled ? '+' : '-', irq_reload ? '+' : '-');
	}
	MMC3(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(MMC3)
struct MMC5 : Mapper {
	NesFile* nf = nullptr;
	uint8_t prgram[131072] = { 0 };
	uint8_t exram[1024] = { 0 };
	uint8_t fill_tile = 0;
	uint8_t fill_attr = 0;
	int prg_mode = 0;
	int chr_mode = 0;
	int exram_mode = 0;
	int nt_mapping = 0;
	uint8_t* prg[5]; // 8k granularity
	bool prg_wr[5];
	uint8_t prg_reg[5] = { 0, 0x80, 0x80, 0x80, 0xFF };
	uint8_t ram_prot1 = 0;
	uint8_t ram_prot2 = 0;
	uint8_t* chr[12];
	uint16_t chr_reg[12];
	uint8_t chr_upper = 0;
	uint8_t mul1 = 0xFF, mul2 = 0xFF;
	uint8_t scanline_compare = 0;
	bool irq_enabled = false;
	bool irq_pending = false;
	bool obj_size = false;
	int prev_line = 0;
	uint16_t last_nt = 0;
	uint8_t* prg_for_reg(uint8_t reg) {
		if (reg & 0x80)
			return nf->get_prg8k(reg&0x7F);
		else
			return &prgram[(reg&0x0F)<<13];
	}
	bool wr_for_reg(int n) {
		return !(prg_reg[n]&0x80);
	}
	void refresh_prg() {
		switch (prg_mode) {
		case 0:
			prg_wr[0] = wr_for_reg(0); prg[0] = prg_for_reg(prg_reg[0]);
			prg_wr[1] = wr_for_reg(4); prg[1] = prg_for_reg(prg_reg[4]&0xFC);
			prg_wr[2] = wr_for_reg(4); prg[2] = prg_for_reg(prg_reg[4]&0xFC | 1);
			prg_wr[3] = wr_for_reg(4); prg[3] = prg_for_reg(prg_reg[4]&0xFC | 2);
			prg_wr[4] = wr_for_reg(4); prg[4] = prg_for_reg(prg_reg[4]&0xFC | 3);
			break;
		case 1:
			prg_wr[0] = wr_for_reg(0); prg[0] = prg_for_reg(prg_reg[0]);
			prg_wr[1] = wr_for_reg(2); prg[1] = prg_for_reg(prg_reg[2]&0xFE);
			prg_wr[2] = wr_for_reg(2); prg[2] = prg_for_reg(prg_reg[2]&0xFE | 1);
			prg_wr[3] = wr_for_reg(4); prg[3] = prg_for_reg(prg_reg[4]&0xFE);
			prg_wr[4] = wr_for_reg(4); prg[4] = prg_for_reg(prg_reg[4]&0xFE | 1);
			break;
		case 2:
			prg_wr[0] = wr_for_reg(0); prg[0] = prg_for_reg(prg_reg[0]);
			prg_wr[1] = wr_for_reg(2); prg[1] = prg_for_reg(prg_reg[2]&0xFE);
			prg_wr[2] = wr_for_reg(2); prg[2] = prg_for_reg(prg_reg[2]&0xFE | 1);
			prg_wr[3] = wr_for_reg(3); prg[3] = prg_for_reg(prg_reg[3]);
			prg_wr[4] = wr_for_reg(4); prg[4] = prg_for_reg(prg_reg[4]);
			break;
		case 3:
			prg_wr[0] = wr_for_reg(0); prg[0] = prg_for_reg(prg_reg[0]);
			prg_wr[1] = wr_for_reg(1); prg[1] = prg_for_reg(prg_reg[1]);
			prg_wr[2] = wr_for_reg(2); prg[2] = prg_for_reg(prg_reg[2]);
			prg_wr[3] = wr_for_reg(3); prg[3] = prg_for_reg(prg_reg[3]);
			prg_wr[4] = wr_for_reg(4); prg[4] = prg_for_reg(prg_reg[4]);
			break;
		}
	}
	void refresh_chr() {
		switch (chr_mode) {
		case 0:
			for (int i = 0; i < 8; i++)
				chr[i] = nf->get_chr1k(chr_reg[7]*8 + i);
			for (int i = 0; i < 4; i++)
				chr[8+i] = nf->get_chr1k(chr_reg[11]*8 + i); // i'm not entirely sure how exactly this slot works
			break;
		case 1:
			for (int i = 0; i < 4; i++)
				chr[i] = nf->get_chr1k(chr_reg[3]*4 + i);
			for (int i = 0; i < 4; i++)
				chr[4+i] = nf->get_chr1k(chr_reg[7]*4 + i);
			for (int i = 0; i < 4; i++)
				chr[8+i] = nf->get_chr1k(chr_reg[11]*4 + i);
			break;
		case 2:
			for (int i = 0; i < 6; i++) {
				chr[i*2] = nf->get_chr2k(chr_reg[i*2 + 1]*2);
				chr[i*2 + 1] = nf->get_chr2k(chr_reg[i*2 + 1]*2 + 1);
			}
			break;
		case 3:
			for (int i = 0; i < 12; i++)
				chr[i] = nf->get_chr1k(chr_reg[i]);
			break;
		}

	}
	void poweron() {
		refresh_prg();
		refresh_chr();
		reset();
	}
	void reset() {
		irq_pending = false;
	}
	uint8_t cpu_read(uint16_t addr) {
		if (addr > 0x6000)
			return prg[(addr>>13) - 3][addr & 0x1FFF];
		if (addr >= 0x5C00 && addr <= 0x5FFF && exram_mode >= 2)
			return exram[addr&0x3FF];
		switch (addr) {
		case 0x5204: {
			bool in_frame = ppu::line >= 240 && (ppu::bg_enable || ppu::obj_enable);
			irq_ack(IRQ_MAPPER);
			if (irq_pending) {
				irq_pending = false;
				return in_frame ? 0xC0 : 0x80;
			} else {
				return in_frame ? 0x40 : 0x00;
			}
		}
		case 0x5205: return mul1*mul2;
		case 0x5206: return (mul1*mul2) >> 8;
		}
		return cpu::data_bus;
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		switch (addr) {
		case 0x2000: obj_size = data&32; break;
		// TODO: $5000-$5013 sound
		// TODO: more faithful scanline counter/irq emulation
		case 0x5100: prg_mode = data&3; refresh_prg(); return;
		case 0x5101: chr_mode = data&3; refresh_chr(); return;
		case 0x5102: ram_prot1 = data&3; return;
		case 0x5103: ram_prot2 = data&3; return;
		case 0x5104: exram_mode = data&3; return;
		case 0x5105: nt_mapping = data; return;
		case 0x5106: fill_tile = data; return;
		case 0x5107: fill_attr = data&3; return;
		case 0x5113: prg_reg[0] = data&0x7F; refresh_prg(); return;
		case 0x5114: prg_reg[1] = data; refresh_prg(); return;
		case 0x5115: prg_reg[2] = data; refresh_prg(); return;
		case 0x5116: prg_reg[3] = data; refresh_prg(); return;
		case 0x5117: prg_reg[4] = data|0x80; refresh_prg(); return;
		case 0x5120: case 0x5121: case 0x5122: case 0x5123:
		case 0x5124: case 0x5125: case 0x5126: case 0x5127:
		case 0x5128: case 0x5129: case 0x512A: case 0x512B:
			chr_reg[addr - 0x5120] = chr_upper<<8 | data; refresh_chr();
			return;
		case 0x5130: chr_upper = data&3; return;
		// TODO: $5200 $5201 $5202 split screen
		case 0x5203: scanline_compare = data; return;
		case 0x5204: irq_enabled = data&0x80; if (irq_enabled && irq_pending) irq_raise(IRQ_MAPPER); return;
		case 0x5205: mul1 = data; return;
		case 0x5206: mul2 = data; return;
		}
		if (addr >= 0x6000 && ram_prot1 == 2 && ram_prot2 == 1) {
			int slot = (addr>>13) - 3;
			if (prg_wr[slot])
				prg[slot][addr & 0x1FFF] = data;
		}
		if (addr >= 0x5C00 && addr <= 0x5FFF && exram_mode != 3) // FIXME: only allow writes during rendering
			exram[addr&0x3FF] = data;
	}
	uint8_t ppu_read(uint16_t addr) {
		if (!bus_inspect) {
			if (ppu::line != prev_line && scanline_compare && scanline_compare < 240 && ppu::line == scanline_compare) {
				irq_pending = true;
				if (irq_enabled) {
					//SDL_Log("line: %d, cyc: %d", ppu::line, ppu::dot);
					irq_raise(IRQ_MAPPER);
				}
			}
			prev_line = ppu::line;
		}
		if (exram_mode == 1 && (ppu::line < 240 || ppu::line == 261) && ppu::dot && (ppu::dot < 257 || ppu::dot > 320)) {
			switch (ppu::dot&7) {
			case 2:
				last_nt = addr&0x3FF;
				break;
			case 4:
				return (exram[last_nt]>>6) * 0x55;
			case 6:
			case 0:
				return nf->get_chr4k(chr_upper<<6 | exram[last_nt]&0x3F)[addr&0xFFF];
			}
		}
		if (addr >= 0x2000) {
			int n = nt_mapping>>(addr>>9 & 6) & 3;
			switch (n) {
			case 0: return ppu_ram0[addr&0x3FF];
			case 1: return ppu_ram1[addr&0x3FF];
			case 2: return exram_mode&2 ? 0 : exram[addr&0x3FF];
			case 3: return (addr&0x3FF) < 0x3C0 ? fill_tile : fill_attr*0x55;
			}
		} else {

			// TODO: i think this is also predicated on rendering? (2002 or in_frame though?)
			if (!obj_size || ppu::line < 240 && ppu::dot >= 257 && ppu::dot <= 320 && ((ppu::dot&7) == 0 || (ppu::dot&7) == 6))
				return chr[addr>>10 & 7][addr&0x3FF];
			else
				return chr[8 + (addr>>10 & 3)][addr&0x3FF];
		}
		return addr&0xFF;
	}
	void ppu_write(uint16_t addr, uint8_t data) {
		if (addr >= 0x2000) {
			int n = nt_mapping>>(addr>>9 & 6) & 3;
			switch (n) {
			case 0: ppu_ram0[addr&0x3FF] = data; break;
			case 1: ppu_ram1[addr&0x3FF] = data; break;
			case 2: exram[addr&0x3FF] = data; break; // TODO: when is this writable?
			}
		}
	}
	void debug_gui() {
		ImGui::Text("prg_mode %d", prg_mode);
		ImGui::Text("chr_mode %d", chr_mode);
		ImGui::Text("exram_mode %d", exram_mode);
		ImGui::Text("nt_mapping %x", nt_mapping);
		for (int i = 0; i < 5; i++)
			ImGui::Text("%x %x", 0x5113+i, prg_reg[i]);
		for (int i = 0; i < 12; i++)
			ImGui::Text("%x %x", 0x5120+i, chr_reg[i]);
		ImGui::Text("chr_upper %x", chr_upper);

	}
	MMC5(NesFile& nf) :nf(&nf) {}
};
DECLARE_MAPPER(MMC5)
}
