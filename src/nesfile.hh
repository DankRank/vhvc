#ifndef VHVC_NESFILE
#define VHVC_NESFILE
#include "common.hh"
#include <vector>
#include "span.hh"
#include "io.hh"
namespace vhvc {
	struct NesFile {
		std::vector<uint8_t> buf;
		std::vector<uint8_t> extra;
		span_u8 header;
		span_u8 trainer;
		span_u8 prgrom;
		span_u8 chrrom;
		span_u8 chrram;
		span_u8 chr;

		uint16_t mapper = 0;
		uint32_t prgram_size = 0;
		uint32_t prgnvram_size = 0;
		uint32_t chrram_size = 0;
		uint32_t chrnvram_size = 0;
		bool is_ines = false;
		bool is_valid = false;
		bool vertical = false;
		bool battery = false;
		bool four_screen = false;

		int count_prg8k() { return prgrom.size() / 8192; }
		int count_prg16k() { return prgrom.size() / 16384; }
		int count_prg32k() { return prgrom.size() / 32768; }
		int count_chr1k() { return chr.size() / 1024; }
		int count_chr2k() { return chr.size() / 2048; }
		int count_chr4k() { return chr.size() / 4096; }
		int count_chr8k() { return chr.size() / 8192; }
		int normalize(int no, int max) {
			if (no < 0)
				no += max;
			if (no < 0) {
				assert(!"rom too small?");
				no = 0;
			}
			return no % max;
		}
		uint8_t* get_prg8k(int no) { return prgrom.data() + normalize(no, count_prg8k())*8192; }
		uint8_t* get_prg16k(int no) { return prgrom.data() + normalize(no, count_prg16k())*16384; }
		uint8_t* get_prg32k(int no) { return prgrom.data() + normalize(no, count_prg32k())*32768; }
		uint8_t* get_chr1k(int no) { return chr.data() + normalize(no, count_chr1k())*1024; }
		uint8_t* get_chr2k(int no) { return chr.data() + normalize(no, count_chr2k())*2048; }
		uint8_t* get_chr4k(int no) { return chr.data() + normalize(no, count_chr4k())*4096; }
		uint8_t* get_chr8k(int no) { return chr.data() + normalize(no, count_chr8k())*8192; }

		NesFile::NesFile(std::vector<uint8_t>&& buf);

		NesFile() = default;
		~NesFile() = default;
		NesFile(const NesFile&) = delete;
		NesFile& operator=(const NesFile&) = delete;
		NesFile(NesFile&&) = default;
		NesFile& operator=(NesFile&&) = default;
	};
	bool load_rom(std::vector<uint8_t>& buf);
	bool apply_patch(std::vector<uint8_t>& buf, cspan_u8 pbuf);
	bool apply_patch(std::vector<uint8_t>& buf, File& patch);
}
#endif
