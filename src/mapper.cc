#include "mapper.hh"
#include "bus.hh"
#include "imgui.h"
namespace vhvc {

void Mapper::poweron() {}
void Mapper::reset() {}
uint8_t Mapper::cpu_read(uint16_t addr) { return cpu::data_bus; }
void Mapper::cpu_write(uint16_t addr, uint8_t data) { }
uint8_t Mapper::ppu_read(uint16_t addr) { return addr & 0xFF; }
void Mapper::ppu_write(uint16_t addr, uint8_t data) {}
void Mapper::debug_gui() {}
Mapper noop_mapper;
Mapper* mapper = &noop_mapper;

bool in_range(cspan_u8 span, void* ptr) {
	return ptr >= span.data() && ptr < span.data() + span.size();
}


struct BasicMapper : Mapper {
	NesFile* nf = nullptr;
	uint8_t* prg[4] = {0}; // 8k granularity
	uint8_t* chr[16] = {0}; // 1k granularity
	uint8_t prgram[8192];
	uint8_t chrram[8192];
	bool has_prgram = true;
	bool has_chrram = false;
	enum {
		MIRRORING_HORIZONTAL,
		MIRRORING_VERTICAL,
		MIRRORING_SCREENA,
		MIRRORING_SCREENB,
	};
	void set_chr1k(int i, uint8_t* bank) {
		chr[i] = bank;
	}
	void set_chr2k(int i, uint8_t* bank) {
		chr[i*2] = bank;
		chr[i*2+1] = &bank[1024];
	}
	void set_chr4k(int slot, uint8_t *bank) {
		for (int i = 0; i < 4; i++)
			chr[slot*4+i] = &bank[1024*i];
	}
	void set_chr8k(uint8_t *bank) {
		for (int i = 0; i < 8; i++)
			chr[i] = &bank[1024*i];
	}
	void set_prg8k(int i, uint8_t* bank) {
		prg[i] = bank;
	}
	void set_prg16k(int i, uint8_t* bank) {
		prg[i*2+0] = &bank[0*8192];
		prg[i*2+1] = &bank[1*8192];
	}
	void set_prg32k(uint8_t* bank) {
		prg[0] = &bank[0*8192];
		prg[1] = &bank[1*8192];
		prg[2] = &bank[2*8192];
		prg[3] = &bank[3*8192];
	}
	void chrram_check() {
		has_chrram = !nf->chrrom.size();
	}
	void set_nametable(int i, uint8_t *nt) {
		chr[12+i] = chr[8+i] = nt;
	}
	void set_nametables(uint8_t* nt0, uint8_t* nt1, uint8_t* nt2, uint8_t* nt3) {
		chr[12] = chr[8] = nt0;
		chr[13] = chr[9] = nt1;
		chr[14] = chr[10] = nt2;
		chr[15] = chr[11] = nt3;
	}
	void set_mirroring(int type) {
		switch (type) {
			case MIRRORING_HORIZONTAL: set_nametables(ppu_ram0, ppu_ram0, ppu_ram1, ppu_ram1); break;
			case MIRRORING_VERTICAL:   set_nametables(ppu_ram0, ppu_ram1, ppu_ram0, ppu_ram1); break;
			case MIRRORING_SCREENA:    set_nametables(ppu_ram0, ppu_ram0, ppu_ram0, ppu_ram0); break;
			case MIRRORING_SCREENB:    set_nametables(ppu_ram1, ppu_ram1, ppu_ram1, ppu_ram1); break;
		}
	}

