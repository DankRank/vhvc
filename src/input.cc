#include "input.hh"
#include "imgui.h"
#include "imgui_internal.h" /* for ArrowButtonEx */
namespace vhvc {

// Family BASIC Keyboard
static bool famikey_connected = false;
static bool famikey_active = false;
static uint8_t famikey_latch = 0;
static int famikey_row = 0;
static uint8_t famikey_state[9] = {0};
static void famikey_handle_input(SDL_Event* ev) {
	bool down = ev->type == SDL_KEYDOWN;
	int row, col;
	switch (ev->key.keysym.scancode) {
	case SDL_SCANCODE_F8:           row = 0; col = 0; break;
	case SDL_SCANCODE_RETURN:       row = 0; col = 1; break;
	case SDL_SCANCODE_RIGHTBRACKET: row = 0; col = 2; break; // [
	case SDL_SCANCODE_DELETE:       row = 0; col = 3; break; // ] (terrible placement, sorry)
	case SDL_SCANCODE_RALT:         row = 0; col = 4; break; // KANA
	case SDL_SCANCODE_RSHIFT:       row = 0; col = 5; break;
	case SDL_SCANCODE_BACKSLASH:    row = 0; col = 6; break; // Yen
	case SDL_SCANCODE_END:          row = 0; col = 7; break; // STOP
	case SDL_SCANCODE_F7:           row = 1; col = 0; break;
	case SDL_SCANCODE_LEFTBRACKET:  row = 1; col = 1; break; // @
	case SDL_SCANCODE_APOSTROPHE:   row = 1; col = 2; break; // :
	case SDL_SCANCODE_SEMICOLON:    row = 1; col = 3; break;
	case SDL_SCANCODE_RCTRL:        row = 1; col = 4; break; // katakana N
	case SDL_SCANCODE_SLASH:        row = 1; col = 5; break;
	case SDL_SCANCODE_MINUS:        row = 1; col = 6; break;
	case SDL_SCANCODE_EQUALS:       row = 1; col = 7; break; // ^
	case SDL_SCANCODE_F6:           row = 2; col = 0; break;
	case SDL_SCANCODE_O:            row = 2; col = 1; break;
	case SDL_SCANCODE_L:            row = 2; col = 2; break;
	case SDL_SCANCODE_K:            row = 2; col = 3; break;
	case SDL_SCANCODE_PERIOD:       row = 2; col = 4; break;
	case SDL_SCANCODE_COMMA:        row = 2; col = 5; break;
	case SDL_SCANCODE_P:            row = 2; col = 6; break;
	case SDL_SCANCODE_0:            row = 2; col = 7; break;
	case SDL_SCANCODE_F5:           row = 3; col = 0; break;
	case SDL_SCANCODE_I:            row = 3; col = 1; break;
	case SDL_SCANCODE_U:            row = 3; col = 2; break;
	case SDL_SCANCODE_J:            row = 3; col = 3; break;
	case SDL_SCANCODE_M:            row = 3; col = 4; break;
	case SDL_SCANCODE_N:            row = 3; col = 5; break;
	case SDL_SCANCODE_9:            row = 3; col = 6; break;
	case SDL_SCANCODE_8:            row = 3; col = 7; break;
	case SDL_SCANCODE_F4:           row = 4; col = 0; break;
	case SDL_SCANCODE_Y:            row = 4; col = 1; break;
	case SDL_SCANCODE_G:            row = 4; col = 2; break;
	case SDL_SCANCODE_H:            row = 4; col = 3; break;
	case SDL_SCANCODE_B:            row = 4; col = 4; break;
	case SDL_SCANCODE_V:            row = 4; col = 5; break;
	case SDL_SCANCODE_7:            row = 4; col = 6; break;
	case SDL_SCANCODE_6:            row = 4; col = 7; break;
	case SDL_SCANCODE_F3:           row = 5; col = 0; break;
	case SDL_SCANCODE_T:            row = 5; col = 1; break;
	case SDL_SCANCODE_R:            row = 5; col = 2; break;
	case SDL_SCANCODE_D:            row = 5; col = 3; break;
	case SDL_SCANCODE_F:            row = 5; col = 4; break;
	case SDL_SCANCODE_C:            row = 5; col = 5; break;
	case SDL_SCANCODE_5:            row = 5; col = 6; break;
	case SDL_SCANCODE_4:            row = 5; col = 7; break;
	case SDL_SCANCODE_F2:           row = 6; col = 0; break;
	case SDL_SCANCODE_W:            row = 6; col = 1; break;
	case SDL_SCANCODE_S:            row = 6; col = 2; break;
	case SDL_SCANCODE_A:            row = 6; col = 3; break;
	case SDL_SCANCODE_X:            row = 6; col = 4; break;
	case SDL_SCANCODE_Z:            row = 6; col = 5; break;
	case SDL_SCANCODE_E:            row = 6; col = 6; break;
	case SDL_SCANCODE_3:            row = 6; col = 7; break;
	case SDL_SCANCODE_F1:           row = 7; col = 0; break;
	case SDL_SCANCODE_ESCAPE:       row = 7; col = 1; break;
	case SDL_SCANCODE_Q:            row = 7; col = 2; break;
	case SDL_SCANCODE_LCTRL:        row = 7; col = 3; break;
	case SDL_SCANCODE_LSHIFT:       row = 7; col = 4; break;
	case SDL_SCANCODE_LALT:         row = 7; col = 5; break; // GRPH
	case SDL_SCANCODE_1:            row = 7; col = 6; break;
	case SDL_SCANCODE_2:            row = 7; col = 7; break;
	case SDL_SCANCODE_HOME:         row = 8; col = 0; break; // CLR HOME
	case SDL_SCANCODE_UP:           row = 8; col = 1; break;
	case SDL_SCANCODE_RIGHT:        row = 8; col = 2; break;
	case SDL_SCANCODE_LEFT:         row = 8; col = 3; break;
	case SDL_SCANCODE_DOWN:         row = 8; col = 4; break;
	case SDL_SCANCODE_SPACE:        row = 8; col = 5; break;
	case SDL_SCANCODE_BACKSPACE:    row = 8; col = 6; break; // DEL
	case SDL_SCANCODE_INSERT:       row = 8; col = 7; break;
	default: return;
	}
	if (down)
		famikey_state[row] |= 1<<col;
	else
		famikey_state[row] &= ~(1<<col);
}
static void famikey_write(uint8_t v) {
	if (famikey_latch&2 && !(v&2))
		if (famikey_row++ == 9)
			famikey_row = 0;
	if (v & 1)
		famikey_row = 0;
	famikey_latch = v;
}
static uint8_t famikey_read() {
	if (!(famikey_latch&4))
		return 0;
	if (famikey_row == 9)
		return 0x1E;
	// key ghosting emulation cuz why not
	// TODO: does it happen on hardware?
	uint8_t oldcols = 0;
	uint8_t cols = famikey_state[famikey_row];
	while (cols != oldcols) {
		oldcols = cols;
		for (int i = 0; i < 9; i++) {
			if (cols & famikey_state[i])
				cols |= famikey_state[i];
		}
	}
	if (famikey_latch&2)
		cols >>= 4;
	return ~cols<<1 & 0x1E;
}
void famikey_debug(bool* p_open) {
	if (ImGui::Begin("Family Keyboard Debug", p_open)) {
		constexpr int u = 15*4;
		ImVec2 pad = ImGui::GetStyle().FramePadding;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		auto key = [&](int row, int col, const char *name, int w=0, ImGuiDir dir = ImGuiDir_None) {
			if (famikey_state[row] & 1<<col) {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(.0f, .6f, .6f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(.0f, .7f, .7f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(.0f, .8f, .8f));
			}
			if (dir == ImGuiDir_None)
				ImGui::Button(name, ImVec2((u<<w)-2*pad.x, u-2*pad.y));
			else
				ImGui::ArrowButtonEx(name, dir, ImVec2((u<<w)-2*pad.x, u-2*pad.y), ImGuiButtonFlags_None);
			ImGui::SameLine();
			if (famikey_state[row] & 1<<col)
				ImGui::PopStyleColor(3);
		};
		auto shiftx = [&](float x) {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x*u);
		};
		auto shifty = [&](float y) {
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y*u);
		};
		shiftx(.25f);
		key(7, 0, "Ｆ１", 1);
		key(6, 0, "Ｆ２", 1);
		key(5, 0, "Ｆ３", 1);
		key(4, 0, "Ｆ４", 1);
		key(3, 0, "Ｆ５", 1);
		key(2, 0, "Ｆ６", 1);
		key(1, 0, "Ｆ７", 1);
		key(0, 0, "Ｆ８", 1);
		ImGui::NewLine(); ImGui::Spacing();
		shiftx(.75f);
		key(7, 6, "　！　\n１　ァ\n　ア　");
		key(7, 7, "　＂　\n２　ィ\n　イ　");
		key(6, 7, "　＃　\n３　ゥ\n　ウ　");
		key(5, 7, "　＄　\n４　ェ\n　エ　");
		key(5, 6, "　％　\n５　ォ\n　オ　");
		key(4, 7, "　＆　\n６　　\n　ナ　");
		key(4, 6, "　＇　\n７　　\n　ニ　");
		key(3, 7, "　（　\n８　　\n　ヌ　");
		key(3, 6, "　）　\n９　　\n　ネ　");
		key(2, 7, "　　　\n０　　\n　ノ　");
		key(1, 6, "　＝　\n－　　\n　ラ　");
		key(1, 7, "　　　\n＾　　\n　リ　");
		key(0, 6, "　　　\n￥　　\n　ル　");
		key(0, 7, "ＳＴＯＰ");
		shiftx(1.f);
		shifty(.5f);
		key(8, 0, " ＣＬＳ\nＨＯＭＥ");
		key(8, 7, "ＩＮＳ");
		key(8, 6, "ＤＥＬ");
		shifty(-.5f);
		ImGui::NewLine(); ImGui::Spacing();
		shiftx(.25f);
		key(7, 1, "ＥＳＣ");
		key(7, 2, "　　　\nＱ　　\n　カ　");
		key(6, 1, "　　　\nＷ　　\n　キ　");
		key(6, 6, "　　　\nＥ　　\n　ク　");
		key(5, 2, "　　　\nＲ　　\n　ケ　");
		key(5, 1, "　　　\nＴ　　\n　コ　");
		key(4, 1, "　　　\nＹ　パ\n　ハ　");
		key(3, 2, "　　　\nＵ　ピ\n　ヒ　");
		key(3, 1, "　　　\nＩ　プ\n　フ　");
		key(2, 1, "　　　\nＯ　ペ\n　ヘ　");
		key(2, 6, "　　　\nＰ　ポ\n　ホ　");
		key(1, 1, "　　　\n＠　　\n　レ　");
		key(0, 2, "　　　\n［　「\n　ロ　");
		key(0, 1, "ＲＥＴＵＲＮ", 1);
		shiftx(1.f);
		shifty(.5f);
		key(8, 1, "##up", 1, ImGuiDir_Up);
		shifty(-.5f);
		ImGui::NewLine(); ImGui::Spacing();
		shiftx(.5f);
		key(7, 3, "ＣＴＲ");
		key(6, 3, "　　　\nＡ　　\n　サ　");
		key(6, 2, "　　　\nＳ　　\n　シ　");
		key(5, 3, "　　　\nＤ　　\n　ス　");
		key(5, 4, "　　　\nＦ　　\n　セ　");
		key(4, 2, "　　　\nＧ　　\n　ソ　");
		key(4, 3, "　　　\nＨ　　\n　マ　");
		key(3, 3, "　　　\nＪ　　\n　ミ　");
		key(2, 3, "　　　\nＫ　　\n　ム　");
		key(2, 2, "　　　\nＬ　　\n　メ　");
		key(1, 3, "　＋　\n；　　\n　モ　");
		key(1, 2, "　＊　\n：　　\n　ー　");
		key(0, 3, "　　　\n［　」\n　。　");
		key(0, 4, "カナ");
		shiftx(.75f);
		shifty(.5f);
		key(8, 3, "##left", 1, ImGuiDir_Left);
		key(8, 2, "##right", 1, ImGuiDir_Right);
		shifty(-.5f);
		ImGui::NewLine(); ImGui::Spacing();
		key(7, 4, "ＳＨＩＦＴ##left", 1);
		key(6, 5, "　　　\nＺ　　\n　タ　");
		key(6, 4, "　　　\nＸ　　\n　チ　");
		key(5, 5, "　　　\nＣ　ッ\n　ツ　");
		key(4, 5, "　　　\nＶ　　\n　テ　");
		key(4, 4, "　　　\nＢ　　\n　ト　");
		key(3, 5, "　　　\nＮ　ャ\n　ヤ　");
		key(3, 4, "　　　\nＭ　ュ\n　ユ　");
		key(2, 5, "　＜　\n，　ョ\n　ヨ　");
		key(2, 4, "　＞　\n．　　\n　ワ　");
		key(1, 5, "　？　\n／　　\n　ヲ　");
		key(1, 4, "　＿　\n　　　\n　ン　");
		key(0, 5, "ＳＨＩＦＴ##right", 1);
		shiftx(1.25f);
		shifty(.5f);
		key(8, 4, "##down", 1, ImGuiDir_Down);
		shifty(-.5f);
		ImGui::NewLine(); ImGui::Spacing();
		shiftx(3.f);
		key(7, 5, "ＧＲＰＨ");
		key(8, 5, "##space", 3);
		ImGui::NewLine(); ImGui::Spacing();
		ImGui::PopStyleVar();
	}
	ImGui::End();
}
struct Joy joy1 = {};
struct Joy joy2 = {};
static uint8_t kbd_state = 0;
uint8_t nsf_console_input = 0;
static Joy* find_joy(int id) {
	if (joy1.joyid == id)
		return &joy1;
	else if (joy2.joyid == id)
		return &joy2;
	return nullptr;
}
void handle_input(SDL_Event* ev) {
	switch (ev->type) {
	case SDL_CONTROLLERDEVICEADDED: {
		SDL_GameController* controller = SDL_GameControllerOpen(ev->cdevice.which);
		struct Joy* j = find_joy(-1);
		if (j) {
			j->joyid = SDL_JoystickGetDeviceInstanceID(ev->cdevice.which);
			j->state = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B) << 0 |
				SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A) << 1 |
				SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK) << 2 |
				SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START) << 3 |
				SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) << 4 |
				SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) << 5 |
				SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) << 6 |
				SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) << 7;
		}
		break;
	}
	case SDL_CONTROLLERDEVICEREMOVED: {
		struct Joy* j = find_joy(ev->cdevice.which);
		if (j) {
			j->joyid = -1;
			j->state = 0;
		}

		SDL_GameController* controller = SDL_GameControllerFromInstanceID(ev->cdevice.which);
		if (controller)
			SDL_GameControllerClose(controller);
		break;
	}
	case SDL_CONTROLLERBUTTONUP:
	case SDL_CONTROLLERBUTTONDOWN: {
		struct Joy* j = find_joy(ev->cdevice.which);
		if (j) {
			uint8_t button = 0;
			switch (ev->cbutton.button) {
			case SDL_CONTROLLER_BUTTON_B: button = 0x01; break;
			case SDL_CONTROLLER_BUTTON_A: button = 0x02; break;
			case SDL_CONTROLLER_BUTTON_BACK: button = 0x04; break;
			case SDL_CONTROLLER_BUTTON_START: button = 0x08; break;
			case SDL_CONTROLLER_BUTTON_DPAD_UP: button = 0x10; break;
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN: button = 0x20; break;
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT: button = 0x40; break;
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: button = 0x80; break;
			}
			if (button) {
				if (ev->type == SDL_CONTROLLERBUTTONDOWN)
					j->state |= button;
				else
					j->state &= ~button;
			}
		}
		break;
	}
	case SDL_KEYDOWN:
	case SDL_KEYUP: {
		if (famikey_active) {
			famikey_handle_input(ev);
			break;
		}
		uint8_t button = 0;
		switch (ev->key.keysym.scancode) {
		case SDL_SCANCODE_X: button = 0x01; break;
		case SDL_SCANCODE_Z: button = 0x02; break;
		case SDL_SCANCODE_BACKSLASH:
		case SDL_SCANCODE_A: button = 0x04; break;
		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_S: button = 0x08; break;
		case SDL_SCANCODE_UP: button = 0x10; break;
		case SDL_SCANCODE_DOWN: button = 0x20; break;
		case SDL_SCANCODE_LEFT: button = 0x40; break;
		case SDL_SCANCODE_RIGHT: button = 0x80; break;
		default: break;
		}
		if (button) {
			if (ev->type == SDL_KEYDOWN)
				kbd_state |= button;
			else
				kbd_state &= ~button;
		}
		break;
	}
	}
}
void input_debug(bool* p_open) {
	if (ImGui::Begin("Joy Debug", p_open)) {
		ImGui::Text("joy1: %s", joy1.joyid == -1 ? "<None>" : SDL_GameControllerName(SDL_GameControllerFromInstanceID(joy1.joyid)));
		ImGui::Text("joy2: %s", joy2.joyid == -1 ? "<None>" : SDL_GameControllerName(SDL_GameControllerFromInstanceID(joy2.joyid)));
		ImGui::Text("joy1: %c %c %c %c %c %c %c %c",
			joy1.state & 0x80 ? 'R' : '-',
			joy1.state & 0x40 ? 'L' : '-',
			joy1.state & 0x20 ? 'D' : '-',
			joy1.state & 0x10 ? 'U' : '-',
			joy1.state & 0x08 ? 'S' : '-',
			joy1.state & 0x04 ? 'C' : '-',
			joy1.state & 0x02 ? 'B' : '-',
			joy1.state & 0x01 ? 'A' : '-');
		ImGui::Text("joy2: %c %c %c %c %c %c %c %c",
			joy2.state & 0x80 ? 'R' : '-',
			joy2.state & 0x40 ? 'L' : '-',
			joy2.state & 0x20 ? 'D' : '-',
			joy2.state & 0x10 ? 'U' : '-',
			joy2.state & 0x08 ? 'S' : '-',
			joy2.state & 0x04 ? 'C' : '-',
			joy2.state & 0x02 ? 'B' : '-',
			joy2.state & 0x01 ? 'A' : '-');
		ImGui::Text("kbd:  %c %c %c %c %c %c %c %c",
			kbd_state & 0x80 ? 'R' : '-',
			kbd_state & 0x40 ? 'L' : '-',
			kbd_state & 0x20 ? 'D' : '-',
			kbd_state & 0x10 ? 'U' : '-',
			kbd_state & 0x08 ? 'S' : '-',
			kbd_state & 0x04 ? 'C' : '-',
			kbd_state & 0x02 ? 'B' : '-',
			kbd_state & 0x01 ? 'A' : '-');
		ImGui::Checkbox("Family Keyboard Connected", &famikey_connected);
		ImGui::Checkbox("Family Keyboard Active", &famikey_active);
	}
	ImGui::End();
}
static uint8_t out_latch = 0;
static uint8_t joy1_shiftreg = 0;
static uint8_t joy2_shiftreg = 0;
uint8_t read_4016() {
	uint8_t rv = joy1_shiftreg & 1;
	joy1_shiftreg >>= 1;
	return rv;
}
uint8_t read_4017() {
	uint8_t rv = joy2_shiftreg & 1;
	joy2_shiftreg >>= 1;
	if (famikey_connected)
		rv |= famikey_read();
	return rv;
}
void write_4016(uint8_t data) {
	if (!(out_latch & 1) && data & 1) {
		joy1_shiftreg = joy1.state | kbd_state;
		joy2_shiftreg = joy2.state;
	}
	out_latch = data;
	if (famikey_connected)
		famikey_write(data);
}
}
