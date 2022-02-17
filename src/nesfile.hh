#ifndef VHVC_NESFILE
#define VHVC_NESFILE
#include <vector>
#include "span.hh"
#include "io.hh"
namespace vhvc {
	struct NesFile {
		std::vector<uint8_t> buf;
		span_u8 header;
		span_u8 trainer;
		span_u8 prgrom;
		span_u8 chrrom;

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
