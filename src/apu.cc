#include "audio.hh"
#include "bus.hh"
#include "apu.hh"
#include <array>
#include <algorithm>
namespace vhvc::apu {
File dump_file;

// NTSC APU clock: 315/88*1000000*6/12/2 Hz = 9843750/11 Hz = 11/9843750 s = 704/630000000 s
// Desired sample rate: 48000 Hz = 1/48000 s = 13125/630000000 s
static constexpr int apu_period = 704;
static constexpr int sampling_period = 13125;
// PAL APU clock: 443361875/100*6/16/2 Hz = 53203425/64 Hz = 64/53203425 s = 40960/34050192000 s
// Desired sample rate: 48000 Hz = 1/48000 s = 709379/34050192000 s
//static constexpr int apu_period = 40960;
//static constexpr int sampling_period = 709379;
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
static constexpr unsigned noise_period_ntsc[16] = {
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};
static constexpr unsigned noise_period_pal[16] = {
	4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
};
static constexpr unsigned dmc_rate_ntsc[16] = {
	428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54
};
static constexpr unsigned dmc_rate_pal[16] = {
	398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98,  78,  66,  50
};
static constexpr int16_t volume = INT16_MAX * .5;
static constexpr std::array<int16_t, 31> pulse_table_gen() {
	std::array<int16_t, 31> tab{};
	tab[0] = 0;
	for (int i = 1; i < 31; i++)
		tab[i] = (95.52 / (8128.0 / i + 100)) * volume;
	return tab;
}
static constexpr std::array<int16_t, 203> tnd_table_gen() {
	std::array<int16_t, 203> tab{};
	tab[0] = 0;
	for (int i = 1; i < 203; i++)
		tab[i] = (163.67 / (24329.0 / i + 100)) * volume;
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
struct LengthCounter {
	int counter = 0;
	bool halt = false;
	void tick() {
		if (!halt && counter)
			counter--;
	}
};
struct Envelope {
	bool constant_vol = false;
	int param = 0;
	bool start_flag = false;
	int divider = 0;
	int decay = 0;
	bool loop = false;
	void tick() {
		if (start_flag) {
			start_flag = false;
			decay = 15;
			divider = param;
		} else {
			if (!divider) {
				divider = param;
				if (!decay) {
					if (loop)
						decay = 15;
				} else {
					decay--;
				}
			} else {
				divider--;
			}
		}
	}
	int output() {
		return constant_vol ? param : decay;
	}
};
struct Pulse {
	bool enabled = false;
	int duty = 0;
	int sequence_no = 0;
	int clock = 0;
	int timer = 0;

	LengthCounter lc;
	Envelope ev;

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
	void sweep_tick() {
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
	}
	void tick() {
		if (clock == 0) {
			sequence_no++;
			sequence_no &= 7;
			clock = timer;
		} else {
			clock--;
		}
	}
	int output() {
		if (sweep_mute || !pulse_sequences[duty][sequence_no] || !lc.counter)
			return 0;
		return ev.output();
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

	LengthCounter lc;

	int linear_clock = 0;
	int linear_timer = 0;
	bool linear_reload = false;

	void linear_tick() {
		if (linear_reload) {
			linear_clock = linear_timer;
		} else {
			if (linear_clock)
				linear_clock--;
		}
		if (!lc.halt)
			linear_reload = false;
	}
	void tick() {
		if (clock == 0) {
			if (lc.counter && linear_clock) {
				sequence_no++;
				sequence_no &= 31;
			}
			clock = timer;
		} else {
			clock--;
		}
	}
	int output() {
		/* Megaman 1 and 2 set timer to 0 to silence triangle channel.
		 * If we don't filter out ultrasonic frequencies, we get hiss from aliasing.
		 * This can be considered a substitute for a low-pass filter.
		 * The formula for triangle period length is apu_period/2 * 32 * (t+1)
		 * | timer | NTSC     | PAL      |
		 * | t = 0 | 55930 Hz | 51956 Hz |
		 * | t = 1 | 27965 Hz | 25978 Hz |
		 * | t = 2 | 18643 Hz | 17319 Hz |
		 */
		if (timer < 2 && lc.counter && linear_clock)
			return 8; // actually 7.5
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

	LengthCounter lc;
	Envelope ev;

	void tick() {
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
		if (sr & 1 || !lc.counter)
			return 0;
		return ev.output();
	}
};
Noise noise{};
struct DMC {
	bool enabled = false;
	bool irq_enabled = false;
	bool loop = false;
	int clock = 0;
	int timer = 54;
	uint8_t level = 0;
	uint16_t addr = 0xC000;
	uint16_t len = 0;
	uint16_t cur_addr = 0xC000;
	uint16_t remaining = 0;

	int bits_remaining = 8;
	bool silence = true;
	uint8_t sr = 0;

	uint8_t buffer = 0;
	bool buffer_full = false;
	bool restart_pending = false;

	void tick() {
		if (clock == 0) {
			clock = timer;
			if (!silence) {
				if (sr & 1) {
					if (level < 126)
						level += 2;
				} else if (level > 1) {
					level -= 2;
				}
			}
			sr >>= 1;
			if (--bits_remaining == 0) {
				if (buffer_full) {
					sr = buffer;
					silence = false;
					buffer_full = false;
				} else {
					silence = true;
				}
				bits_remaining = 8;
			}
		} else {
			clock--;
		}
		if (enabled && !buffer_full && remaining) {
			inspect_lock lk; // FIXME: actual DMA
			buffer = cpu_read(cur_addr);
			buffer_full = true;
			cur_addr++;
			cur_addr |= 0x8000;
			if (--remaining == 0) {
				if (loop || restart_pending) {
					cur_addr = addr;
					remaining = len;
					restart_pending = false;
				} else {
					if (irq_enabled)
						irq_raise(IRQ_DMC);
				}
			}
		}
	}
	int output() {
		return level;
	}
};
DMC dmc{};
bool five_step = false;
bool interrupt_inhibit = true;
int frame_counter = 0;
int delay_4017 = 0;
enum {
	STEP_1,
	STEP_2,
	STEP_3,
	STEP_4_PRE,
	STEP_4,
	STEP_4_POST,
	STEP_5
};
template<bool NTSC>
static constexpr always_inline int counter_to_index(int ctr) {
	switch (ctr) {
		case (NTSC?3728:4156)*2 + 1:   return STEP_1;
		case (NTSC?7456:8313)*2 + 1:   return STEP_2;
		case (NTSC?11185:12469)*2 + 1: return STEP_3;
		case (NTSC?14914:16626)*2:     return STEP_4_PRE;
		case (NTSC?14914:16626)*2 + 1: return STEP_4;
		case (NTSC?14915:16627)*2:     return STEP_4_POST;
		case (NTSC?18640:20782)*2 + 1: return STEP_5;
		default: return -1;
	}
}
void do_cycle() {
	// FIXME: ugly

	switch (counter_to_index<true>(frame_counter)) {
	case STEP_1: quarter_clock = true; break;
	case STEP_2: quarter_clock = true; half_clock = true; break;
	case STEP_3: quarter_clock = true; break;
	case STEP_4_PRE:
		if (!five_step && !interrupt_inhibit)
			irq_raise(IRQ_FRAMECOUNTER);
		break;
	case STEP_4:
		if (!five_step) {
			quarter_clock = true; half_clock = true;
			if (!interrupt_inhibit)
				irq_raise(IRQ_FRAMECOUNTER);
		}
		break;
	case STEP_4_POST:
		if (!five_step) {
			if (!interrupt_inhibit)
				irq_raise(IRQ_FRAMECOUNTER);
			frame_counter = 0;
		}
		break;
	case STEP_5:
		quarter_clock = true; half_clock = true;
		frame_counter = 0;
		break;
	}
	if ((frame_counter & 1) == 1) {
		if (delay_4017 && !--delay_4017) {
			frame_counter = 0;
			if (five_step) {
				quarter_clock = true;
				half_clock = true;
			}
		}
	}
	if (half_clock) {
		pulse1.sweep_tick();
		pulse2.sweep_tick();
		pulse1.lc.tick();
		pulse2.lc.tick();
		triangle.lc.tick();
		noise.lc.tick();
	}
	if (quarter_clock) {
		pulse1.ev.tick();
		pulse2.ev.tick();
		triangle.linear_tick();
		noise.ev.tick();
	}
	triangle.tick();
	if ((frame_counter & 1) == 1) {
		pulse1.tick();
		pulse2.tick();
		noise.tick();
		dmc.tick();
		sampling_clock += apu_period;
		if (sampling_clock > sampling_period) {
			sampling_clock -= sampling_period;
			if (dump_file) {
				int16_t buf[5] = {
					mix(pulse1.output(), 0, 0, 0, 0),
					mix(0, pulse2.output(), 0, 0, 0),
					mix(0, 0, triangle.output(), 0, 0),
					mix(0, 0, 0, noise.output(), 0),
					mix(0, 0, 0, 0, dmc.output()),
				};
				dump_file.write(buf, 2, 5);
			}
			audio::enqueue(mix(pulse1.output(), pulse2.output(), triangle.output(), noise.output(), dmc.output()));
		}
	}
	frame_counter++;
	quarter_clock = false;
	half_clock = false;
}
uint8_t read_4015() {
	// TODO: race between reading and clearing FC irq
	bool fcirq = irq_status(IRQ_FRAMECOUNTER);
	if (!bus_inspect)
		irq_ack(IRQ_FRAMECOUNTER);
	return (pulse1.lc.counter ? 1 : 0) |
		(pulse2.lc.counter ? 2 : 0) |
		(triangle.lc.counter ? 4 : 0) |
		(noise.lc.counter ? 8 : 0) |
		(fcirq ? 0x40 : 0) |
		(irq_status(IRQ_DMC) ? 0x80 : 0);
}
void reg_write(uint16_t addr, uint8_t data) {
	switch (addr) {
	case 0x4000:
	case 0x4004: {
		Pulse& p = (addr & 4 ? pulse2 : pulse1);
		p.duty = data>>6;
		p.ev.loop = p.lc.halt = data&0x20;
		p.ev.constant_vol = data&0x10;
		p.ev.param = data&0x0F;
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
			p.lc.counter = length_table[data>>3];
		p.timer = p.timer&0x0FF | (data&7)<<8;
		p.ev.start_flag = true;
		p.sequence_no = 0;
		p.sweep_recalc();
		break;
	}
	case 0x4008:
		triangle.lc.halt = data&0x80;
		triangle.linear_timer = data&0x7F;
		break;
	case 0x400A:
		triangle.timer = triangle.timer&0x700 | data;
		break;
	case 0x400B:
		if (triangle.enabled)
			triangle.lc.counter = length_table[data>>3];
		triangle.timer = triangle.timer&0x0FF | (data&7)<<8;
		triangle.linear_reload = true;
		break;
	case 0x400C:
		noise.ev.loop = noise.lc.halt = data&0x20;
		noise.ev.constant_vol = data&0x10;
		noise.ev.param = data&0x0F;
		break;
	case 0x400E:
		noise.mode = data&0x80;
		noise.timer = noise_period_ntsc[data&0x0F];
		break;
	case 0x400F:
		if (noise.enabled)
			noise.lc.counter = length_table[data>>3];
		noise.ev.start_flag = true;
		break;
	case 0x4010:
		dmc.irq_enabled = data&0x80;
		if (!dmc.irq_enabled)
			irq_ack(IRQ_DMC);
		dmc.loop = data&0x40;
		dmc.timer = dmc_rate_ntsc[data&0x0F]/2;
		break;
	case 0x4011: dmc.level = data&0x7F; break;
	case 0x4012: dmc.addr = 0xC000 | data<<6; break;
	case 0x4013: dmc.len = data<<4 | 1; break;
	case 0x4015:
		if (!(pulse1.enabled = data&1))
			pulse1.lc.counter = 0;
		if (!(pulse2.enabled = data&2))
			pulse2.lc.counter = 0;
		if (!(triangle.enabled = data&4))
			triangle.lc.counter = 0;
		if (!(noise.enabled = data&8))
			noise.lc.counter = 0;
		if ((dmc.enabled = data & 16)) {
			if (dmc.remaining == 0) {
				dmc.cur_addr = dmc.addr;
				dmc.remaining = dmc.len;
			} else {
				dmc.restart_pending = true;
			}
		} else {
			dmc.remaining = 0;
		}
		irq_ack(IRQ_DMC);
		break;
	case 0x4017:
		five_step = data & 0x80;
		interrupt_inhibit = data & 0x40;
		if (interrupt_inhibit)
			irq_ack(IRQ_FRAMECOUNTER);
		// FIXME: the following stuff should be delayed by 3 or 4 cycles
		delay_4017 = 2;
		break;
	}
}
}
