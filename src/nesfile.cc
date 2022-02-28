#include "nesfile.hh"
#include <utility>
#include "mapper.hh"
#include "bus.hh"
namespace vhvc {
static constexpr size_t calc_size(size_t num, size_t banksize) {
	if (num < 0xF00) {
		return num * banksize;
	} else {
		return ((num & 3) * 2 + 1) << (num >> 2 & 0x3F);
	}
}
static constexpr size_t prgrom_size(uint8_t hdr[16]) {
	return calc_size(hdr[9] << 8 & 0xF00 | hdr[4], 16384);
}
static constexpr size_t chrrom_size(uint8_t hdr[16]) {
	return calc_size(hdr[9] << 4 & 0xF00 | hdr[5], 8192);
}
static bool fix_ines(uint8_t hdr[16], size_t sz) {
	if ((hdr[7] & 0x0C) == 0x08) { // NES 2.0
		if (sz < 16 + prgrom_size(hdr) + chrrom_size(hdr)) {
			memset(hdr + 7, 0, 9); // 6-byte iNES
		}
	} else if (hdr[7] & 0x0C || hdr[12] || hdr[13] || hdr[14] || hdr[15]) { // 6-byte iNES
		memset(hdr + 7, 0, 9);
	} else { // 7-byte iNES
		memset(hdr + 8, 0, 8);
	}
	if ((hdr[7] & 0x0C) != 0x08) { // iNES
		// This PRGRAM stuff makes me want to reconsider supporting iNES format
		hdr[10] = hdr[6] & 2 ? 0x70 : 0x07; // 8K of PRGRAM or PRGNVRAM, depending on the battery bit
		hdr[11] = hdr[5] ? 0 : 0x07; // 8K of CHR-RAM if no CHR-ROM
		return true;
	}
	return false;
}
static bool split_span(span_u8& rest, size_t len, span_u8& out) {
	if (rest.size() < len)
		return false;
	out = rest.first(len);
	rest = rest.subspan(len);
	return true;
}
NesFile::NesFile(std::vector<uint8_t>&& a_buf) :buf(a_buf) {
	is_valid = false;

	span_u8 sp = buf;
	if (!split_span(sp, 16, header))
		return;
	is_ines = fix_ines(header.data(), buf.size());

	vertical = header[6] & 1;
	battery = header[6] & 2;
	if (header[6] & 4) {
		if (!split_span(sp, 512, trainer))
			return;
	} else {
		trainer = span_u8();
	}
	four_screen = header[6] & 8;
	mapper = header[6] & 0xF0 | header[7]<<4 & 0xF00 | header[8]<<8 & 0xF000 | header[8] & 0xF;

	prgram_size = 64 << (header[10] & 0xF);
	prgnvram_size = 64 << (header[10]>>4 & 0xF);
	chrram_size = 64 << (header[11] & 0xF);
	chrnvram_size = 64 << (header[11] >> 4 & 0xF);

	if (is_ines && mapper>>4 == 13) { // CPROM
		chrram_size = 16384;
	}

	if (!split_span(sp, prgrom_size(header.data()), prgrom))
		return;
	if (!split_span(sp, chrrom_size(header.data()), chrrom))
		return;

	extra.resize(chrram_size);
	chrram = span_u8(extra.data(), extra.data() + chrram_size);

	if (chrrom.size() == 0) {
		chr = chrram;
	} else {
		chr = chrrom;
	}
	is_valid = true;
}
bool load_rom(std::vector<uint8_t> &buf) {
	mapper_setup(*new NesFile(std::move(buf)));
	return true;
}
bool apply_patch(std::vector<uint8_t>& buf, cspan_u8 pbuf) {
	const uint8_t* p = pbuf.data();
	const uint8_t* end = p + pbuf.size();
	if (p + 5 >= end || memcmp(p, "PATCH", 5))
		return false;
	p += 5;
	for (;;) {
		if (p + 3 >= end)
			return false;
		size_t offset = p[0]<<16 | p[1]<<8 | p[2];
		p += 3;
		if (offset == 0x454F46)
			break;
		if (p + 2 >= end)
			return false;
		uint16_t len = p[0]<<8 | p[1];
		p += 2;
		if (len == 0) {
			if (p + 3 >= end)
				return false;
			len = p[0]<<8 + p[1];
			if (offset + len > buf.size())
				buf.resize(offset + len);
			memset(buf.data() + offset, p[3], len);
			p += 3;
		} else {
			if (p + len >= end)
				return false;
			if (offset + len > buf.size())
				buf.resize(offset + len);
			memcpy(buf.data()+offset, &*p, len);
			p += len;
		}
	}
	if (p + 3 >= end) {
		size_t size = p[0] << 16 | p[1] << 8 | p[2];
		buf.resize(size);
	}
	return true;
}
bool apply_patch(std::vector<uint8_t>& buf, File& patch) {
	if (!patch)
		return false;
	std::vector<uint8_t> pbuf;
	if (!patch.readInto(pbuf))
		return false;
	return apply_patch(buf, span_u8(pbuf));
}
}
