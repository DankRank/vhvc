#include "input.hh"
#include "cpu.hh"
#include "cpudebug.hh"
#include "ppudebug.hh"
#include "bus.hh"
#include "nesfile.hh"
#include "mapper.hh"
#include "palette.hh"
#include "audio.hh"
#include "state.hh"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

namespace vhvc {

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool is_running = true;
bool show_demo_window = false;
bool show_input_debug = false;
bool run_cpu = false;
bool show_file_input = false;
bool show_mapper_debug = false;
char filename[512];
void events_basic() {
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		handle_input(&ev);
		if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_Q)
			is_running = false;
		if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_G) {
			LogState st;
			cpu::state(st);
		}
		if (ev.type == SDL_QUIT) {
			is_running = false;
			break;
		}
	}
	ppudebug::draw_ppu_texture();
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, ppudebug::ppu_texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	cpu::step(29580);
	audio::flip();
}
void events() {
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		ImGui_ImplSDL2_ProcessEvent(&ev);
		handle_input(&ev);
		if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_PAUSE)
			run_cpu = !run_cpu;
		if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_R && ev.key.keysym.mod & KMOD_CTRL)
			bus_reset();
		if (ev.type == SDL_DROPFILE) {
			File f = File::fromFile(ev.drop.file, "rb");
			if (f) {
				std::vector<uint8_t> buf;
				if (f.readInto(buf)) {
					load_rom(buf);
					bus_poweron();
				}
			}
			SDL_free(ev.drop.file);
		}
		if (ev.type == SDL_QUIT) {
			is_running = false;
			break;
		}
	}
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);
	if (show_input_debug)
		input_debug(&show_input_debug);
	cpudebug::gui();
	ppudebug::gui();

	if (show_file_input) {
		if (ImGui::Begin("Open", &show_file_input)) {
			ImGui::InputText("Filename", filename, 512);
			if (ImGui::Button("Load")) {
				show_file_input = false;
				File f = File::fromFile(filename, "rb");
				if (f) {
					std::vector<uint8_t> buf;
					if (f.readInto(buf)) {
						load_rom(buf);
						bus_poweron();
					}
				}
			}
		}
		ImGui::End();
	}

	if (show_mapper_debug) {
		if (ImGui::Begin("Mapper Debug", &show_mapper_debug))
			mapper->debug_gui();
		ImGui::End();
	}

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::MenuItem("Open", nullptr, &show_file_input);
			if (ImGui::MenuItem("Reset"))
				bus_reset();
			if (ImGui::MenuItem("Exit"))
				is_running = false;
			if (ImGui::MenuItem("beep")) {
				int16_t s;
				for (int i = 0; i < 1000; i++) {
					s = 1000;
					audio::enqueue(s);
					audio::enqueue(s);
					audio::enqueue(s);
					audio::enqueue(s);
					audio::enqueue(s);
					audio::enqueue(s);
					s = -1000;
					audio::enqueue(s);
					audio::enqueue(s);
					audio::enqueue(s);
					audio::enqueue(s);
					audio::enqueue(s);
					audio::enqueue(s);
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("CPU")) {
			ImGui::MenuItem("Run CPU", nullptr, &run_cpu);
			ImGui::MenuItem("CPU State", nullptr, &cpudebug::show_cpu_state);
			ImGui::MenuItem("CPU Memory", nullptr, &cpudebug::show_cpu_memory);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("PPU")) {
			ImGui::MenuItem("PPU State", nullptr, &ppudebug::show_ppu_state);
			ImGui::MenuItem("Pattern Tables", nullptr, &ppudebug::show_pt_window);
			ImGui::MenuItem("Nametables", nullptr, &ppudebug::show_nt_window);
			ImGui::MenuItem("Palette", nullptr, &ppudebug::show_pal_window);
			ImGui::MenuItem("PPU Output", nullptr, &ppudebug::show_ppu_output);
			ImGui::MenuItem("Break on Scanline", nullptr, &ppudebug::break_on_scanline);
			ImGui::MenuItem("Break on VBlank", nullptr, &ppudebug::break_on_vblank);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools")) {
			ImGui::MenuItem("Joy debug", nullptr, &show_input_debug);
			ImGui::MenuItem("Mapper Debug", nullptr, &show_mapper_debug);
			ImGui::MenuItem("ImGui demo", nullptr, &show_demo_window);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::Render();
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
	SDL_RenderClear(renderer);
	ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
	SDL_RenderPresent(renderer);

	// FIXME: curerently relying on vsync for timing
	//static uint64_t start = SDL_GetTicks();
	//uint64_t now = SDL_GetTicks();
	//if (now - start > 16) {
	if (run_cpu)
		cpu::step(29580);
		//while (now - start > 16)
		//	start += 16;
	//}
	audio::flip();
}
int main(int argc, char** argv)
{
	palette::set_default_colors();

	const char *load_on_start = "donkey_kong.nes";
	bool basic_mode = false;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--nestest")) {
			File f = File::fromFile("nestest.nes", "rb");
			if (f) {
				std::vector<uint8_t> buf;
				if (f.readInto(buf)) {
					static const uint8_t patch[] = "PATCH\x00\x40\x0C\x00\x02\x00\xC0""EOF";
					apply_patch(buf, cspan_u8(patch));
					load_rom(buf);
					cpudebug::is_debugging = true;
					cpudebug::nestest = true;
					bus_poweron();
					cpu::step(8992);
				}
			}
			return 0;
		}
		if (!strcmp(argv[i], "--basic"))
			basic_mode = true;
		if (argv[i][0] != '-')
			load_on_start = argv[i];
	}
	{
		File f = File::fromFile(load_on_start, "rb");
		if (f) {
			std::vector<uint8_t> buf;
			if (f.readInto(buf))
				load_rom(buf);
		}
		bus_poweron();
	}

	SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE, "controller_map.txt");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER)) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}
	int w = basic_mode ? 512 : 1280;
	int h = basic_mode ? 480 : 960;
	window = SDL_CreateWindow("vhvc", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!window) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		return 1;
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		return 1;
	}

	if (!audio::init())
		return 1;

	if (!ppudebug::init(renderer)) {
		return 1;
	}

	if (!basic_mode) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui_ImplSDL2_InitForSDLRenderer(window);
		ImGui_ImplSDLRenderer_Init(renderer);
	}

	while (is_running) {
		basic_mode ? events_basic() : events();
	}

	if (!basic_mode) {
		ImGui_ImplSDLRenderer_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
}

int main(int argc, char** argv) {
	return vhvc::main(argc, argv);
}
