#include "mapper.hh"
#include "bus.hh"
namespace vhvc {
#define DECLARE_MAPPER(T) \
	template<> Mapper *new_mapper<T>(NesFile &nf) { return new T(nf); }
#define DECLARE_MAPPER_BOOL(T) \
	template<> Mapper *new_mapper<T>(NesFile &nf, bool param) { return new T(nf, param); }
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
	UxROM(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_BOOL(UxROM)
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
DECLARE_MAPPER_BOOL(CNROM)
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
DECLARE_MAPPER_BOOL(AxROM)
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
DECLARE_MAPPER_BOOL(ColorDreams)
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
DECLARE_MAPPER_BOOL(GxROM)
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
DECLARE_MAPPER_BOOL(BNROM)
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
	CPROM(NesFile& nf, bool bus_conflict) :LatchMapper(nf, bus_conflict) {}
};
DECLARE_MAPPER_BOOL(CPROM)
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
DECLARE_MAPPER_BOOL(UN1ROM)
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
DECLARE_MAPPER_BOOL(UNROM_AND)
}
