#include "mapper.hh"
#include "bus.hh"
#include "cpu.hh"
#include "apu.hh"
#include "input.hh"
#include <utility>
namespace vhvc {
struct NsfMapper : Mapper {
	NsfFile* nf = nullptr;
	uint8_t* prg[8] = {0}; // 4k granularity
	uint8_t prgram[8192];
	uint8_t current_song;
	uint8_t ram_41FF;
	uint8_t last_input;
	void set_bank(int slot, uint8_t bank) {
		prg[slot] = nf->rom.data() + (bank%nf->bank_count)*0x1000;
	}
	void poweron() {
		for (int i = 0; i < 8; i++)
			set_bank(i, i);
		current_song = nf->starting_song-1;
		ram_41FF = 0;
		last_input = 0;
	}
	uint8_t cpu_read(uint16_t addr) {
		if (addr & 0x8000) {
			if (addr >= 0xFFFA)
				switch (addr) {
					case 0xFFFA: return 0x07; // nmi low
					case 0xFFFB: return 0x41; // nmi high
					case 0xFFFC: return 0x10; // reset low
					case 0xFFFD: return 0x41; // reset high
					case 0xFFFE: return 0x06; // irq low
					case 0xFFFF: return 0x41; // irq high
				}
			return prg[addr>>12 & 7][addr & 0xFFF];
		}
		if ((addr & 0xE000) == 0x6000)
			return prgram[addr & 0x1FFF];
		if (addr >= 0x4100 && addr < 0x4200) {
			static constexpr uint8_t reset_rom[256] = {
				// INIT call at 0x4100
				0x4C, 0x00, 0x00, // JMP INIT
				// PLAY call at 0x4103
				0x4C, 0x00, 0x00, // JMP PLAY
				// IRQ at 0x4106
				0x40,             // RTI
				// NMI at 0x4107
				0x48,             // PHA
				0xAD, 0x02, 0x20, // LDA $2002
				0x8D, 0xFF, 0x41, // STA $41FF
				0x68,             // PLA
				0x40,             // RTI
				// RESET at 0x4110
				// basic PPU initialization
				0x78,             // SEI
				0xD8,             // CLD
				0xA9, 0x00,       // LDA #$00
				0x8D, 0x00, 0x20, // STA $2000
				0x8D, 0x01, 0x20, // STA $2001
				0x2C, 0x02, 0x20, // BIT 0x2002
				0x2C, 0x02, 0x20, // BIT 0x2002
				0x10, 0xFB,       // BPL -5
				0x2C, 0x02, 0x20, // BIT 0x2002
				0x10, 0xFB,       // BPL -5
				0xA9, 0x80,       // LDA #$80
				0x8D, 0x00, 0x20, // STA $2000
				// run INIT
				0x8D, 0xFE, 0x41, // STA $41FE
				0x20, 0x00, 0x41, // JSR 0x4100
				// wait one more frame
				0xA9, 0x00,       // LDA #$00
				0x8D, 0xFF, 0x41, // STA $41FF
				0x2C, 0xFF, 0x41, // BIT 0x41FF
				0x10, 0xFB,       // BPL -5
				// main loop
				0xA9, 0x00,       // LDA #$00
				0x8D, 0xFF, 0x41, // STA $41FF
				0x20, 0x03, 0x41, // JSR 0x4103
				0x2C, 0xFF, 0x41, // BIT 0x41FF
				0x10, 0xFB,       // BPL -5
				0x30, 0xF1,       // BMI -15
			};
			switch (addr) {
				case 0x4101: return nf->init & 0xFF;
				case 0x4102: return nf->init >> 8;
				case 0x4104: return nf->play & 0xFF;
				case 0x4105: return nf->play >> 8;
				case 0x41FF: return ram_41FF;
			}
			return reset_rom[addr & 0xFF];
		}
		return cpu::data_bus;

	}
	void cpu_write(uint16_t addr, uint8_t data) {
		if ((addr & 0xE000) == 0x6000)
			prgram[addr & 0x1FFF] = data;
		if ((addr & 0x5FF8) == 0x5FF8 && nf->banking)
			set_bank(addr & 7, data);
		if (addr == 0x41FE) {
			memset(cpu_ram, 0, sizeof(cpu_ram));
			memset(prgram, 0, sizeof(prgram));
			for (int i = 0x4000; i < 0x4014; i++)
				apu::reg_write(i, 0x00);
			apu::reg_write(0x4015, 0x00);
			apu::reg_write(0x4015, 0x0F);
			apu::reg_write(0x4017, 0x40);
			if (nf->banking)
				for (int i = 0; i < 8; i++)
					set_bank(i, nf->bank_init[i]);
			cpu::s = 0xFF;
			cpu::a = current_song;
			cpu::x = 0; // NTSC
			cpu::y = 0;
			SDL_Log("song %d", current_song);
		}
		if (addr == 0x41FF) {
			if ((ram_41FF = data)) {
				// NMI happened
				write_4016(1);
				write_4016(0);
				uint8_t input = 0;
				for (int i = 0; i < 8; i++)
					input |= (read_4016()&1) << i;
				input &= ~std::exchange(last_input, input);
				if (input & 0x40) {
					if (current_song != 0) {
						current_song--;
						bus_reset();
						cpu::exit_requested = true;
					}
				} else if (input & 0x80) {
					if (current_song != nf->total_songs-1) {
						current_song++;
						bus_reset();
						cpu::exit_requested = true;
					}
				}
			}
		}
	}
	NsfMapper(NsfFile &nf) :nf(&nf) {}
};
void mapper_setup(NsfFile& nf) {
	mapper_cleanup();
	mapper = new NsfMapper(nf);
}
}
