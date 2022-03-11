#include "mapper.hh"
#include "bus.hh"
namespace vhvc {
#define DECLARE_MAPPER(T) \
	template<> Mapper *new_mapper<T>(NesFile &nf) { return new T(nf); }
#define DECLARE_MAPPER_INT(T) \
	template<> Mapper *new_mapper<T>(NesFile &nf, int param) { return new T(nf, param); }
struct Jaleco087 : BasicMapper {
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if ((addr & 0xE000) == 0x6000) {
			set_chr8k(nf->get_chr8k(data<<1 & 2 | data>>1 & 1));
		}
	}
	Jaleco087(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(Jaleco087)
struct Jaleco140 : BasicMapper {
	void poweron() {
		set_prg32k(nf->get_prg32k(0));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if ((addr & 0xE000) == 0x6000) {
			set_prg32k(nf->get_prg32k(data>>4 & 3));
			set_chr8k(nf->get_chr8k(data & 15));
		}
	}
	Jaleco140(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(Jaleco140)
struct Jaleco086 : BasicMapper {
	void poweron() {
		set_prg32k(nf->get_prg32k(0));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if ((addr & 0x7000) == 0x6000) { // NOTE: /ROMSEL delay bug. Does it have bus conflicts?
			set_prg32k(nf->get_prg32k(data>>4 & 3));
			set_chr8k(nf->get_chr8k(data>>4 & 4 | data&3));
		}
		// TODO: 7000-7FFF sampled sound
	}
	Jaleco086(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(Jaleco086)
struct Mapper038 : BasicMapper {
	void poweron() {
		set_prg32k(nf->get_prg32k(0));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if ((addr & 0x7000) == 0x7000) { // NOTE: /ROMSEL delay bug. Does it have bus conflicts?
			set_prg32k(nf->get_prg32k(data & 3));
			set_chr8k(nf->get_chr8k(data>>2 & 3));
		}
	}
	Mapper038(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(Mapper038)
struct Jaleco078 : BasicMapper {
	bool holy_diver = false;
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(holy_diver ? MIRRORING_HORIZONTAL : MIRRORING_SCREENA);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if (addr & 0x8000) {
			data &= cpu_read(addr);
			set_prg16k(0, nf->get_prg16k(data&7));
			if (holy_diver)
				set_mirroring(data & 16 ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
			else
				set_mirroring(data & 16 ? MIRRORING_SCREENB : MIRRORING_SCREENA);
			set_chr8k(nf->get_chr8k(data>>4));
		}
	}
	Jaleco078(NesFile& nf, int holy_diver) :BasicMapper(nf), holy_diver(holy_diver) {}
};
DECLARE_MAPPER_INT(Jaleco078)
struct Jaleco072_092 : BasicMapper {
	bool and_gate;
	bool cpu_latch = false;
	bool ppu_latch = false;
	void poweron() {
		cpu_latch = ppu_latch = false;
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if (addr & 0x8000) {
			data &= cpu_read(addr);
			if (data&0x80 && !cpu_latch)
				set_prg16k(and_gate ? 1 : 0, nf->get_prg16k(data&15));
			if (data&0x40 && !ppu_latch)
				set_chr8k(nf->get_chr8k(data&15));
			cpu_latch = data&0x80;
			ppu_latch = data&0x40;
			// TODO: sound hw?
		}
	}
	Jaleco072_092(NesFile& nf, int variant) :BasicMapper(nf), and_gate(variant) {}
};
DECLARE_MAPPER_INT(Jaleco072_092)
struct Jaleco101 : BasicMapper {
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if ((addr & 0xE000) == 0x6000) {
			set_chr8k(nf->get_chr8k(data));
		}
	}
	Jaleco101(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(Jaleco101)
struct JalecoSS88006 : BasicMapper {
	int prgreg[3] = {0};
	int chrreg[8] = {0};
	bool prgram_wr_enable = false;
	uint16_t irq_latch = 0;
	uint16_t irq_counter = 0;
	uint8_t irq_mode = 0;
	void irq_tick() {
		if (irq_mode & 1) {
			uint16_t mask;
			if (irq_mode & 8) {
				mask = 0x000F;
			} else if (irq_mode & 4) {
				mask = 0x00FF;
			} else if (irq_mode & 2) {
				mask = 0x0FFF;
			} else {
				mask = 0xFFFF;
			}
			if ((irq_counter & mask) == 0) {
				irq_counter |= mask;
				irq_raise(IRQ_MAPPER);
			} else {
				irq_counter--;
			}
		}
	}
	void poweron() {
		set_prg8k(0, nf->get_prg8k(0));
		set_prg8k(1, nf->get_prg8k(0));
		set_prg8k(2, nf->get_prg8k(0));
		set_prg8k(3, nf->get_prg8k(-1));
		for (int i = 0; i < 8; i++)
			set_chr1k(i, nf->get_chr1k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	uint8_t cpu_read(uint16_t addr) {
		if (!bus_inspect)
			irq_tick();
		return BasicMapper::cpu_read(addr);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		if (!bus_inspect)
			irq_tick();
		if (addr > 0x6000 && addr < 0x8000 && !prgram_wr_enable)
			return;
		BasicMapper::cpu_write(addr, data);
		switch (addr&0xF003) {
		case 0x8000: set_prg8k(0, nf->get_prg8k((prgreg[0] = prgreg[0]&0x30 | data&0x0F     ))); break;
		case 0x8001: set_prg8k(0, nf->get_prg8k((prgreg[0] = prgreg[0]&0x0F | data<<4 & 0x30))); break;
		case 0x8002: set_prg8k(1, nf->get_prg8k((prgreg[1] = prgreg[1]&0x30 | data&0x0F     ))); break;
		case 0x8003: set_prg8k(1, nf->get_prg8k((prgreg[1] = prgreg[1]&0x0F | data<<4 & 0x30))); break;
		case 0x9000: set_prg8k(2, nf->get_prg8k((prgreg[2] = prgreg[2]&0x30 | data&0x0F     ))); break;
		case 0x9001: set_prg8k(2, nf->get_prg8k((prgreg[2] = prgreg[2]&0x0F | data<<4 & 0x30))); break;
		case 0x9002:
			has_prgram = data&1;
			prgram_wr_enable = data&2;
			break;
		case 0x9003: break;
		case 0xA000: set_chr1k(0, nf->get_chr1k((chrreg[0] = chrreg[0]&0xF0 | data&0x0F     ))); break;
		case 0xA001: set_chr1k(0, nf->get_chr1k((chrreg[0] = chrreg[0]&0x0F | data<<4 & 0xF0))); break;
		case 0xA002: set_chr1k(1, nf->get_chr1k((chrreg[1] = chrreg[1]&0xF0 | data&0x0F     ))); break;
		case 0xA003: set_chr1k(1, nf->get_chr1k((chrreg[1] = chrreg[1]&0x0F | data<<4 & 0xF0))); break;
		case 0xB000: set_chr1k(2, nf->get_chr1k((chrreg[2] = chrreg[2]&0xF0 | data&0x0F     ))); break;
		case 0xB001: set_chr1k(2, nf->get_chr1k((chrreg[2] = chrreg[2]&0x0F | data<<4 & 0xF0))); break;
		case 0xB002: set_chr1k(3, nf->get_chr1k((chrreg[3] = chrreg[3]&0xF0 | data&0x0F     ))); break;
		case 0xB003: set_chr1k(3, nf->get_chr1k((chrreg[3] = chrreg[3]&0x0F | data<<4 & 0xF0))); break;
		case 0xC000: set_chr1k(4, nf->get_chr1k((chrreg[4] = chrreg[4]&0xF0 | data&0x0F     ))); break;
		case 0xC001: set_chr1k(4, nf->get_chr1k((chrreg[4] = chrreg[4]&0x0F | data<<4 & 0xF0))); break;
		case 0xC002: set_chr1k(5, nf->get_chr1k((chrreg[5] = chrreg[5]&0xF0 | data&0x0F     ))); break;
		case 0xC003: set_chr1k(5, nf->get_chr1k((chrreg[5] = chrreg[5]&0x0F | data<<4 & 0xF0))); break;
		case 0xD000: set_chr1k(6, nf->get_chr1k((chrreg[6] = chrreg[6]&0xF0 | data&0x0F     ))); break;
		case 0xD001: set_chr1k(6, nf->get_chr1k((chrreg[6] = chrreg[6]&0x0F | data<<4 & 0xF0))); break;
		case 0xD002: set_chr1k(7, nf->get_chr1k((chrreg[7] = chrreg[7]&0xF0 | data&0x0F     ))); break;
		case 0xD003: set_chr1k(7, nf->get_chr1k((chrreg[7] = chrreg[7]&0x0F | data<<4 & 0xF0))); break;
		case 0xE000: irq_latch = irq_latch & 0xFFF0 | data & 0x000F; break;
		case 0xE001: irq_latch = irq_latch & 0xFF0F | data<<4 & 0x00F0; break;
		case 0xE002: irq_latch = irq_latch & 0xF0FF | data<<8 & 0x0F00; break;
		case 0xE003: irq_latch = irq_latch & 0x0FFF | data<<12 & 0xF000; break;
		case 0xF000: irq_ack(IRQ_MAPPER); irq_counter = irq_latch; break;
		case 0xF001: irq_ack(IRQ_MAPPER); irq_mode = data&15; break;
		case 0xF002: {
			constexpr int mirroring_modes[4] = { MIRRORING_HORIZONTAL, MIRRORING_VERTICAL, MIRRORING_SCREENA, MIRRORING_SCREENB };
			set_mirroring(mirroring_modes[data&3]);
			break;
		}
		case 0xF003: // TODO: expansion sound
			break;
		}
	}
	JalecoSS88006(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(JalecoSS88006)
}