	uint8_t cpu_read(uint16_t addr) {
		if (addr & 0x8000)
			return prg[addr>>13 & 3][addr & 0x1FFF];
		if (has_prgram && (addr & 0xE000) == 0x6000)
			return prgram[addr & 0x1FFF];
		return cpu::data_bus;
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		if (has_prgram && (addr & 0xE000) == 0x6000)
			prgram[addr & 0x1FFF] = data;
	}
	uint8_t ppu_read(uint16_t addr) {
		return chr[addr >> 10][addr & 0x3FF];
	}
	void ppu_write(uint16_t addr, uint8_t data) {
		if (has_chrram || addr >= 0x2000) {
			chr[addr>>10][addr & 0x3FF] = data;
		}
	}
	const char *describe_pointer(uint8_t *ptr) {
		static char buf[512];
		if (in_range(nf->prgrom, ptr))
			sprintf(buf, "PRGROM:%llX", ptr - nf->prgrom.data());
		else if (in_range(nf->chrrom, ptr))
			sprintf(buf, "CHRROM:%llX", ptr - nf->chrrom.data());
		else if (in_range(span_u8(ppu_ram, sizeof(ppu_ram)), ptr))
			sprintf(buf, "CIRAM:%llX", ptr - ppu_ram);
		else if (in_range(nf->chrram, ptr))
			sprintf(buf, "CHRRAM:%llX", ptr - nf->chrram.data());
		else if (in_range(span_u8(chrram, sizeof(chrram)), ptr))
			sprintf(buf, "CHRRAM:%llX", ptr - chrram);
		else if (!ptr)
			sprintf(buf, "NULL");
		else
			sprintf(buf, "UNK:%p", ptr);
		return buf;
	}
	void debug_gui() {
		ImGui::Text("8000: %s", describe_pointer(prg[0]));
		ImGui::Text("A000: %s", describe_pointer(prg[1]));
		ImGui::Text("C000: %s", describe_pointer(prg[2]));
		ImGui::Text("E000: %s", describe_pointer(prg[3]));
		ImGui::Text("0000: %s", describe_pointer(chr[0]));
		ImGui::Text("0400: %s", describe_pointer(chr[1]));
		ImGui::Text("0800: %s", describe_pointer(chr[2]));
		ImGui::Text("0C00: %s", describe_pointer(chr[3]));
		ImGui::Text("1000: %s", describe_pointer(chr[4]));
		ImGui::Text("1400: %s", describe_pointer(chr[5]));
		ImGui::Text("1800: %s", describe_pointer(chr[6]));
		ImGui::Text("1C00: %s", describe_pointer(chr[7]));
		ImGui::Text("2000: %s", describe_pointer(chr[8]));
		ImGui::Text("2400: %s", describe_pointer(chr[9]));
		ImGui::Text("2800: %s", describe_pointer(chr[10]));
		ImGui::Text("2C00: %s", describe_pointer(chr[11]));
	}
	BasicMapper(NesFile& nf) :nf(&nf) {}
};
struct NROM : BasicMapper {
	void poweron() {
		set_prg8k(0, nf->get_prg8k(0));
		set_prg8k(1, nf->get_prg8k(1));
		set_prg8k(2, nf->get_prg8k(2));
		set_prg8k(3, nf->get_prg8k(3));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	NROM(NesFile& nf) :BasicMapper(nf) {}
};
template<typename T>
struct LatchMapper : BasicMapper {
	bool bus_conflict = false;
	LatchMapper(NesFile& nf, bool bus_conflict) :BasicMapper(nf), bus_conflict(bus_conflict) {}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if (addr & 0x8000) {
			if (bus_conflict)
				data &= cpu_read(addr);
			((T*)this)->on_latch(data);
		}
	}
};
struct UxROM : LatchMapper<UxROM> {
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void on_latch(uint8_t data) {
		set_prg16k(0, nf->get_prg16k(data));
	}
	UxROM(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
struct CNROM : LatchMapper<CNROM> {
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void on_latch(uint8_t data) {
		set_chr8k(nf->get_chr8k(data));
	}
	CNROM(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
struct AxROM : LatchMapper<AxROM> {
	void poweron() {
		set_prg32k(nf->get_prg32k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(MIRRORING_SCREENA);
	}
	void on_latch(uint8_t data) {
		set_mirroring(data & 0x10 ? MIRRORING_SCREENB : MIRRORING_SCREENA);
		set_prg32k(nf->get_prg32k(data&7));
	}
	AxROM(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
struct ColorDreams : LatchMapper<ColorDreams> {
	void poweron() {
		set_prg32k(nf->get_prg32k(0));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void on_latch(uint8_t data) {
		set_prg32k(nf->get_prg32k(data&3));
		set_chr8k(nf->get_chr8k(data>>4));
	}
	ColorDreams(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
struct GxROM : LatchMapper<GxROM> {
	void poweron() {
		set_prg32k(nf->get_prg32k(0));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void on_latch(uint8_t data) {
		set_prg32k(nf->get_prg32k(data>>4 & 3));
		set_chr8k(nf->get_chr8k(data&3));
	}
	GxROM(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
struct BNROM : LatchMapper<BNROM> {
	void poweron() {
		set_prg32k(nf->get_prg32k(0));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void on_latch(uint8_t data) {
		set_prg32k(nf->get_prg32k(data&3));
	}
	BNROM(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
struct NINA001 : BasicMapper {
	void poweron() {
		set_prg32k(nf->get_prg32k(0));
		set_chr4k(0, nf->get_chr4k(0));
		set_chr4k(1, nf->get_chr4k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		switch (addr) {
		case 0x7FFD: set_prg32k(nf->get_prg32k(data&1)); break;
		case 0x7FFE: set_chr4k(0, nf->get_chr4k(data&15)); break;
		case 0x7FFF: set_chr4k(1, nf->get_chr4k(data&15)); break;
		}
	}
	NINA001(NesFile& nf) :BasicMapper(nf) {}
};
struct CPROM : LatchMapper<CPROM> {
	void poweron() {
		set_prg32k(nf->get_prg32k(0));
		set_chr4k(0, nf->get_chr4k(0));
		set_chr4k(1, nf->get_chr4k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void on_latch(uint8_t data) {
		set_chr4k(1, nf->get_chr4k(data&3));
	}
	CPROM(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
struct UN1ROM : LatchMapper<UN1ROM> {
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void on_latch(uint8_t data) {
		set_prg16k(0, nf->get_prg16k(data>>2 & 7));
	}
	UN1ROM(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
struct UNROM_AND : LatchMapper<UNROM_AND> {
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(0));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void on_latch(uint8_t data) {
		set_prg16k(1, nf->get_prg16k(data&7));
	}
	UNROM_AND(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
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
			set_chr4k(0, nf->get_chr4k(reg_chr1));
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
	MMC2(NesFile& nf, bool isMMC4) :BasicMapper(nf), isMMC4(isMMC4) {}
};
struct DxROM : BasicMapper {
	int cur_reg = 0;
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		switch (addr & 0xE001) {
		case 0x8000:
			cur_reg = data & 7;
			break;
		case 0x8001:
			switch (cur_reg) {
			case 0: set_chr2k(0, nf->get_chr1k(data&0x3E)); break;
			case 1: set_chr2k(1, nf->get_chr1k(data&0x3E)); break;
			case 2: set_chr1k(4, nf->get_chr1k(data&0x3F)); break;
			case 3: set_chr1k(5, nf->get_chr1k(data&0x3F)); break;
			case 4: set_chr1k(6, nf->get_chr1k(data&0x3F)); break;
			case 5: set_chr1k(7, nf->get_chr1k(data&0x3F)); break;
			case 6: set_prg8k(0, nf->get_prg8k(data&0x0F)); break;
			case 7: set_prg8k(1, nf->get_prg8k(data&0x0F)); break;
			}
			break;
		}
	}
	DxROM(NesFile& nf) :BasicMapper(nf) {}
};
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
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(MIRRORING_VERTICAL);
	}
	void irq_tick() {
		if (!ppu_a12_d3 && !ppu_a12_d2 && !ppu_a12_d1 && ppu_a12) {
			if (irq_reload || irq_counter == 0) {
				if (!irq_reload)
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
		irq_tick();
		switch (addr & 0xE001) {
		case 0x8000:
			cur_reg = data & 7;
			prg_swap = data & 0x40;
			chr_swap = data & 0x80;
			break;
		case 0x8001:
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
void mapper_cleanup() {
	if (mapper != &noop_mapper)
		delete mapper;
	mapper = &noop_mapper;
}
// FIXME: lots of bugs (in particual buffer overruns) in mapper impls
#define MAPNO(maj, min) ((maj)<<4 | (min))
void mapper_setup(NesFile& nf) {
	mapper_cleanup();
	switch (nf.mapper) {
	case MAPNO(0, 0): mapper = new NROM(nf); break;
	case MAPNO(1, 0): mapper = new MMC1(nf); break;
	case MAPNO(2, 0): [[fallthrough]];
	case MAPNO(2, 2): mapper = new UxROM(nf, true); break;
	case MAPNO(2, 1): mapper = new UxROM(nf, false); break;
	case MAPNO(3, 0): [[fallthrough]];
	case MAPNO(3, 2): mapper = new CNROM(nf, true); break;
	case MAPNO(3, 1): mapper = new CNROM(nf, false); break;
	case MAPNO(4, 0): mapper = new MMC3(nf); break;
	case MAPNO(7, 0): [[fallthrough]];
	case MAPNO(7, 2): mapper = new AxROM(nf, true); break;
	case MAPNO(7, 1): mapper = new AxROM(nf, false); break;
	case MAPNO(9, 0): mapper = new MMC2(nf, false); break;
	case MAPNO(10, 0): mapper = new MMC2(nf, true); break;
	case MAPNO(11, 0): mapper = new ColorDreams(nf, false); break;
	case MAPNO(13, 0): mapper = new CPROM(nf, true); break;
	case MAPNO(34, 0): mapper = nf.chrrom.size() > 8192 ? (Mapper*)new NINA001(nf) : new BNROM(nf, true); break;
	case MAPNO(34, 1): mapper = new NINA001(nf); break;
	case MAPNO(34, 2): mapper = new BNROM(nf, true); break;
	case MAPNO(66, 0): mapper = new GxROM(nf, false); break;
	case MAPNO(94, 0): mapper = new UN1ROM(nf, true); break;
	case MAPNO(180, 0): mapper = new UNROM_AND(nf, true); break;
	case MAPNO(206, 0): mapper = new DxROM(nf); break;
	}
}
}
