#ifndef VHVC_MAPPER
#define VHVC_MAPPER
#include "nesfile.hh"
#include "cpu.hh"
namespace vhvc {
	struct Mapper {
		virtual void poweron();
		virtual void reset();
		virtual uint8_t cpu_read(uint16_t addr);
		virtual void cpu_write(uint16_t addr, uint8_t data);
		virtual uint8_t ppu_read(uint16_t addr);
		virtual void ppu_write(uint16_t addr, uint8_t data);
		virtual void debug_gui();
	};
	extern Mapper* mapper;
	void mapper_cleanup();
	void mapper_setup(NesFile& nf);
}
#endif
