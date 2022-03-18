#include "input.hh"
#include "imgui.h"
namespace vhvc {

struct Joy joy1 = {};
struct Joy joy2 = {};
static uint8_t kbd_state = 0;
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
	return rv;
}
void write_4016(uint8_t data) {
	if (!(out_latch & 1) && data & 1) {
		joy1_shiftreg = joy1.state | kbd_state;
		joy2_shiftreg = joy2.state;
	}
	out_latch = data;
}
}
