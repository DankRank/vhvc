#include "mapper.hh"
#include "bus.hh"
#include <utility>
namespace vhvc {
#define DECLARE_MAPPER(T) \
	template<> Mapper *new_mapper<T>(NesFile &nf) { return new T(nf); }
#define DECLARE_MAPPER_INT(T) \
	template<> Mapper *new_mapper<T>(NesFile &nf, int param) { return new T(nf, param); }
struct VRC1 : BasicMapper {
	uint8_t chr0 = 0;
	uint8_t chr1 = 0;
	void poweron() {
		set_prg8k(0, nf->get_prg8k(0));
		set_prg8k(1, nf->get_prg8k(0));
		set_prg8k(2, nf->get_prg8k(0));
		set_prg8k(3, nf->get_prg8k(-1));
		set_chr4k(0, nf->get_chr4k(0));
		set_chr4k(1, nf->get_chr4k(0));
		set_mirroring(MIRRORING_VERTICAL);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		data &= 15;
		switch (addr & 0xF000) {
		case 0x8000: set_prg8k(0, nf->get_prg8k(data)); break;
		case 0xA000: set_prg8k(1, nf->get_prg8k(data)); break;
		case 0xC000: set_prg8k(2, nf->get_prg8k(data)); break;
		case 0x9000:
			set_mirroring(data&1 ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL);
			chr0 = chr0&0x0F | (data&2 ? 0x10 : 0);
			chr1 = chr1&0x0F | (data&4 ? 0x10 : 0);
			set_chr4k(0, nf->get_chr4k(chr0));
			set_chr4k(1, nf->get_chr4k(chr1));
			break;
		case 0xE000:
			chr0 = chr0&0x10 | data;
			set_chr4k(0, nf->get_chr4k(chr0));
			break;
		case 0xF000:
			chr1 = chr1&0x10 | data;
			set_chr4k(1, nf->get_chr4k(chr1));
			break;
		}
	}
	VRC1(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(VRC1)
struct VRCIRQ {
	uint8_t latch = 0;
	bool mode = false;
	bool enabled = false;
	bool enabled_after_ack = false;
	uint8_t counter = 0;
	int prescaler = 341;
	void write_ctrl(uint8_t data) {
		irq_ack(IRQ_MAPPER);
		mode = data&4;
		enabled = data&2;
		enabled_after_ack = data&1;
		if (enabled) {
			counter = latch;
			prescaler = 341;
		}
	}
	void write_ack() {
		irq_ack(IRQ_MAPPER);
		enabled = enabled_after_ack;
	}
	void tick() {
		if (enabled) {
			prescaler -= 3;
			if (mode || prescaler <= 0) {
				if (counter == 0xFF) {
					irq_raise(IRQ_MAPPER); // FIXME: this is late by two cycles
					counter = latch;
				} else {
					counter++;
				}
				if (prescaler <= 0)
					prescaler = 341;
			}
		}
	}
};
struct VRC2 : BasicMapper {
	struct VRC2Variant {
		bool isVRC4;
		bool swapped;
		int shift1;
		int shift2;
		bool isVRC2a;
		bool isVRC2c;
	};
	static constexpr VRC2Variant variants[] = {
	/*           VRC4   swap sh1 sh2 VRC2a */
	/* VRC2a */ {false, true,  0, 0, true}, // A1 A0
	/* VRC2b */ {false, false, 0}, // A0 A1
	/* VRC2c */ {false, true,  0, 0, false, true}, // A1 A0
	/* VRC4a */ {true,  false, 1}, // A1 A2
	/* VRC4b */ {true,  true,  0}, // A1 A0
	/* VRC4c */ {true,  false, 6}, // A6 A7
	/* VRC4d */ {true,  true,  2}, // A3 A2
	/* VRC4e */ {true,  false, 2}, // A2 A3
	/* VRC4f */ {true,  false, 0}, // A0 A1
	/* Map21 */ {true,  false, 1, 6}, // A1 A2 | A6 A7
	/* Map23 */ {true,  false, 0, 2}, // A0 A1 | A2 A3
	/* Map25 */ {true,  true,  0, 2}, // A1 A0 | A3 A2
	};
	VRC2Variant v;
	bool swap_mode = false;
	uint16_t chr_reg[8] = {0};
	bool vrc2_latch = false;
	VRCIRQ irq{};

	void poweron() {
		set_prg8k(0, nf->get_prg8k(0));
		set_prg8k(1, nf->get_prg8k(0));
		set_prg8k(2, nf->get_prg8k(-2));
		set_prg8k(3, nf->get_prg8k(-1));
		for (int i = 0; i < 8; i++)
			set_chr1k(i, nf->get_chr1k(0));
		set_mirroring(MIRRORING_VERTICAL);
		has_prgram = v.isVRC2c;
	}
	uint8_t cpu_read(uint16_t addr) {
		if (v.isVRC4)
			irq.tick();
		if (!v.isVRC4 && !v.isVRC2c && addr > 0x6000 && addr < 0x7000) {
			return cpu::data_bus & 0xFE | (vrc2_latch ? 1 : 0);
		}
		return BasicMapper::cpu_read(addr);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		if (addr > 0x6000 && addr < 0x7000) {
			vrc2_latch = data&1;
		}
		//if (addr < 0x8000)
		//	return;
		int low_bits = addr>>v.shift1 & 3;
		if (v.shift2)
			low_bits |= addr>>v.shift2 & 3;
		if (v.swapped)
			low_bits = low_bits>>1 | low_bits<<1 & 2;
		//printf("%04X = %02X || %04X\n", addr, data, addr&0xF000 | low_bits);
		switch(addr&0xF000 | low_bits) {
		case 0x8000: case 0x8001: case 0x8002: case 0x8003:
			set_prg8k(swap_mode ? 2 : 0, nf->get_prg8k(data&31));
			break;
		case 0xA000: case 0xA001: case 0xA002: case 0xA003:
			set_prg8k(1, nf->get_prg8k(data&31));
			break;
		case 0xB000: case 0xB001: case 0xB002: case 0xB003:
		case 0xC000: case 0xC001: case 0xC002: case 0xC003:
		case 0xD000: case 0xD001: case 0xD002: case 0xD003:
		case 0xE000: case 0xE001: case 0xE002: case 0xE003: {
			int reg = (addr>>11 & 0x1E | low_bits>>1 & 1) - (0xB<<1);
			if (low_bits & 1) {
				chr_reg[reg] = chr_reg[reg]&0x00F | (data&0x1F)<<4;
			} else {
				chr_reg[reg] = chr_reg[reg]&0x1F0 | data&0x0F;
			}
			if (v.isVRC2a)
				set_chr1k(reg, nf->get_chr1k((chr_reg[reg]&0xFF)>>1));
			else if (v.isVRC4)
				set_chr1k(reg, nf->get_chr1k(chr_reg[reg]&0xFF));
			else
				set_chr1k(reg, nf->get_chr1k(chr_reg[reg]));
			break;
		}
		case 0x9002:
			if (v.isVRC4) {
				has_prgram = data&1;
				if (bool(data&2) != swap_mode) {
					swap_mode = data&2;
					std::swap(prg[0], prg[2]);
				}
				break;
			}
			[[fallthrough]];
		case 0x9001: case 0x9003:
			if (v.isVRC4)
				break;
			[[fallthrough]];
		case 0x9000: {
			constexpr int mirroring_modes[4] = { MIRRORING_VERTICAL, MIRRORING_HORIZONTAL, MIRRORING_SCREENA, MIRRORING_SCREENB };
			if (v.isVRC4)
				set_mirroring(mirroring_modes[data&3]);
			else
				set_mirroring(mirroring_modes[data&1]);
			break;
		}
		case 0xF000: irq.latch = irq.latch&0xF0 | data&0x0F; break;
		case 0xF001: irq.latch = irq.latch&0x0F | data<<4; break;
		case 0xF002: irq.write_ctrl(data); break;
		case 0xF003: irq.write_ack(); break;
		}
		if (v.isVRC4)
			irq.tick();
	}
	VRC2(NesFile& nf, int variant) :BasicMapper(nf), v(variants[variant]) {}
};
DECLARE_MAPPER_INT(VRC2)
struct VRC3 : BasicMapper {
	uint16_t latch = 0;
	bool mode = false;
	bool enabled = false;
	bool enabled_after_ack = false;
	uint16_t counter = 0;
	void irq_tick() {
		if (enabled) {
			if (mode ? counter&0xFF == 0xFF : counter == 0xFFFF) {
				irq_raise(IRQ_MAPPER);
				if (mode)
					counter = counter&0xFF00 | latch&0x00FF;
				else
					counter = latch;
			} else {
				counter++;
			}
		}
	}
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg16k(1, nf->get_prg16k(-1));
		set_chr8k(nf->get_chr8k(0));
		chrram_check();
		set_mirroring(nf->vertical ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL);
	}
	uint8_t cpu_read(uint16_t addr) {
		irq_tick();
		return BasicMapper::cpu_read(addr);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		irq_tick();
		switch (addr&0xF000) {
		case 0x8000: latch = latch&0xFFF0 | data & 0x000F; break;
		case 0x9000: latch = latch&0xFF0F | data<<4 & 0x00F0; break;
		case 0xA000: latch = latch&0xF0FF | data<<8 & 0x0F00; break;
		case 0xB000: latch = latch&0x0FFF | data<<12 & 0xF000; break;
		case 0xC000:
			irq_ack(IRQ_MAPPER);
			mode = data&4;
			enabled = data&2;
			enabled_after_ack = data&1;
			if (enabled)
				counter = latch;
			break;
		case 0xD000:
			irq_ack(IRQ_MAPPER);
			enabled = enabled_after_ack;
			break;
		case 0xF000: set_prg16k(0, nf->get_prg16k(data&7)); break;
		}
	}
	VRC3(NesFile& nf) :BasicMapper(nf) {}
};
DECLARE_MAPPER(VRC3)
struct VRC6 : BasicMapper {
	bool swapped;
	uint8_t mode = 0;
	uint8_t cr[8] = {0};
	VRCIRQ irq{};
	void poweron() {
		set_prg16k(0, nf->get_prg16k(0));
		set_prg8k(2, nf->get_prg8k(0));
		set_prg8k(3, nf->get_prg8k(-1));
		update_mapping();
	}
	void update_mapping() {
		if ((mode & 3) == 0) {
			for (int i = 0; i < 8; i++)
				set_chr1k(i, nf->get_chr1k(cr[i]));
			constexpr int mirroring_modes[4] = { MIRRORING_VERTICAL, MIRRORING_HORIZONTAL, MIRRORING_SCREENA, MIRRORING_SCREENB };
			set_mirroring(mirroring_modes[mode>>2 & 3]);
			// TODO: ROM nametables
			// TODO: bit 5
		}
		// TODO: other modes
	}
	uint8_t cpu_read(uint16_t addr) {
		irq.tick();
		return BasicMapper::cpu_read(addr);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		irq.tick();
		if (swapped)
			addr = addr&0xFFFC | addr>>1 & 1 | addr<<1 & 2;
		switch (addr&0xF003) {
		case 0x8000: case 0x8001: case 0x8002: case 0x8003:
			set_prg16k(0, nf->get_prg16k(data&15));
			break;
		case 0xC000: case 0xC001: case 0xC002: case 0xC003:
			set_prg8k(2, nf->get_prg8k(data&31));
			break;
		case 0xB003:
			has_prgram = data&128;
			mode = data&63;
			update_mapping();
			break;
		case 0xD000: case 0xD001: case 0xD002: case 0xD003:
		case 0xE000: case 0xE001: case 0xE002: case 0xE003:
			cr[addr>>11 & 4 | addr&3] = data;
			update_mapping();
			break;
		case 0xF000: irq.latch = data; break;
		case 0xF001: irq.write_ctrl(data); break;
		case 0xF002: irq.write_ack(); break;
		}
	}
	VRC6(NesFile& nf, int variant) :BasicMapper(nf), swapped(variant) {}
};
DECLARE_MAPPER_INT(VRC6)
struct VRC7 : BasicMapper {
	uint16_t mask;
	VRCIRQ irq{};
	void poweron() {
		set_prg8k(0, nf->get_prg8k(0));
		set_prg8k(1, nf->get_prg8k(0));
		set_prg8k(2, nf->get_prg8k(0));
		set_prg8k(3, nf->get_prg8k(-1));
		for (int i = 0; i < 8; i++)
			set_chr1k(i, nf->get_chr1k(0));
		set_mirroring(MIRRORING_VERTICAL);
	}
	uint8_t cpu_read(uint16_t addr) {
		irq.tick();
		return BasicMapper::cpu_read(addr);
	}
	void cpu_write(uint16_t addr, uint8_t data) {
		BasicMapper::cpu_write(addr, data);
		irq.tick();
		switch (addr&0xF000 | (addr&mask ? 8 : 0)) {
		case 0x8000: set_prg8k(0, nf->get_prg8k(data&63)); break;
		case 0x8008: set_prg8k(1, nf->get_prg8k(data&63)); break;
		case 0x9000: set_prg8k(2, nf->get_prg8k(data&63)); break;
		case 0x9008:
			// TODO: VRC7 audio
			// vrc7_write(addr & 0x0020, data);
			break;
		case 0xA000: set_chr1k(0, nf->get_chr1k(data)); break;
		case 0xA008: set_chr1k(1, nf->get_chr1k(data)); break;
		case 0xB000: set_chr1k(2, nf->get_chr1k(data)); break;
		case 0xB008: set_chr1k(3, nf->get_chr1k(data)); break;
		case 0xC000: set_chr1k(4, nf->get_chr1k(data)); break;
		case 0xC008: set_chr1k(5, nf->get_chr1k(data)); break;
		case 0xD000: set_chr1k(6, nf->get_chr1k(data)); break;
		case 0xD008: set_chr1k(7, nf->get_chr1k(data)); break;
		case 0xE000: {
			has_prgram = data&128;
			// TODO: silence audio bit
			// vrc7_silence(data & 64);
			constexpr int mirroring_modes[4] = { MIRRORING_VERTICAL, MIRRORING_HORIZONTAL, MIRRORING_SCREENA, MIRRORING_SCREENB };
			set_mirroring(mirroring_modes[data&3]);
			break;
		}
		case 0xE008: irq.latch = data; break;
		case 0xF000: irq.write_ctrl(data); break;
		case 0xF008: irq.write_ack(); break;
		}
	}
	VRC7(NesFile& nf, int variant) :BasicMapper(nf) {
		switch (variant) {
		default: mask = 0x0018; break;
		case VARIANT_VRC7b: mask = 0x0008; break;
		case VARIANT_VRC7a: mask = 0x0010; break;
		}
	}
};
DECLARE_MAPPER_INT(VRC7)
}
