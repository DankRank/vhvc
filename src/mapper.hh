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
	Mapper *new_mapper(NesFile &nf, int param);
	enum MapperVariant {
		// discrete logic mappers
		NO_BUS_CONFLICTS = 0,
		HAS_BUS_CONFLICTS = 1,
		// MMC2/MMC4
		VARIANT_MMC2 = 0,
		VARIANT_MMC4 = 1,
		// VRC2/VRC4
		VARIANT_VRC2a = 0,
		VARIANT_VRC2b,
		VARIANT_VRC2c,
		VARIANT_VRC4a,
		VARIANT_VRC4b,
		VARIANT_VRC4c,
		VARIANT_VRC4d,
		VARIANT_VRC4e,
		VARIANT_VRC4f,
		VARIANT_Mapper21,
		VARIANT_Mapper23,
		VARIANT_Mapper25,
		// VRC6
		VARIANT_VRC6a = 0,
		VARIANT_VRC6b,
		// VRC7
		VARIANT_Mapper85 = 0,
		VARIANT_VRC7b,
		VARIANT_VRC7a,
		// Camerica
		VARIANT_CamericaBasic = 0, // BF9093
		VARIANT_CamericaFireHawk, // BF9097
		VARIANT_CamericaQuattro, // BF9096
		VARIANT_CamericaAladdin,
		VARIANT_CamericaCompat,
	};
	template<typename T>
	Mapper *new_mapper(NesFile &nf, MapperVariant param) {
		return new_mapper<T>(nf, (int)param);
	}
}
#endif
