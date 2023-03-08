#ifndef VHVC_FDSFILE
#define VHVC_FDSFILE
#include "common.hh"
#include <vector>
#include <array>
#include "span.hh"
#include "io.hh"
namespace vhvc {
	struct FdsFile {
		std::vector<uint8_t> image;
		std::array<uint8_t, 8192> bios;
		std::vector<std::vector<span_u8>> sides;
		std::vector<std::vector<uint8_t>> serial_sides;
		bool is_valid = false;

		static constexpr uint16_t crc(cspan_u8 sp);

		FdsFile(std::vector<uint8_t>&& buf);

		FdsFile() = default;
		~FdsFile() = default;
		FdsFile(const FdsFile&) = delete;
		FdsFile& operator=(const FdsFile&) = delete;
		FdsFile(FdsFile&&) = default;
		FdsFile& operator=(FdsFile&&) = default;
	};
	bool load_fds(std::vector<uint8_t>& buf);
}
#endif
