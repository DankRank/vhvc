#include "input.hh"
#include "imgui.h"
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
