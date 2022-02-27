#include "bus.hh"
#include "apu.hh"
#include "audio.hh"
#include <array>
#include <algorithm>
namespace vhvc::apu {
// NTSC APU clock: 315/88*1000000*6/12/2 Hz = 9843750/11 Hz = 11/9843750 s = 704/630000000 s
// Desired sample rate: 48000 Hz = 1/48000 s = 13125/630000000 s
static constexpr int apu_period = 704;
static constexpr int sampling_period = 13125;
int sampling_clock = 0;

bool quarter_clock = false;
bool half_clock = false;
static constexpr uint8_t pulse_sequences[4][8] = {
	{ 0, 1, 0, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 1, 1, 0, 0, 0 },
	{ 1, 0, 0, 1, 1, 1, 1, 1 },
};
static constexpr uint8_t length_table[32] = {
	10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};
static constexpr uint8_t noise_period_ntsc[16] = {
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};
static constexpr uint8_t noise_period_pal[16] = {
	4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
};
static constexpr std::array<int16_t, 31> pulse_table_gen() {
	std::array<int16_t, 31> tab{};
	tab[0] = 0;
	for (int i = 1; i < 31; i++)
		tab[i] = (95.52 / (8128.0 / i + 100)) * 2000;
	return tab;
}
static constexpr std::array<int16_t, 203> tnd_table_gen() {
	std::array<int16_t, 203> tab{};
	tab[0] = 0;
	for (int i = 1; i < 203; i++)
		tab[i] = (163.67 / (24329.0 / i + 100)) * 2000;
	return tab;
}
static constexpr std::array<int16_t, 31> pulse_table = pulse_table_gen();
static constexpr std::array<int16_t, 203> tnd_table = tnd_table_gen();
static constexpr int16_t mix(int pulse1, int pulse2, int triangle, int noise, int dmc) {
	return std::clamp<int16_t>(pulse_table[pulse1 + pulse2] + tnd_table[triangle*3 + noise*2 + dmc], INT16_MIN, INT16_MAX);
}
// Some terminology
// clock = a counter that ticks down when clocked and does something when it hits 0
// timer = the reload value for the said counter
// probably more correct names would be timer_counter and timer_period/timer_reload
struct Pulse {
	bool enabled = false;
	int duty = 0;
	int sequence_no = 0;
	int clock = 0;
	int timer = 0;

	int length_counter = 0;
	bool length_halt = false;

	bool constant_vol = false;
	int envelope_param = 0;
	bool start_flag = false;
	int envelope_divider = 0;
	int envelope_decay = 0;

	bool sweep_en = false;
	int sweep_next = 0;
	int sweep_clock = 0;
	int sweep_timer = 0;
	bool sweep_timer_reload = false;
	bool sweep_negate = false;
	int sweep_shift = 0;
	bool sweep_ch2 = false;
	bool sweep_mute = false;
	void sweep_recalc() {
		int delta = timer>>sweep_shift;
		if (sweep_negate)
			delta = ~delta + (int)sweep_ch2;
		int next = timer + delta;
		// TODO: figure out how muting works under negation
		sweep_mute = next&0x800 || timer < 8;
		sweep_next = next&0x7FF;
	}
	void tick() {
		if (half_clock) {
			if (sweep_clock == 0 && sweep_en && sweep_shift != 0 && !sweep_mute) {
				timer = sweep_next;
				sweep_recalc();
			}
			if (sweep_clock == 0 || sweep_timer_reload) {
				sweep_clock = sweep_timer;
				sweep_timer_reload = false;
			} else {
				sweep_clock--;
			}
			if (!length_halt && length_counter)
				length_counter--;
		}
		if (quarter_clock) {
			if (start_flag) {
				start_flag = 0;
				envelope_decay = 15;
				envelope_divider = envelope_param;
			} else {
				if (!envelope_divider) {
					envelope_divider = envelope_param;
					if (!envelope_decay) {
						if (length_halt)
							envelope_decay = 15;
					} else {
						envelope_decay--;
					}
				} else {
					envelope_divider--;
				}
			}
		}

		if (clock == 0) {
			sequence_no++;
			sequence_no &= 7;
			clock = timer;
		} else {
			clock--;
		}	
	}
	int output() {
		if (sweep_mute || !pulse_sequences[duty][sequence_no] || !length_counter)
			return 0;
		return constant_vol ? envelope_param : envelope_decay;
	}
	Pulse(bool sweep_ch2) :sweep_ch2(sweep_ch2) {}
};
Pulse pulse1(false);
Pulse pulse2(true);
struct Triangle {
	bool enabled = false;
	int sequence_no = 0;
	int clock = 0;
	int timer = 0;

	int length_counter = 0;
	bool length_halt = false;

	int linear_clock = 0;
	int linear_timer = 0;
	bool linear_reload = false;

	void tick() {
		if (half_clock) {
			if (!length_halt && length_counter)
				length_counter--;
		}
		if (quarter_clock) {
			if (linear_reload) {
				linear_clock = linear_timer;
			} else {
				if (linear_clock)
					linear_clock--;
			}
			if (!length_halt)
				linear_reload = false;
		}
		if (clock == 0) {
			if (length_counter && linear_clock) {
				sequence_no++;
				sequence_no &= 31;
			}
			clock = timer;
		} else {
			clock--;
		}
	}
	int output() {
		return sequence_no < 16 ? 15 - sequence_no : sequence_no - 16;
	}
};
Triangle triangle{};
struct Noise {
	bool enabled = false;
	uint16_t sr = 1;
	bool mode = false;
	int clock = 0;
	int timer = 0;

	int length_counter = 0;
	bool length_halt = false;

