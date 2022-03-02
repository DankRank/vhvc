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

	void BasicMapper::set_chr1k(int i, uint8_t* bank) {
		chr[i] = bank;
	}
	void BasicMapper::set_chr2k(int i, uint8_t* bank) {
		chr[i*2] = bank;
		chr[i*2+1] = &bank[1024];
	}
	void BasicMapper::set_chr4k(int slot, uint8_t *bank) {
		for (int i = 0; i < 4; i++)
			chr[slot*4+i] = &bank[1024*i];
	}
	void BasicMapper::set_chr8k(uint8_t *bank) {
		for (int i = 0; i < 8; i++)
			chr[i] = &bank[1024*i];
	}
	void BasicMapper::set_prg8k(int i, uint8_t* bank) {
		prg[i] = bank;
	}
	void BasicMapper::set_prg16k(int i, uint8_t* bank) {
		prg[i*2+0] = &bank[0*8192];
		prg[i*2+1] = &bank[1*8192];
	}
	void BasicMapper::set_prg32k(uint8_t* bank) {
		prg[0] = &bank[0*8192];
		prg[1] = &bank[1*8192];
		prg[2] = &bank[2*8192];
		prg[3] = &bank[3*8192];
	}
	void BasicMapper::chrram_check() {
		has_chrram = !nf->chrrom.size();
	}
	void BasicMapper::set_nametable(int i, uint8_t *nt) {
		chr[12+i] = chr[8+i] = nt;
	}
	void BasicMapper::set_nametables(uint8_t* nt0, uint8_t* nt1, uint8_t* nt2, uint8_t* nt3) {
		chr[12] = chr[8] = nt0;
		chr[13] = chr[9] = nt1;
		chr[14] = chr[10] = nt2;
		chr[15] = chr[11] = nt3;
	}
	void BasicMapper::set_mirroring(int type) {
		switch (type) {
			case MIRRORING_HORIZONTAL: set_nametables(ppu_ram0, ppu_ram0, ppu_ram1, ppu_ram1); break;
			case MIRRORING_VERTICAL:   set_nametables(ppu_ram0, ppu_ram1, ppu_ram0, ppu_ram1); break;
			case MIRRORING_SCREENA:    set_nametables(ppu_ram0, ppu_ram0, ppu_ram0, ppu_ram0); break;
			case MIRRORING_SCREENB:    set_nametables(ppu_ram1, ppu_ram1, ppu_ram1, ppu_ram1); break;
		}
	}

	uint8_t BasicMapper::cpu_read(uint16_t addr) {
		if (addr & 0x8000)
			return prg[addr>>13 & 3][addr & 0x1FFF];
		if (has_prgram && (addr & 0xE000) == 0x6000)
			return prgram[addr & 0x1FFF];
		return cpu::data_bus;
	}
	void BasicMapper::cpu_write(uint16_t addr, uint8_t data) {
		if (has_prgram && (addr & 0xE000) == 0x6000)
			prgram[addr & 0x1FFF] = data;
	}
	uint8_t BasicMapper::ppu_read(uint16_t addr) {
		return chr[addr >> 10][addr & 0x3FF];
	}
	void BasicMapper::ppu_write(uint16_t addr, uint8_t data) {
		if (has_chrram || addr >= 0x2000) {
			chr[addr>>10][addr & 0x3FF] = data;
		}
	}
	const char *BasicMapper::describe_pointer(uint8_t *ptr) {
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
	void BasicMapper::debug_gui() {
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
	BasicMapper::BasicMapper(NesFile& nf) :nf(&nf) {}
struct NROM;
struct UxROM;
struct CNROM;
struct AxROM;
struct ColorDreams;
struct GxROM;
struct BNROM;
struct NINA001;
struct CPROM;
struct UN1ROM;
struct UNROM_AND;
struct MMC1;
struct MMC2;
struct DxROM;
struct MMC3;
struct MMC5;
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
	case MAPNO(0, 0): mapper = new_mapper<NROM>(nf); break;
	case MAPNO(1, 0): mapper = new_mapper<MMC1>(nf); break;
	case MAPNO(2, 0): [[fallthrough]];
	case MAPNO(2, 2): mapper = new_mapper<UxROM>(nf, true); break;
	case MAPNO(2, 1): mapper = new_mapper<UxROM>(nf, false); break;
	case MAPNO(3, 0): [[fallthrough]];
	case MAPNO(3, 2): mapper = new_mapper<CNROM>(nf, true); break;
	case MAPNO(3, 1): mapper = new_mapper<CNROM>(nf, false); break;
	case MAPNO(4, 0): mapper = new_mapper<MMC3>(nf); break;
	case MAPNO(5, 0): mapper = new_mapper<MMC5>(nf); break;
	case MAPNO(7, 0): [[fallthrough]];
	case MAPNO(7, 2): mapper = new_mapper<AxROM>(nf, true); break;
	case MAPNO(7, 1): mapper = new_mapper<AxROM>(nf, false); break;
	case MAPNO(9, 0): mapper = new_mapper<MMC2>(nf, false); break;
	case MAPNO(10, 0): mapper = new_mapper<MMC2>(nf, true); break;
	case MAPNO(11, 0): mapper = new_mapper<ColorDreams>(nf, false); break;
	case MAPNO(13, 0): mapper = new_mapper<CPROM>(nf, true); break;
	case MAPNO(34, 0): mapper = nf.chrrom.size() > 8192 ? (Mapper*)new_mapper<NINA001>(nf) : new_mapper<BNROM>(nf, true); break;
	case MAPNO(34, 1): mapper = new_mapper<NINA001>(nf); break;
	case MAPNO(34, 2): mapper = new_mapper<BNROM>(nf, true); break;
	case MAPNO(66, 0): mapper = new_mapper<GxROM>(nf, false); break;
	case MAPNO(94, 0): mapper = new_mapper<UN1ROM>(nf, true); break;
	case MAPNO(180, 0): mapper = new_mapper<UNROM_AND>(nf, true); break;
	case MAPNO(206, 0): mapper = new_mapper<DxROM>(nf); break;
	}
}
}
