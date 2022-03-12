#include "mapper.hh"
#include "bus.hh"
namespace vhvc {
struct Namcot108 : BasicMapper {
	bool prg_fixed = false;
	uint8_t or_mask = 0;
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
			case 2: set_chr1k(4, nf->get_chr1k(data&0x3F | or_mask)); break;
			case 3: set_chr1k(5, nf->get_chr1k(data&0x3F | or_mask)); break;
			case 4: set_chr1k(6, nf->get_chr1k(data&0x3F | or_mask)); break;
			case 5: set_chr1k(7, nf->get_chr1k(data&0x3F | or_mask)); break;
			case 6: if (!prg_fixed) set_prg8k(0, nf->get_prg8k(data&0x0F)); break;
			case 7: if (!prg_fixed) set_prg8k(1, nf->get_prg8k(data&0x0F)); break;
			}
			break;
		}
	}
	Namcot108(NesFile& nf, int variant) :BasicMapper(nf),
		prg_fixed(variant == VARIANT_Namcot108_FixedPrg),
		or_mask(variant == VARIANT_Namcot108_SplitChr ? 0x40 : 0x00) {}
};
DECLARE_MAPPER_INT(Namcot108)
struct Namcot3453 : Namcot108 {
	void poweron() {
		Namcot108::poweron();
		set_mirroring(MIRRORING_SCREENA);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		Namcot108::cpu_write(addr, data);
		if (addr & 0x8000)
			set_mirroring(data & 0x40 ? MIRRORING_SCREENB : MIRRORING_SCREENA);
	}
	Namcot3453(NesFile& nf) :Namcot108(nf, VARIANT_Namcot108_SplitChr) {}
};
DECLARE_MAPPER(Namcot3453)
struct Namcot3446 : BasicMapper {
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
			case 2: set_chr2k(0, nf->get_chr2k(data&0x3F)); break;
			case 3: set_chr2k(1, nf->get_chr2k(data&0x3F)); break;
			case 4: set_chr2k(2, nf->get_chr2k(data&0x3F)); break;
			case 5: set_chr2k(3, nf->get_chr2k(data&0x3F)); break;
			case 6: set_prg8k(0, nf->get_prg8k(data&0x0F)); break;
			case 7: set_prg8k(1, nf->get_prg8k(data&0x0F)); break;
			}
			break;
		}
	}
	Namcot3446(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(Namcot3446)
struct Namcot3425 : Namcot108 {
	void poweron() {
		Namcot108::poweron();
		set_mirroring(MIRRORING_SCREENA);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		Namcot108::cpu_write(addr, data);
		if ((addr & 0xE001) == 0x8001 && cur_reg < 2) {
			set_nametable(cur_reg*2 + 0, data&0x20 ? ppu_ram1 : ppu_ram0);
			set_nametable(cur_reg*2 + 1, data&0x20 ? ppu_ram1 : ppu_ram0);
		}
	}
	Namcot3425(NesFile& nf) :Namcot108(nf, VARIANT_Namcot108_Normal) {}
};
DECLARE_MAPPER(Namcot3425)
}
