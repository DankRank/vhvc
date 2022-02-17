#ifndef VHVC_PPU
#define VHVC_PPU
#include <stdint.h>
namespace vhvc::ppu {
	extern uint8_t iobus ;
	extern bool vertical_increment;
	extern bool obj_pattern;
	extern bool bg_pattern;
	extern bool obj_size;
	extern bool nmi_enabled;
	extern bool grayscale;
	extern bool bg_left;
	extern bool obj_left;
	extern bool bg_enable;
	extern bool obj_enable;
	extern int emphasis;
	extern bool in_vblank;
	extern bool obj0_hit;
	extern bool obj_overflow;
	extern uint8_t oamaddr;
	extern uint8_t oam[256];
	extern uint8_t oam_copy[32];
	extern uint8_t oam_buf;
	extern int oam_state;
	extern int oam_copy_ptr;
	extern uint16_t v;
	extern uint16_t t;
	extern uint16_t x_fine;
	extern bool write_latch;
	extern uint8_t read_buffer;
	extern uint32_t rgb_palette[32];
	extern uint8_t palette[32];
	extern int dot;
	extern int line;
	extern bool odd_frame;
	extern uint32_t framebuffer[256*240];
	extern bool obj0_next_line;
	extern uint8_t tile_buf;
	extern uint8_t attr_buf;
	extern uint8_t bits0_buf;
	extern uint8_t bits1_buf;
	extern uint8_t obj_bits0[8];
	extern uint8_t obj_bits1[8];
	extern uint64_t bg_shiftreg;
	extern uint8_t obj_out[256];
	enum {
		PPUCTRL = 0,
		PPUMASK,
		PPUSTATUS,
		OAMADDR,
		OAMDATA,
		PPUSCROLL,
		PPUADDR,
		PPUDATA,
	};
	void do_cycle();
	uint8_t reg_read(int reg);
	void reg_write(int reg, uint8_t data);
}

#endif
