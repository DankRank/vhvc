#include "input.hh"
#include "cpu.hh"
#include "cpudebug.hh"
#include "ppu.hh"
#include "ppudebug.hh"
#include "bus.hh"
#include "nesfile.hh"
#include "nsffile.hh"
#include "fdsfile.hh"
#include "mapper.hh"
#include "palette.hh"
#include "audio.hh"
#include "apu.hh"
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
bool show_famikey_debug = false;
bool run_cpu = false;
bool show_file_input = false;
bool show_mapper_debug = false;
char filename[512];
int sync_type = 1;
enum {
	SYNC_TO_VSYNC,
	SYNC_TO_AUDIO,
	SYNC_TO_NOTHING,
};
void set_sync_type(int type) {
	sync_type = type;
	SDL_RenderSetVSync(renderer, type == SYNC_TO_VSYNC);
	ppudebug::sync_to_vblank = type != SYNC_TO_AUDIO;
	audio::sync_to_audio = type == SYNC_TO_AUDIO;
}
bool load_file(std::vector<uint8_t>& buf)
{
	if (buf.size() > 0x16 && !memcmp(buf.data(), "NES\032", 4))
		return load_nes(buf);
	if (buf.size() > 0x80 && !memcmp(buf.data(), "NESM\032\001", 6))
		return load_nsf(buf);
	if (buf.size() >= 65500) {
		if (!memcmp(buf.data(), "FDS\032", 4) || !memcmp(buf.data(), "\1*NINTENDO-HVC*", 15))
			return load_fds(buf);
	}
	return false;
}
Uint32 event_console = 0;
int console_thread(void*) {
	SDL_Event ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = event_console;
	for (;;) {
		switch (getchar()) {
			case '<': ev.user.code = 0x40; SDL_PushEvent(&ev); break;
			case '>': ev.user.code = 0x80; SDL_PushEvent(&ev); break;
			case EOF: ev.user.code = 0; ev.type = SDL_QUIT; SDL_PushEvent(&ev); return 0;
		}
	}
}
void events_headless() {
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		if (ev.type == event_console) {
			nsf_console_input |= ev.user.code;
		}
		if (ev.type == SDL_QUIT) {
			is_running = false;
			break;
		}
	}
	if (audio::need_frame())
		cpu::step(29580);
	audio::flip();
}
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
		if (ev.type == SDL_KEYDOWN &&
				(ev.key.keysym.scancode == SDL_SCANCODE_P ||
				ev.key.keysym.scancode == SDL_SCANCODE_PAUSE)) {
			run_cpu = !run_cpu;
			SDL_SetWindowTitle(window, run_cpu ? "vhvc" : "vhvc (paused)");
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
	if (run_cpu && audio::need_frame())
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
					load_file(buf);
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
	if (show_famikey_debug)
		famikey_debug(&show_famikey_debug);
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
						load_file(buf);
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
			if (ImGui::MenuItem("Sync to VSync", nullptr, sync_type == SYNC_TO_VSYNC))
				set_sync_type(SYNC_TO_VSYNC);
			if (ImGui::MenuItem("Sync to Audio", nullptr, sync_type == SYNC_TO_AUDIO))
				set_sync_type(SYNC_TO_AUDIO);
			if (ImGui::MenuItem("Don't Sync", nullptr, sync_type == SYNC_TO_NOTHING))
				set_sync_type(SYNC_TO_NOTHING);
			if (ImGui::MenuItem("Trace Instructions", nullptr, &cpudebug::nestest))
				cpudebug::is_debugging = cpudebug::nestest || cpudebug::log_intr;
			if (ImGui::MenuItem("Trace Interrupts", nullptr, &cpudebug::log_intr))
				cpudebug::is_debugging = cpudebug::nestest || cpudebug::log_intr;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("PPU")) {
			ImGui::MenuItem("PPU State", nullptr, &ppudebug::show_ppu_state);
			ImGui::MenuItem("Pattern Tables", nullptr, &ppudebug::show_pt_window);
			ImGui::MenuItem("Nametables", nullptr, &ppudebug::show_nt_window);
			ImGui::MenuItem("Palette", nullptr, &ppudebug::show_pal_window);
			ImGui::MenuItem("Events", nullptr, &ppudebug::show_events);
			ImGui::MenuItem("PPU Output", nullptr, &ppudebug::show_ppu_output);
			ImGui::MenuItem("Break on Scanline", nullptr, &ppudebug::break_on_scanline);
			ImGui::MenuItem("Break on VBlank", nullptr, &ppudebug::break_on_vblank);
			if (ImGui::MenuItem("Trigger SZH"))
				ppu::obj0_hit = true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools")) {
			ImGui::MenuItem("Joy debug", nullptr, &show_input_debug);
			ImGui::MenuItem("Family Keyboard Debug", nullptr, &show_famikey_debug);
			ImGui::MenuItem("Mapper Debug", nullptr, &show_mapper_debug);
			if (ImGui::MenuItem("Dump Audio", nullptr, !!apu::dump_file))
				apu::dump_file = apu::dump_file ? File() : File::fromFile("apudump.bin", "wb");
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

	if (run_cpu && audio::need_frame())
		cpu::step(29580);
	audio::flip();
}
int main(int argc, char** argv)
{
	palette::set_default_colors();

	const char *load_on_start = "donkey_kong.nes";
	bool basic_mode = false;
	bool headless_mode = false;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--nestest")) {
			File f = File::fromFile("nestest.nes", "rb");
			if (f) {
				std::vector<uint8_t> buf;
				if (f.readInto(buf)) {
					static const uint8_t patch[] = "PATCH\x00\x40\x0C\x00\x02\x00\xC0""EOF";
					apply_patch(buf, cspan_u8(patch));
					load_file(buf);
					cpudebug::is_debugging = true;
					cpudebug::nestest = true;
					bus_poweron();
					cpu::step(8992);
				}
			}
			return 0;
		}
		if (!strcmp(argv[i], "--headless"))
			run_cpu = basic_mode = headless_mode = true;
		if (!strcmp(argv[i], "--basic"))
			run_cpu = basic_mode = true;
		if (argv[i][0] != '-')
			load_on_start = argv[i];
	}
	{
		File f = File::fromFile(load_on_start, "rb");
		if (f) {
			std::vector<uint8_t> buf;
			if (f.readInto(buf))
				load_file(buf);
		}
		bus_poweron();
	}

	SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE, "controller_map.txt");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER)) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}
	if (!headless_mode) {
		int w = basic_mode ? 512 : 1280;
		int h = basic_mode ? 480 : 960;
		window = SDL_CreateWindow("vhvc", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		if (!window) {
			fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
			return 1;
		}
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		if (!renderer) {
			fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
			return 1;
		}
	}
	set_sync_type(SYNC_TO_AUDIO);

	if (!audio::init())
		return 1;

	if (!headless_mode && !ppudebug::init(renderer)) {
		return 1;
	}

	if (headless_mode) {
		event_console = SDL_RegisterEvents(1);
		SDL_Thread *thread = SDL_CreateThread(console_thread, "Console Reader", nullptr);
		if (!thread) {
			fprintf(stderr, "SDL_CreateThread: %s\n", SDL_GetError());
			return 1;
		}
		SDL_DetachThread(thread);
	}

	if (!basic_mode) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();
		ImFontConfig config;
		config.MergeMode = true;
		if (File::fromFile("NotoSansCJKjp-Regular.otf", "rb"))
			io.Fonts->AddFontFromFileTTF("NotoSansCJKjp-Regular.otf", 13.0f, &config, io.Fonts->GetGlyphRangesJapanese());
		io.Fonts->Build();
		ImGui_ImplSDL2_InitForSDLRenderer(window);
		ImGui_ImplSDLRenderer_Init(renderer);
	}

	while (is_running) {
		if (headless_mode)
			events_headless();
		else if (basic_mode)
			events_basic();
		else
			events();
	}

	if (!basic_mode) {
		ImGui_ImplSDLRenderer_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	if (!headless_mode) {
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
	}
	SDL_Quit();
	return 0;
}
}

extern "C" int main(int argc, char** argv) {
	return vhvc::main(argc, argv);
}
