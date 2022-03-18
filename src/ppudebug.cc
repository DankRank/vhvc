#include "ppudebug.hh"
#include <stdio.h>
#include "imgui.h"
#include "bus.hh"
#include "ppu.hh"
#include "palette.hh"
namespace vhvc::ppudebug {
static inline uint32_t* add_lines(void *p, int pitch, int y) {
	return (uint32_t*)((uint8_t*)p + pitch*y);
}
void draw_pt(SDL_Texture *tex) {
	inspect_lock lk;
	void* pixels;
	int pitch;
	SDL_LockTexture(tex, NULL, &pixels, &pitch);
	for (int t = 0; t < 512; t++) {
		uint32_t* pp = add_lines(pixels, pitch, t/16*8) + t%16*8;
		for (int y = 0; y < 8; y++) {
			uint8_t bits0 = ppu_read(t*16 + y);
			uint8_t bits1 = ppu_read(t*16 + y + 8);
			for (int x = 0; x < 8; x++)
				*pp++ = palette::colors[(bits0>>(7-x) & 1 | bits1>>(7-x)<<1 & 1)<<4];
			pp = add_lines(pp, pitch, 1) - 8;
		}
	}
	SDL_UnlockTexture(tex);
}
// TODO: remove this buggy mess
void draw_obj_over_nt(void *pixels, int pitch) {
	for (int i = 0; i < 256; i+=4) {
		int y = ppu::oam[i];
		int tile = ppu::oam[i+1];
		int attr = ppu::oam[i+2] & 3;
		bool flipx = ppu::oam[i+2] & 0x40;
		bool flipy = ppu::oam[i+2] & 0x80;
		int x = ppu::oam[i+3]+1;
		uint32_t* pp = add_lines(pixels, pitch, y) + x;
		// NOTE: we don't care about bounds checking here, because we're drawing on top of the NT0
		for (int y = 0; y < (ppu::obj_size ? 16 : 8); y++) {
			int ny;
			int ntile;
			if (ppu::obj_size) {
				ntile = tile&0xFE | (tile<<8 & 0x100);
				ny = (y<<1 & 0x10) | (y & 0x7);
				if (flipy)
					ny ^= 0x17;
			} else {
				ntile = tile + (ppu::obj_pattern ? 0x100 : 0);
				ny = y;
				if (flipy)
					ny ^= 0x07;
			}
			uint8_t bits0 = ppu_read(ntile*16 + ny);
			uint8_t bits1 = ppu_read(ntile*16 + ny + 8);
			for (int x = 0; x < 8; x++) {
				int nx = flipx ? x : 7-x;
				if ((bits0 | bits1)>>nx & 1)
					*pp++ = ppu::rgb_palette[0x10 | attr<<2 | bits0>>nx&1 | bits1>>nx<<1 & 2];
				else
					pp++;
			}
			pp = add_lines(pp, pitch, 1) - 8;
		}
	}
}
void draw_nt(SDL_Texture* tex) {
	inspect_lock lk;
	void* pixels;
	int pitch;
	SDL_LockTexture(tex, NULL, &pixels, &pitch);
	uint16_t pattern_table = ppu::bg_pattern ? 0x1000 : 0x0000;
	for (int nt = 0; nt < 4; nt++) {
		uint16_t addr = 0x2000 + (nt<<10);
		uint32_t* p = add_lines(pixels, pitch, nt&2 ? 240 : 0) + (nt&1 ? 256 : 0);
		for (int ty = 0; ty < 30; ty++) {
			for (int tx = 0; tx < 32; tx++) {
				uint32_t *pp = add_lines(p, pitch, ty*8) + tx*8;
				uint8_t tile = ppu_read(addr + ty*32 + tx);
				uint8_t attr = ppu_read(addr + 0x3C0 + ty/4*8 + tx/4);
				if (tx & 2) attr >>= 2;
				if (ty & 2) attr >>= 4;
				attr &= 3;
				for (int y = 0; y < 8; y++) {
					uint8_t bits0 = ppu_read(pattern_table + tile*16 + y);
					uint8_t bits1 = ppu_read(pattern_table + tile*16 + y + 8);
					for (int x = 0; x < 8; x++) {
						if ((bits0 | bits1)>>(7-x) & 1)
							*pp++ = ppu::rgb_palette[attr<<2 | bits0>>(7-x) & 1 | bits1>>(7-x)<<1 & 2];
						else
							*pp++ = ppu::rgb_palette[0];
					}
					pp = add_lines(pp, pitch, 1) - 8;
				}
			}
		}
	}
	draw_obj_over_nt(pixels, pitch);
	SDL_UnlockTexture(tex);
}
void draw_palette(SDL_Texture* tex) {
	void* pixels;
	int pitch;
	SDL_LockTexture(tex, NULL, &pixels, &pitch);
	for (int y = 0; y < 2; y++) {
		uint32_t* p = add_lines(pixels, pitch, y);
		for (int x = 0; x < 16; x++) {
			*p++ = ppu::rgb_palette[y*16 + x];
		}
	}
	SDL_UnlockTexture(tex);
}
void on_reg_read(int reg, uint8_t data) {
	(void)reg;
	(void)data;
	//fprintf(stderr, "PPU READ  %d %02X\n", reg, data);
}
void on_reg_write(int reg, uint8_t data) {
	(void)reg;
	(void)data;
	//if (reg == 5)
	//	fprintf(stderr, "PPU WRITE  %d %02X\n", reg, data);
}
SDL_Texture* pt_texture = NULL;
SDL_Texture* nt_texture = NULL;
SDL_Texture* pal_texture = NULL;
SDL_Texture* ppu_texture = NULL;
bool show_pt_window = false;
bool show_nt_window = true;
bool show_pal_window = false;
bool show_ppu_output = true;
bool show_ppu_state = false;
bool sync_to_vblank = false;
bool break_on_scanline = false;
bool break_on_vblank = false;
bool init(SDL_Renderer *renderer) {
	pt_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 128, 256);
	if (!pt_texture) {
		fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
		return false;
	}
	nt_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 512, 480);
	if (!nt_texture) {
		fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
		return false;
	}
	pal_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 16, 2);
	if (!pal_texture) {
		fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
		return false;
	}
	ppu_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
	if (!ppu_texture) {
		fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
		return false;
	}
	return true;
}
void draw_ppu_texture() {
	void* pixels;
	int pitch;
	SDL_LockTexture(ppu_texture, NULL, &pixels, &pitch);
	for (int y = 0; y < 240; y++) {
		uint32_t* pp = add_lines(pixels, pitch, y);
		memcpy(pp, &ppu::framebuffer[256*y], 256*sizeof(uint32_t));
	}
	SDL_UnlockTexture(ppu_texture);
}
void gui() {
	if (show_pt_window) {
		if (ImGui::Begin("Pattern Tables", &show_pt_window)) {
			draw_pt(pt_texture);
			ImGui::Image(pt_texture, ImVec2{ 128 * 2, 256 * 2 });
		}
		ImGui::End();
	}
	if (show_nt_window) {
		if (ImGui::Begin("Nametables", &show_nt_window)) {
			draw_nt(nt_texture);
			ImGui::Image(nt_texture, ImVec2{ 512 * 2, 480 * 2 });
		}
		ImGui::End();
	}
	if (show_pal_window) {
		if (ImGui::Begin("Palette", &show_pal_window)) {
			for (int i = 0; i < 32; i += 16) {
				using ppu::palette;
				ImGui::Text("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
					palette[i], palette[i + 1], palette[i + 2], palette[i + 3],
					palette[i + 4], palette[i + 5], palette[i + 6], palette[i + 7],
					palette[i + 8], palette[i + 9], palette[i + 10], palette[i + 11],
					palette[i + 12], palette[i + 13], palette[i + 14], palette[i + 15]);
			}
			draw_palette(pal_texture);
			ImGui::Image(pal_texture, ImVec2{ 16 * 32, 2 * 32 });
		}
		ImGui::End();
	}
	if (show_ppu_output) {
		draw_ppu_texture();
		if (ImGui::Begin("PPU Output", &show_ppu_output)) {
			ImGui::Image(ppu_texture, ImVec2{ 256 * 2, 240 * 2 });
		}
		ImGui::End();
	}
	if (show_ppu_state) {
		if (ImGui::Begin("PPU State", &show_ppu_state)) {
			ImGui::Text("Cycle num: %3d,%3d", ppu::line, ppu::dot);
			ImGui::Text("scroll_t: %04X NT%d x:%d.%d y:%d.%d", ppu::t, ppu::t>>10 & 3, ppu::t & 31, ppu::x_fine, ppu::t>>5 & 31, ppu::t>>12 & 7);
			ImGui::Text("scroll_v: %04X NT%d x:%d.%d y:%d.%d", ppu::v, ppu::v>>10 & 3, ppu::v & 31, ppu::x_fine, ppu::v>>5 & 31, ppu::v>>12 & 7);
			ImGui::Text("Object pattern: %s | Background pattern: %s",
				ppu::obj_size ? "8x16" : ppu::obj_pattern ? "$1000" : "$0000",
				ppu::bg_pattern ? "$1000" : "$0000");
			ImGui::Text("Object: %s | Background: %s",
				ppu::obj_enable ? (ppu::obj_left ? "Enabled" : "Clipped") : "Disabled",
				ppu::bg_enable ? (ppu::bg_left ? "Enabled" : "Clipped") : "Disabled");
			ImGui::Text("Flags:%s%s%s%s%s%s",
				ppu::vertical_increment ? " +vinc" : "",
				ppu::nmi_enabled ? " +nmi" : "",
				ppu::grayscale ? " +grayscale" : "",
				ppu::emphasis&1 ? " +red" : "",
				ppu::emphasis&2 ? " +green" : "",
				ppu::emphasis&4 ? " +blue" : "");
			ImGui::Text("Status:%s%s%s",
				ppu::in_vblank ? " +vblank" : "",
				ppu::obj0_hit ? " +obj0" : "",
				ppu::obj_overflow ? " +overflow" : "");
		}
		ImGui::End();
	}
}
}
