#include "ppu.hh"
#include "palette.hh"
#include "ppudebug.hh"
#include "bus.hh"
namespace vhvc {
	extern bool run_cpu; // FIXME: move this to some header
}
namespace vhvc::ppu {
uint8_t iobus = 0;
bool vertical_increment = false;
bool obj_pattern = false;
bool bg_pattern = false;
bool obj_size = false;
bool nmi_enabled = false;
bool grayscale = false;
bool bg_left = false;
bool obj_left = false;
bool bg_enable = false;
bool obj_enable = false;
int emphasis = 0;
bool in_vblank = false;
bool obj0_hit = false;
bool obj_overflow = false;
uint8_t oamaddr = 0;
uint8_t oam[256] = {0};
uint8_t oam_copy[32] = {0};
int obj_count = 0;
uint8_t oam_buf = 0;
int oam_state = 0;
int oam_copy_ptr = 0;
uint16_t v = 0;
uint16_t t = 0;
uint16_t x_fine = 0;
bool write_latch = false;
uint8_t read_buffer = 0;
uint32_t rgb_palette[32] = {0};
uint8_t palette[32] = {0};
int dot = 0;
int line = 0;
bool odd_frame = false;
uint32_t framebuffer[256*240] = {0};
bool obj0_next_line = false;
uint8_t tile_buf = 0;
uint8_t attr_buf = 0;
uint8_t bits0_buf = 0;
uint8_t bits1_buf = 0;
uint8_t obj_bits0[8] = { 0 };
uint8_t obj_bits1[8] = { 0 };
uint64_t bg_shiftreg = 0;
// pz--ccccc
// ||  +++++- color index
// |+-------- sprite zero
// +--------- priority
uint8_t obj_out[256] = { 0 };
uint32_t palette_to_rgb(uint8_t data) {
	if (grayscale)
		data &= 0x30;
	return palette::colors[emphasis<<6 | data];
}
void palette_write(int entry, uint8_t data) {
	data &= 0x3F;
	if (entry & 3) {
		palette[entry] = data;
		rgb_palette[entry] = palette_to_rgb(data);
	} else {
		palette[entry & 0x0F] = palette[entry | 0x10] = data;
		rgb_palette[entry & 0x0F] = rgb_palette[entry | 0x10] = palette_to_rgb(data);
	}
}
void poweron() {
	reset();
	oamaddr = 0;
	v = 0;
	for (int i = 0; i < 32; i++)
		palette_write(i, 0);
	memset(oam, 0xFF, 256);
}
void reset() {
	vertical_increment = obj_pattern = bg_pattern = obj_size = nmi_enabled = false;
	grayscale = bg_left = obj_left = bg_enable = obj_enable = false;
	emphasis = 0;
	t = x_fine = 0;
	write_latch = false;
	odd_frame = false;
	line = dot = 0;
	read_buffer = 0;
	// TODO: implement warm-up?
}

static void do_oam_eval_write_cycle() {
	// FIXME: this is terrible
	switch (oam_state) {
	case 0:
		oam_copy[oam_copy_ptr] = oam_buf;
		if (line >= oam_buf && line < oam_buf + (obj_size ? 16 : 8)) {
			if (!oamaddr)
				obj0_next_line = true;
			oamaddr++;
			oam_state++;
			oam_copy_ptr++;
			obj_count++;
		} else {
			oamaddr += 4;
		}
		break;
	case 1: case 2: case 3:
		oam_copy[oam_copy_ptr++] = oam_buf;
		oamaddr++;
		if (oam_state == 3) {
			if (oamaddr == 0)
				oam_state = 8;
			else if (oam_copy_ptr == 32)
				oam_state = 4;
			else
				oam_state = 0;
		} else {
			oam_state++;
		}
		break;
	case 4:
		if (line >= oam_buf && line < oam_buf + (obj_size ? 16 : 8)) {
			obj_overflow = true;
			oamaddr++;
			oam_state++;
		} else {
			oamaddr += (oamaddr&0x3) == 3 ? 1 : 5;
			// FIXME: do I check for overflow here?
		}
		break;
	case 5: case 6: case 7:
		oamaddr++;
		oam_state++;
		if (oam_state == 7) {
			if ((oamaddr&0x3) == 3) // FIXME: this is really sus, i might've interpreted the wiki page wrong
				oamaddr += 4;
			if ((oamaddr&0xFC) == 0) {
				oamaddr = 0; // FIXME: not sure about this one either
				oam_state = 8;
			} else {
				oam_state = 4;
			}
		}
		break;
	case 8:
		oamaddr += 4;
		break;
	}
}
/*
for i in range(256):
	j = i<<7&128 | i<<5&64 | i<<3&32 | i<<1&16 | i>>1&8 | i>>3&4 | i>>5&2 | i>>7&1
	print(f'0x{j:02X}, ', end='\n' if i%16 == 15 else '')
*/
static const uint8_t reverse[256] = {
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF,
};
static void render_obj_line() {
	memset(obj_out, 0x00, 256);
	for (int i = 7; i >= 0; i--) {
		uint8_t bits0 = obj_bits0[i];
		uint8_t bits1 = obj_bits1[i];
		if (oam_copy[i*4 + 2]&0x40) {
			bits0 = reverse[bits0];
			bits1 = reverse[bits1];
		}
		// This puts the palette number in the right position and the priority in bit 7
		uint8_t attr = oam_copy[i*4 + 2]<<2 & 0x8C | 0x10;
		if (obj0_next_line && i == 0)
			attr |= 0x40;
		uint8_t opaque = bits0 | bits1;
		int x = oam_copy[i*4 + 3];
		for (int i = 0; i < 8 && x < 256; i++, x++) {
			if (opaque & 1<<(7-i)) {
				obj_out[x] = attr | bits1>>(7-i)<<1 & 2 | bits0>>(7-i) & 1;
			}
		}
	}
	obj0_next_line = false;
#if 0
	if (line < 239)
		for (int i = 0; i < 256; i++)
			framebuffer[(line+1)*256 + i] = rgb_palette[obj_out[i]&0x1F];
#endif
};
/*
for i in range(256):
	j = i&1 | i<<3&16 | i<<6&256 | i<<9&4096 | i<<12&65536 | i<<15&1048576 | i<<18&16777216 | i<<21&268435456
	print(f'0x{j:08X}, ', end='\n' if i%8 == 7 else '')
*/
static const uint32_t mux_to_dword[256] = {
	0x00000000, 0x00000001, 0x00000010, 0x00000011, 0x00000100, 0x00000101, 0x00000110, 0x00000111,
	0x00001000, 0x00001001, 0x00001010, 0x00001011, 0x00001100, 0x00001101, 0x00001110, 0x00001111,
	0x00010000, 0x00010001, 0x00010010, 0x00010011, 0x00010100, 0x00010101, 0x00010110, 0x00010111,
	0x00011000, 0x00011001, 0x00011010, 0x00011011, 0x00011100, 0x00011101, 0x00011110, 0x00011111,
	0x00100000, 0x00100001, 0x00100010, 0x00100011, 0x00100100, 0x00100101, 0x00100110, 0x00100111,
	0x00101000, 0x00101001, 0x00101010, 0x00101011, 0x00101100, 0x00101101, 0x00101110, 0x00101111,
	0x00110000, 0x00110001, 0x00110010, 0x00110011, 0x00110100, 0x00110101, 0x00110110, 0x00110111,
	0x00111000, 0x00111001, 0x00111010, 0x00111011, 0x00111100, 0x00111101, 0x00111110, 0x00111111,
	0x01000000, 0x01000001, 0x01000010, 0x01000011, 0x01000100, 0x01000101, 0x01000110, 0x01000111,
	0x01001000, 0x01001001, 0x01001010, 0x01001011, 0x01001100, 0x01001101, 0x01001110, 0x01001111,
	0x01010000, 0x01010001, 0x01010010, 0x01010011, 0x01010100, 0x01010101, 0x01010110, 0x01010111,
	0x01011000, 0x01011001, 0x01011010, 0x01011011, 0x01011100, 0x01011101, 0x01011110, 0x01011111,
	0x01100000, 0x01100001, 0x01100010, 0x01100011, 0x01100100, 0x01100101, 0x01100110, 0x01100111,
	0x01101000, 0x01101001, 0x01101010, 0x01101011, 0x01101100, 0x01101101, 0x01101110, 0x01101111,
	0x01110000, 0x01110001, 0x01110010, 0x01110011, 0x01110100, 0x01110101, 0x01110110, 0x01110111,
	0x01111000, 0x01111001, 0x01111010, 0x01111011, 0x01111100, 0x01111101, 0x01111110, 0x01111111,
	0x10000000, 0x10000001, 0x10000010, 0x10000011, 0x10000100, 0x10000101, 0x10000110, 0x10000111,
	0x10001000, 0x10001001, 0x10001010, 0x10001011, 0x10001100, 0x10001101, 0x10001110, 0x10001111,
	0x10010000, 0x10010001, 0x10010010, 0x10010011, 0x10010100, 0x10010101, 0x10010110, 0x10010111,
	0x10011000, 0x10011001, 0x10011010, 0x10011011, 0x10011100, 0x10011101, 0x10011110, 0x10011111,
	0x10100000, 0x10100001, 0x10100010, 0x10100011, 0x10100100, 0x10100101, 0x10100110, 0x10100111,
	0x10101000, 0x10101001, 0x10101010, 0x10101011, 0x10101100, 0x10101101, 0x10101110, 0x10101111,
	0x10110000, 0x10110001, 0x10110010, 0x10110011, 0x10110100, 0x10110101, 0x10110110, 0x10110111,
	0x10111000, 0x10111001, 0x10111010, 0x10111011, 0x10111100, 0x10111101, 0x10111110, 0x10111111,
	0x11000000, 0x11000001, 0x11000010, 0x11000011, 0x11000100, 0x11000101, 0x11000110, 0x11000111,
	0x11001000, 0x11001001, 0x11001010, 0x11001011, 0x11001100, 0x11001101, 0x11001110, 0x11001111,
	0x11010000, 0x11010001, 0x11010010, 0x11010011, 0x11010100, 0x11010101, 0x11010110, 0x11010111,
	0x11011000, 0x11011001, 0x11011010, 0x11011011, 0x11011100, 0x11011101, 0x11011110, 0x11011111,
	0x11100000, 0x11100001, 0x11100010, 0x11100011, 0x11100100, 0x11100101, 0x11100110, 0x11100111,
	0x11101000, 0x11101001, 0x11101010, 0x11101011, 0x11101100, 0x11101101, 0x11101110, 0x11101111,
	0x11110000, 0x11110001, 0x11110010, 0x11110011, 0x11110100, 0x11110101, 0x11110110, 0x11110111,
	0x11111000, 0x11111001, 0x11111010, 0x11111011, 0x11111100, 0x11111101, 0x11111110, 0x11111111,
};
static void do_vram_reads() {
	if ((line < 240 || line == 261) && dot > 0) {
		int which_byte = 8;
		switch (dot & 7) {
		case 2: // read nametable
		read_nt:
			tile_buf = ppu_read(0x2000 | (v&0x0FFF));
			break;
		case 4: // read attribute table
			if (dot == 340)
				goto read_nt;
			attr_buf = ppu_read(0x23C0 | (v&0x0C00) | ((v>>4)&0x38) | ((v>>2)&0x07));
			if (v & 0x0002) attr_buf >>= 2;
			if (v & 0x0040) attr_buf >>= 4;
			attr_buf &= 3;
			break;
		case 6: // read low byte
			which_byte = 0;
			[[fallthrough]];
		case 0: // read high byte
			if (dot >= 257 && dot <= 320) { // get obj tile
				int obj_num = (dot-257)>>3;
				int tile = oam_copy[obj_num*4 + 1];
				unsigned y = line - oam_copy[obj_num*4];
				// FIXME: non existent sprites might not be in range. what happens in that case?
				if (oam_copy[obj_num*4 + 2]&0x80) // vertical flip
					y ^= 15;
				if (obj_size) {
					if (tile & 1)
						tile += 256 - 1;
					if (y & 8)
						tile += 1;
				} else if (obj_pattern) {
					tile += 256;
				}
				y &= 7;
				uint8_t bits = ppu_read(tile*16 + y + which_byte);
				if (obj_num >= obj_count)
					bits = 0;
				(which_byte ? obj_bits1 : obj_bits0)[obj_num] = bits;
				if (dot == 320)
					render_obj_line();
			} else { // get bg tile
				int tile = tile_buf;
				int y = (v>>12)&7;
				if (bg_pattern)
					tile += 256;
				(which_byte ? bits1_buf : bits0_buf) = ppu_read(tile*16 + y + which_byte);
			}
			break;
		case 1:
			if (dot != 1) {
				uint32_t bits0 = mux_to_dword[bits0_buf];
				uint32_t bits1 = mux_to_dword[bits1_buf];
				uint32_t attr0 = attr_buf&1 ? 0x44444444 : 0;
				uint32_t attr1 = attr_buf&2 ? 0x88888888 : 0;
				bg_shiftreg = bg_shiftreg&0xFFFFFFFF00000000 | (bits0|bits1)*15 & (attr1 | attr0 | bits1<<1 | bits0);
			}
		}
	}
}
static void do_draw() {
	if (line < 240 || line == 261) {
		if (dot >= 1 && dot <= 256) {
			// FIXME: i'm not sure on the timing of checking the enable bits and the fine x
			uint8_t bg = bg_shiftreg>>(60-4*x_fine) & 15;
			uint8_t &obj = obj_out[dot-1];
			bool bg_on =  bg != 0  && bg_enable  && (dot <= 8 ? bg_left  : true);
			bool obj_on = obj != 0 && obj_enable && (dot <= 8 ? obj_left : true);
			if (bg_on && obj_on) {
				if (obj & 0x80)
					obj = obj&0x40 | bg;
			} else if (bg_on) {
				obj = bg;
			} else if (obj_on) {
				obj &= 0x1F;
			} else {
				obj = 0;
			}
			bg_shiftreg <<= 4;
		}
		if (dot >= 2 && dot <= 257) {
			if (obj_out[dot-2] & 0x40) {
				obj0_hit = true;
				obj_out[dot-2] &= 0x1F;
			}
		}
		if (line != 261 && dot >= 4 && dot <= 259) {
			framebuffer[line*256 + dot-4] = rgb_palette[obj_out[dot-4]];
		}
		if (dot >= 321 && dot <= 336)
			bg_shiftreg <<= 4;
	}
}
static void advance_scroll_regs() {
	if (line < 240 || line == 261) {
		if (dot >= 1 && dot <= 256 || dot >= 321) {
			if ((dot&7) == 0) { // increment horizontal scroll
				if ((v&0x001F) == 0x001F)
					v = v&0x7FE0 ^ 0x0400;
				else
					v++;
			}
		}
		if (dot == 256) { // increment vertical scroll
			if ((v&0x7000) != 0x7000)
				v += 0x1000;
			else if ((v&0x03E0) == 0x03A0)
				v = v&0x0C1F ^ 0x0800;
			else if ((v&0x03E0) == 0x03E0)
				v = v&0x0C1F;
			else
				v = (v&0x0FFF) + 0x20;
		}
		if (dot == 257) { // copy horizontal scroll
			v = v&0x7BE0 | t&0x041F;
		}
	}
	if (line == 261 && dot >= 280 && dot <= 304) { // copy vertical scroll
		v = t&0x7BE0 | v&0x041F;
	}
}
static void oam_eval() {
	if (dot < 65) { // secondary OAM clear
		obj_count = 0; // TODO: figure out exactly how this works
		if (dot && !(dot&1)) // dots 2 4 6 8 etc
			oam_copy[(dot-1) >> 1] = 0xFF;
	} else if (dot < 257) { // OAM evaluation
		if (dot == 65) {
			oamaddr = 0;
			oam_state = 0;
			oam_copy_ptr = 0;
		}
		if (dot & 1) {
			oam_buf = oam[oamaddr];
		} else {
			do_oam_eval_write_cycle();
		}
	}
}
static void update_flags() {
	if (line == 241 && dot == 1) {
		in_vblank = true;
		cpu::set_nmi(nmi_enabled);
		cpu::exit_requested = true;
		if (ppudebug::break_on_vblank)
			run_cpu = false;
	}
	if (line == 261 && dot == 1) {
		in_vblank = false;
		cpu::set_nmi(false);
		obj0_hit = false;
		obj_overflow = false;
	}
}

static void advance_dot_clock() {
	if (dot++ == 340) {
		dot = 0;
		if (line++ == 261) {
			line = 0;
			if ((odd_frame = !odd_frame))
				dot++;
		}
		if (ppudebug::break_on_scanline) {
			cpu::exit_requested = true;
			run_cpu = false;
		}
	}
}
void do_cycle() {
	bool is_rendering = bg_enable || obj_enable;
	if (is_rendering)
		do_vram_reads();
	do_draw();
	if (is_rendering) {
		if (line < 240)
			oam_eval();
		advance_scroll_regs();
	}
	update_flags();
	advance_dot_clock();
}
uint8_t reg_read(int reg) {
	switch (reg) {
	case PPUSTATUS:
		iobus = (int)in_vblank<<7 | (int)obj0_hit<<6 | (int)obj_overflow<<5 | iobus&0x1F;
		in_vblank = false;
		cpu::set_nmi(false);
		write_latch = false;
		// TODO: add the race condition
		break;
	case OAMDATA:
		// TODO: OAM readback
		if ((bg_enable || obj_enable) && line < 240 && dot > 0 && dot < 65) // return FF during secondary OAM clear
			iobus = 0xFF;
		break;
	case PPUDATA:
		uint16_t addr = v & 0x3FFF;
		if (addr < 0x3F00)
			iobus = read_buffer;
		else
			iobus = palette[addr & 0x1F];
		{
			inspect_lock lk; // FIXME: it should interact with rendering reads somehow
			read_buffer = ppu_read(addr);
		}
		v = (v + (vertical_increment ? 32 : 1))&0x7FFF;
		break;
	}
	ppudebug::on_reg_read(reg, iobus);
	return iobus;
}
void reg_write(int reg, uint8_t data) {
	ppudebug::on_reg_write(reg, data);
	iobus = data;
	switch (reg) {
	case PPUCTRL:
		t = t&0x73FF | data<<10 & 0x0C00;
		vertical_increment = data&4;
		obj_pattern = data&8;
		bg_pattern = data&16;
		obj_size = data&32;
		nmi_enabled = data&128;
		if (!nmi_enabled)
			cpu::set_nmi(false);
		else
			cpu::set_nmi(in_vblank);
		break;
	case PPUMASK:
		grayscale = data&1;
		bg_left = data&2;
		obj_left = data&4;
		bg_enable = data&8;
		obj_enable = data&16;
		emphasis = data>>5;
		for (int i = 0; i < 32; i++)
			rgb_palette[i] = palette_to_rgb(palette[i]);
		break;
	case OAMADDR:
		// FIXME: lots of quirks
		oamaddr = data;
		break;
	case OAMDATA:
		// FIXME: lots of quirks
		if ((oamaddr & 3) == 2)
			data &= 0xE3;
		oam[oamaddr++] = data;
		break;
	case PPUSCROLL:
		if (!write_latch) {
			t = t&0x7FE0 | data>>3;
			x_fine = data&7;
		} else {
			t = t&0x0C1F | data<<12 & 0x7000 | data<<2 & 0x03E0;
		}
		write_latch = !write_latch;
		break;
	case PPUADDR:
		if (!write_latch)
			t = t&0xFF | data<<8 & 0x3F00;
		else
			v = t = t&0x7F00 | data;
		write_latch = !write_latch;
		break;
	case PPUDATA:
		uint16_t addr = v & 0x3FFF;
		if (addr < 0x3F00) {
			inspect_lock lk; // FIXME: it should interact with rendering reads somehow
			ppu_write(addr, data);
		} else {
			palette_write(addr & 0x1F, data);
		}
		v = (v + (vertical_increment ? 32 : 1)) & 0x7FFF;
		break;
	}
}
}
