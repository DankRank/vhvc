#include "nsffile.hh"
#include "mapper.hh"
namespace vhvc {
// TODO: copy pasted from nesfile.cc, should probably be in some common place
static bool split_span(span_u8& rest, size_t len, span_u8& out) {
	if (rest.size() < len)
		return false;
	out = rest.first(len);
	rest = rest.subspan(len);
	return true;
}
NsfFile::NsfFile(std::vector<uint8_t>&& buf) {
	is_valid = false;
	span_u8 sp = buf;
	span_u8 header;
	if (!split_span(sp, 0x80, header))
		return;
	total_songs = header[6];
	starting_song = header[7];
	if (!total_songs || starting_song == 0 || starting_song > total_songs)
		return;
	load = header[8] | header[9]<<8;
	init = header[10] | header[11]<<8;
	play = header[12] | header[13]<<8;
	for (int i = 0; i < 8; i++) {
		if ((bank_init[i] = header[0x70 + i]))
			banking = true;
	}

	if (!banking) {
		if (load < 0x8000 || sp.size() - (load&0xFFF) > 0x8000)
			return;
		bank_count = 8; // for convenience in NsfMapper::set_bank
		rom.resize(0x8000);
	} else {
		size_t len = (load&0xFFF) + sp.size();
		if (len > 0x1000*256 || len == 0)
			return;
		// round up to 4K
		len += 0xFFF;
		len &= ~0xFFF;
		bank_count = len >> 12;
		rom.resize(len);
	}
	// funnily enough, this part is the same regardless of banking.
	memcpy(rom.data() + (load&0xFFF), sp.data(), sp.size());
	is_valid = true;
}
bool load_nsf(std::vector<uint8_t> &buf) {
	mapper_setup(*new NsfFile(std::move(buf)));
	return true;
}
}
