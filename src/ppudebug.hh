#ifndef VHVC_PPUDEBUG
#define VHVC_PPUDEBUG
#include "common.hh"
namespace vhvc::ppudebug {
	extern bool show_pt_window;
	extern bool show_nt_window;
	extern bool show_pal_window;
	extern bool show_ppu_output;
	extern bool show_ppu_state;
	extern bool break_on_scanline;
	extern bool break_on_vblank;
	extern SDL_Texture* ppu_texture;
	void draw_ppu_texture();
	void on_reg_read(int reg, uint8_t data);
	void on_reg_write(int reg, uint8_t data);
	bool init(SDL_Renderer* renderer);
	void gui();
}
#endif
