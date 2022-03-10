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
}
