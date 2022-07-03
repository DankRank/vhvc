#ifndef VHVC_NSFFILE
#define VHVC_NSFFILE
#include "common.hh"
#include <vector>
#include "span.hh"
#include "io.hh"
namespace vhvc {
	struct NsfFile {
		std::vector<uint8_t> rom;
		uint8_t total_songs;
		uint8_t starting_song;
		uint16_t load, init, play;
		uint8_t bank_init[8];
		int bank_count = 0;
		bool banking = false;
		bool is_valid = false;

		NsfFile(std::vector<uint8_t>&& buf);

		NsfFile() = default;
		~NsfFile() = default;
		NsfFile(const NsfFile&) = delete;
		NsfFile& operator=(const NsfFile&) = delete;
		NsfFile(NsfFile&&) = default;
		NsfFile& operator=(NsfFile&&) = default;
	};
	bool load_nsf(std::vector<uint8_t>& buf);
}
#endif
