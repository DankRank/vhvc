#ifndef VHVC_MAPPER
#define VHVC_MAPPER
#include "common.hh"
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
		void set_chr1k(int i, uint8_t* bank);
		void set_chr2k(int i, uint8_t* bank);
		void set_chr4k(int slot, uint8_t *bank);
		void set_chr8k(uint8_t *bank);
		void set_prg8k(int i, uint8_t* bank);
		void set_prg16k(int i, uint8_t* bank);
		void set_prg32k(uint8_t* bank);
		void chrram_check();
		void set_nametable(int i, uint8_t *nt);
		void set_nametables(uint8_t* nt0, uint8_t* nt1, uint8_t* nt2, uint8_t* nt3);
		void set_mirroring(int type);
		uint8_t cpu_read(uint16_t addr);
		void cpu_write(uint16_t addr, uint8_t data);
		uint8_t ppu_read(uint16_t addr);
		void ppu_write(uint16_t addr, uint8_t data);
		const char *describe_pointer(uint8_t *ptr);
		void debug_gui();
		BasicMapper(NesFile& nf);
	};
	template<typename T>
	Mapper *new_mapper(NesFile &nf);
	template<typename T>
	Mapper *new_mapper(NesFile &nf, bool param);
}
#endif