	bool constant_vol = false;
	int envelope_param = 0;
	bool start_flag = false;
	int envelope_divider = 0;
	int envelope_decay = 0;

	void tick() {
		if (half_clock) {
			if (!length_halt && length_counter)
				length_counter--;
		}
		if (quarter_clock) {
			if (start_flag) {
				start_flag = 0;
				envelope_decay = 15;
				envelope_divider = envelope_param;
			} else {
				if (!envelope_divider) {
					envelope_divider = envelope_param;
					if (!envelope_decay) {
						if (length_halt)
							envelope_decay = 15;
					} else {
						envelope_decay--;
					}
				} else {
					envelope_divider--;
				}
			}
		}
		if (clock == 0) {
			if (mode)
				sr |= (sr>>6 ^ sr)<<15;
			else
				sr |= (sr>>1 ^ sr)<<15;
			sr >>= 1;
			clock = timer;
		} else {
			clock--;
		}
	}
	int output() {
		if (sr & 1 || !length_counter)
			return 0;
		return constant_vol ? envelope_param : envelope_decay;
	}
};
Noise noise{};
bool five_step = false;
bool interrupt_inhibit = true;
int frame_counter = 0;
void do_cycle() {
	// FIXME: ugly
	
	switch (frame_counter) {
	case 3728*2 + 1: quarter_clock = true; break;
	case 7456*2 + 1: quarter_clock = true; half_clock = true; break;
	case 11185*2 + 1: quarter_clock = true; break;
	case 14914*2:
		if (!five_step && !interrupt_inhibit)
			set_irq_internal(true);
		break;
	case 14914*2 + 1:
		if (!five_step) {
			quarter_clock = true; half_clock = true;
			if (!interrupt_inhibit)
				set_irq_internal(true);
		}
		break;
	case 14915*2:
		if (!five_step) {
			if (!interrupt_inhibit)
				set_irq_internal(true);
			frame_counter = 0;
		}
		break;
	case 18640*2 + 1:
		quarter_clock = true; half_clock = true;
		frame_counter = 0;
		break;
	}
	triangle.tick();
	if ((frame_counter & 1) == 1) {
		pulse1.tick();
		pulse2.tick();
		noise.tick();
		sampling_clock += apu_period;
		if (sampling_clock > sampling_period) {
			sampling_clock -= sampling_period;
			audio::enqueue(mix(pulse1.output(), pulse2.output(), triangle.output(), noise.output(), 0));
		}
	}
	frame_counter++;
	quarter_clock = false;
	half_clock = false;
}
uint8_t read_4015() {
	if (!bus_inspect)
		set_irq_internal(false);
	return (pulse1.length_counter ? 1 : 0) |
		(pulse2.length_counter ? 2 : 0) |
		(triangle.length_counter ? 4 : 0) |
		(noise.length_counter ? 8 : 0);
}
void reg_write(uint16_t addr, uint8_t data) {
	switch (addr) {
	case 0x4000:
	case 0x4004: {
		Pulse& p = (addr & 4 ? pulse2 : pulse1);
		p.duty = data>>6;
		p.length_halt = data&0x20;
		p.constant_vol = data&0x10;
		p.envelope_param = data&0x0F;
		break;
	}
	case 0x4001:
	case 0x4005: {
		Pulse& p = (addr & 4 ? pulse2 : pulse1);
		p.sweep_en = data&0x80;
		p.sweep_timer = data>>4 & 0x7;
		p.sweep_timer_reload = true;
		p.sweep_negate = data&0x08;
		p.sweep_shift = data&3;
		p.sweep_recalc();
		break;
	}
	case 0x4002:
	case 0x4006: {
		Pulse& p = (addr & 4 ? pulse2 : pulse1);
		p.timer = p.timer&0x700 | data;
		p.sweep_recalc();
		break;
	}
	case 0x4003:
	case 0x4007: {
		Pulse& p = (addr & 4 ? pulse2 : pulse1);
		if (p.enabled)
			p.length_counter = length_table[data>>3];
		p.timer = p.timer&0x0FF | (data&7)<<8;
		p.start_flag = true;
		p.sequence_no = 0;
		p.sweep_recalc();
		break;
	}
	case 0x4008:
		triangle.length_halt = data&0x80;
		triangle.linear_timer = data&0x7F;
		break;
	case 0x400A:
		triangle.timer = triangle.timer&0x700 | data;
		break;
	case 0x400B:
		if (triangle.enabled)
			triangle.length_counter = length_table[data>>3];
		triangle.timer = triangle.timer&0x0FF | (data&7)<<8;
		triangle.linear_reload = true;
		break;
	case 0x400C:
		noise.length_halt = data&0x20;
		noise.constant_vol = data&0x10;
		noise.envelope_param = data&0x0F;
		break;
	case 0x400E:
		noise.mode = data&0x80;
		noise.timer = noise_period_ntsc[data&0x0F];
		break;
	case 0x400F:
		if (noise.enabled)
			noise.length_counter = length_table[data>>3];
		noise.start_flag = true;
		break;
	case 0x4015:
		if (!(pulse1.enabled = data&1))
			pulse1.length_counter = 0;
		if (!(pulse2.enabled = data&2))
			pulse2.length_counter = 0;
		if (!(triangle.enabled = data&4))
			triangle.length_counter = 0;
		if (!(noise.enabled = data&8))
			noise.length_counter = 0;
		break;
	case 0x4017:
		five_step = data & 0x80;
		interrupt_inhibit = data & 0x40;
		// FIXME: the following stuff should be delayed by 3 or 4 cycles
		frame_counter = 0;
		quarter_clock = true;
		half_clock = true;
		break;
	}
}
}
