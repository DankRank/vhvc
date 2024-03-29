#include "mapper.hh"
#include "bus.hh"
namespace vhvc {
struct NROM : BasicMapper {
	void poweron() {
		has_prgram = true; // for family basic
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
DECLARE_MAPPER(NROM)
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
	UxROM(NesFile& nf, int bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_INT(UxROM)
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
	CNROM(NesFile& nf, int bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_INT(CNROM)
struct CNROMCopyProtection : LatchMapper<CNROMCopyProtection> {
	int bank;
	bool chr_enabled = false;
	bool is_seicross = false;
	bool first_read = false;
	void poweron() {
		first_read = true;
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void on_latch(uint8_t data) {
		// This needs more investigation, but I think an alternative heuristic could be to check
		// if the same value gets written twice, because the games run the check in a loop.
		// The downside is some wasted cycles compared to the real hardware.
		if (first_read) {
			first_read = false;
			is_seicross = data == 0x21;
		}
		if (bank == -1)
			chr_enabled = (data&15 && data != 0x13) != is_seicross;
		else
			chr_enabled = (data&3) == bank;
	}
	uint8_t ppu_read(uint16_t addr) {
		if (addr < 0x2000 && !chr_enabled)
			return addr&0xFF;
		return LatchMapper::ppu_read(addr);
	}
	CNROMCopyProtection(NesFile& nf, int bank) :LatchMapper(nf, true), bank(bank) {}
};
DECLARE_MAPPER_INT(CNROMCopyProtection)
struct AxROM : LatchMapper<AxROM> {
	void poweron() {
		set_prg32k(nf->get_prg32k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(MIRRORING_SCREENA);
	}
	void on_latch(uint8_t data) {
		set_mirroring(data & 0x10 ? MIRRORING_SCREENB : MIRRORING_SCREENA);
		set_prg32k(nf->get_prg32k(data&15));
	}
	AxROM(NesFile& nf, int bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_INT(AxROM)
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
	ColorDreams(NesFile& nf, int bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_INT(ColorDreams)
struct DeathRace : BasicMapper {
	void poweron() {
		set_prg32k(nf->get_prg32k(0));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if (addr & 0x8000) {
			data |= 1;
			data &= cpu_read(addr);
			set_prg32k(nf->get_prg32k(data&3));
			set_chr8k(nf->get_chr8k(data>>4));
		}
	}
	DeathRace(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(DeathRace)
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
	GxROM(NesFile& nf, int bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_INT(GxROM)
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
	BNROM(NesFile& nf, int bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_INT(BNROM)
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
DECLARE_MAPPER(NINA001)
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
	CPROM(NesFile& nf, int bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_INT(CPROM)
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
	UN1ROM(NesFile& nf, int bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_INT(UN1ROM)
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
	UNROM_AND(NesFile& nf, int bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_INT(UNROM_AND)
struct Camerica : BasicMapper {
	int variant;
	int bank = 0;
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		if (variant == VARIANT_CamericaQuattro)
			set_prg16k(1, nf->get_prg16k(3));
		else
			set_prg16k(1, nf->get_prg16k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		if (variant == VARIANT_CamericaFireHawk)
			set_mirroring(MIRRORING_SCREENA);
		else
			set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if (addr >= 0xC000) {
			switch (variant) {
			case VARIANT_CamericaCompat: [[fallthrough]];
			case VARIANT_CamericaBasic: bank = data&15; break;
			case VARIANT_CamericaFireHawk: bank = data&7; break;
			case VARIANT_CamericaQuattro: [[fallthrough]];
			case VARIANT_CamericaAladdin: bank = bank&0x0C | data&3; break;
			}
			set_prg16k(0, nf->get_prg16k(bank));
		} else if (addr >= 0x8000) {
			switch (variant) {
			case VARIANT_CamericaCompat:
				if (addr < 0x9000)
					break;
				[[fallthrough]];
			case VARIANT_CamericaFireHawk:
				if (addr < 0xA000)
					set_mirroring(data&16 ? MIRRORING_SCREENA : MIRRORING_SCREENB);
				break;
			case VARIANT_CamericaAladdin:
				data = data>>1 & 0x08 | data<<1 & 0x10; // swap outer bits
				[[fallthrough]];
			case VARIANT_CamericaQuattro:
				bank = data>>1 & 0x0C | bank&0x03;
				set_prg16k(0, nf->get_prg16k(bank));
				set_prg16k(1, nf->get_prg16k(bank&0x0C | 3));
				break;
			}
		}
	}
	Camerica(NesFile& nf, int variant) :BasicMapper(nf), variant(variant) {}
};
DECLARE_MAPPER_INT(Camerica)
struct Action52 : BasicMapper {
	bool openbus = false;
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(1));
		set_chr8k(nf->get_chr8k(0));
		set_mirroring(MIRRORING_VERTICAL);
	}
	uint8_t cpu_read(uint16_t addr) {
		return openbus ? cpu::data_bus : BasicMapper::cpu_read(addr);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if (addr & 0x8000) {
			set_chr8k(nf->get_chr8k((addr&15) << 2 | data&3));
			int bank = addr>>6 & 0x7F;
			openbus = bank >= 0x40 && bank < 0x60;
			if (bank >= 0x60)
				bank -= 0x20;
			if (addr & 0x20) {
				set_prg16k(0, nf->get_prg16k(bank));
				set_prg16k(1, nf->get_prg16k(bank));
			} else {
				set_prg16k(0, nf->get_prg16k(bank&~1));
				set_prg16k(1, nf->get_prg16k(bank|1));
			}
			set_mirroring(addr&0x2000 ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
		}
	}
	Action52(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(Action52)
}
