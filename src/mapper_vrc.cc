#include "mapper.hh"
#include "bus.hh"
namespace vhvc {
#define DECLARE_MAPPER(T) \
	template<> Mapper *new_mapper<T>(NesFile &nf) { return new T(nf); }
#define DECLARE_MAPPER_BOOL(T) \
	template<> Mapper *new_mapper<T>(NesFile &nf, bool param) { return new T(nf, param); }
struct VRC1 : BasicMapper {
	uint8_t chr0 = 0;
	uint8_t chr1 = 0;
	void poweron() {
		set_prg8k(0, nf->get_prg8k(0));
		set_prg8k(1, nf->get_prg8k(0));
		set_prg8k(2, nf->get_prg8k(0));
		set_prg8k(3, nf->get_prg8k(-1));
		set_chr4k(0, nf->get_chr4k(0));
		set_chr4k(1, nf->get_chr4k(0));
		set_mirroring(MIRRORING_VERTICAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		data &= 15;
		switch (addr & 0xF000) {
		case 0x8000: set_prg8k(0, nf->get_prg8k(data)); break;
		case 0xA000: set_prg8k(1, nf->get_prg8k(data)); break;
		case 0xC000: set_prg8k(2, nf->get_prg8k(data)); break;
		case 0x9000:
			set_mirroring(data&1 ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
			chr0 = chr0&0x0F | (data&2 ? 0x10 : 0);
			chr1 = chr1&0x0F | (data&4 ? 0x10 : 0);
			set_chr4k(0, nf->get_chr4k(chr0));
			set_chr4k(1, nf->get_chr4k(chr1));
			break;
		case 0xE000:
			chr0 = chr0&0x10 | data;
			set_chr4k(0, nf->get_chr4k(chr0));
			break;
		case 0xF000:
			chr1 = chr1&0x10 | data;
			set_chr4k(1, nf->get_chr4k(chr1));
			break;
		}
	}
	VRC1(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(VRC1)
}
