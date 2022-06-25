#include "cpudebug.hh"
#include <stdio.h>
#include "bus.hh"
#include "imgui.h"
namespace vhvc::cpudebug {
static const char* insnames[256] = {
	"BRK","ORA","JAM","SLO","NOP","ORA","ASL","SLO",
	"PHP","ORA","ASL","ANC","NOP","ORA","ASL","SLO",
	"BPL","ORA","JAM","SLO","NOP","ORA","ASL","SLO",
	"CLC","ORA","NOP","SLO","NOP","ORA","ASL","SLO",
	"JSR","AND","JAM","RLA","BIT","AND","ROL","RLA",
	"PLP","AND","ROL","ANC","BIT","AND","ROL","RLA",
	"BMI","AND","JAM","RLA","NOP","AND","ROL","RLA",
	"SEC","AND","NOP","RLA","NOP","AND","ROL","RLA",
	"RTI","EOR","JAM","SRE","NOP","EOR","LSR","SRE",
	"PHA","EOR","LSR","ALR","JMP","EOR","LSR","SRE",
	"BVC","EOR","JAM","SRE","NOP","EOR","LSR","SRE",
	"CLI","EOR","NOP","SRE","NOP","EOR","LSR","SRE",
	"RTS","ADC","JAM","RRA","NOP","ADC","ROR","RRA",
	"PLA","ADC","ROR","ARR","JMP","ADC","ROR","RRA",
	"BVS","ADC","JAM","RRA","NOP","ADC","ROR","RRA",
	"SEI","ADC","NOP","RRA","NOP","ADC","ROR","RRA",
	"NOP","STA","NOP","SAX","STY","STA","STX","SAX",
	"DEY","NOP","TXA","ANE","STY","STA","STX","SAX",
	"BCC","STA","JAM","SHA","STY","STA","STX","SAX",
	"TYA","STA","TXS","TAS","SHY","STA","SHX","SHA",
	"LDY","LDA","LDX","LAX","LDY","LDA","LDX","LAX",
	"TAY","LDA","TAX","LXA","LDY","LDA","LDX","LAX",
	"BCS","LDA","JAM","LAX","LDY","LDA","LDX","LAX",
	"CLV","LDA","TSX","LAS","LDY","LDA","LDX","LAX",
	"CPY","CMP","NOP","DCP","CPY","CMP","DEC","DCP",
	"INY","CMP","DEX","SBX","CPY","CMP","DEC","DCP",
	"BNE","CMP","JAM","DCP","NOP","CMP","DEC","DCP",
	"CLD","CMP","NOP","DCP","NOP","CMP","DEC","DCP",
	"CPX","SBC","NOP","ISC","CPX","SBC","INC","ISC",
	"INX","SBC","NOP","SBC","CPX","SBC","INC","ISC",
	"BEQ","SBC","JAM","ISC","NOP","SBC","INC","ISC",
	"SED","SBC","NOP","ISC","NOP","SBC","INC","ISC",
};
enum {
	IMP, A, IMM, ABS, JMP, ABX, ABY, ZP, ZPX, ZPY, IZX, IZY, REL, IND
};
static int mode_to_len[] = {
	1, 1, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 3
};
static char mode[256] = {
	IMP,IZX,IMP,IZX,ZP, ZP, ZP, ZP,
	IMP,IMM,A,  IMM,ABS,ABS,ABS,ABS,
	REL,IZY,IMP,IZY,ZPX,ZPX,ZPX,ZPX,
	IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	JMP,IZX,IMP,IZX,ZP, ZP, ZP, ZP,
	IMP,IMM,A,  IMM,ABS,ABS,ABS,ABS,
	REL,IZY,IMP,IZY,ZPX,ZPX,ZPX,ZPX,
	IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMP,IZX,IMP,IZX,ZP, ZP, ZP, ZP,
	IMP,IMM,A,  IMM,JMP,ABS,ABS,ABS,
	REL,IZY,IMP,IZY,ZPX,ZPX,ZPX,ZPX,
	IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMP,IZX,IMP,IZX,ZP, ZP, ZP, ZP,
	IMP,IMM,A,  IMM,IND,ABS,ABS,ABS,
	REL,IZY,IMP,IZY,ZPX,ZPX,ZPX,ZPX,
	IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMM,IZX,IMM,IZX,ZP, ZP, ZP, ZP,
	IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,
	REL,IZY,IMP,IZY,ZPX,ZPX,ZPY,ZPY,
	IMP,ABY,IMP,ABY,ABX,ABX,ABY,ABY,
	IMM,IZX,IMM,IZX,ZP, ZP, ZP, ZP,
	IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,
	REL,IZY,IMP,IZY,ZPX,ZPX,ZPY,ZPY,
	IMP,ABY,IMP,ABY,ABX,ABX,ABY,ABY,
	IMM,IZX,IMM,IZX,ZP, ZP, ZP, ZP,
	IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,
	REL,IZY,IMP,IZY,ZPX,ZPX,ZPX,ZPX,
	IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
	IMM,IZX,IMM,IZX,ZP, ZP, ZP, ZP,
	IMP,IMM,IMP,IMM,ABS,ABS,ABS,ABS,
	REL,IZY,IMP,IZY,ZPX,ZPX,ZPX,ZPX,
	IMP,ABY,IMP,ABY,ABX,ABX,ABX,ABX,
};
static constexpr bool is_illegal(uint8_t op) {
	return (op & 0x03) == 0x03 ||
		(op & 0xe7) == 0x04 ||
		(op & 0x96) == 0x12 ||
		(op & 0x8e) == 0x02 ||
		(op & 0x97) == 0x14 ||
		(op & 0xcf) == 0x44 ||
		(op & 0xfd) == 0x80 ||
		(op & 0xfd) == 0x89 ||
		(op & 0x1e) == 0x12 ||
		(op & 0x57) == 0x54 ||
		(op & 0x56) == 0x52 ||
		(op & 0x4e) == 0x42 ||
		(op & 0xfd) == 0x9c;
}
static const char* disassemble(uint16_t pc) {
	inspect_lock lk;

	static char buf[48];
	uint8_t ir = cpu_read(pc);
	int m = mode[ir];
	uint8_t b1 = 0, b2 = 0;
	uint16_t w = 0;
	char* p = buf;
	char il = is_illegal(ir) ? '*' : ' ';
	const char* insname = insnames[ir];
	if (!strcmp("ISC", insname))
		insname = "ISB";
	switch (mode_to_len[m]) {
	case 1:
		p += sprintf(buf, "%04X  %02X       %c%-3.3s ",     pc, ir,         il, insname);
		break;
	case 2:
		b1 = cpu_read(pc + 1);
		p += sprintf(buf, "%04X  %02X %02X    %c%-3.3s ",   pc, ir, b1,     il, insname);
		break;
	case 3:
		b1 = cpu_read(pc + 1);
		b2 = cpu_read(pc + 2);
		w = b2 << 8 | b1;
		p += sprintf(buf, "%04X  %02X %02X %02X %c%-3.3s ", pc, ir, b1, b2, il, insname);
		break;
	}

	switch (m) {
	case IMP: break;
	case A:   sprintf(p, "A"); break;
	case IMM: sprintf(p, "#$%02X", b1); break;
	case ABS: sprintf(p, "$%04X = %02X", w, cpu_read(w)); break;
	case JMP: sprintf(p, "$%04X", w); break;
	case ABX: sprintf(p, "$%04X,X @ %04X = %02X", w, uint16_t(w + cpu::x), cpu_read(w + cpu::x)); break;
	case ABY: sprintf(p, "$%04X,Y @ %04X = %02X", w, uint16_t(w + cpu::y), cpu_read(w + cpu::y)); break;
	case ZP:  sprintf(p, "$%02X = %02X", b1, cpu_read(b1)); break;
	case ZPX: sprintf(p, "$%02X,X @ %02X = %02X", b1, uint8_t(b1 + cpu::x), cpu_read(uint8_t(b1 + cpu::x))); break;
	case ZPY: sprintf(p, "$%02X,Y @ %02X = %02X", b1, uint8_t(b1 + cpu::y), cpu_read(uint8_t(b1 + cpu::y))); break;
	case IZX:
		w = cpu_read(uint8_t(b1 + cpu::x + 1)) << 8 | cpu_read(uint8_t(b1 + cpu::x));
		sprintf(p, "($%02X,X) @ %02X = %04X = %02X", b1, uint8_t(b1 + cpu::x), w, cpu_read(w));
		break;
	case IZY:
		w = cpu_read(uint8_t(b1 + 1)) << 8 | cpu_read(b1);
		sprintf(p, "($%02X),Y = %04X @ %04X = %02X", b1, w, uint16_t(w + cpu::y), cpu_read(w + cpu::y));
		break;
	case REL: sprintf(p, "$%04X", pc + 2 + int8_t(b1)); break;
	case IND: sprintf(p, "($%04X) = %02X%02X", w, cpu_read(w&0xFF00 | uint8_t(w + 1)), cpu_read(w)); break;
	}
	return buf;
}
static int cycles = 0;
bool is_debugging = false;
bool nestest = false;
bool log_intr = false;
void on_insn() {
	if (nestest) {
		printf("%-47s A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%d\n",
			disassemble(cpu::pc),
			cpu::a,
			cpu::x,
			cpu::y,
			cpu::get_flags(),
			cpu::s,
			cycles * 3 / 341 % 262,
			cycles * 3 % 341,
			cycles
		);
	}
}
void on_cycle() {
	if (nestest)
		cycles++;
}
void on_intr() {
	if (log_intr) {
		printf("INTR:%s%s%s%s%s%s%s\n",
			cpu::irq ? " IRQ (" : "",
			cpu::irq & IRQ_FRAMECOUNTER ? " FrameCounter" : "",
			cpu::irq & IRQ_DMC ? " DMC" : "",
			cpu::irq & IRQ_MAPPER ? " Mapper" : "",
			cpu::irq ? " )" : "",
			cpu::nmi ? " NMI" : "",
			cpu::resetting ? " RESET" : "");
	}
}

bool show_cpu_state = false;
bool show_cpu_memory = false;
void gui() {
	if (show_cpu_state) {
		if (ImGui::Begin("CPU State", &show_cpu_state)) {
			ImGui::Text("A:%02X X:%02X Y:%02X S:%02X P:%02X PC:%04X", cpu::a, cpu::x, cpu::y, cpu::s, cpu::get_flags(), cpu::pc);
			if (cpu::jammed)
				ImGui::Text("[JAMMED]");
		}
		ImGui::End();
	}
	if (show_cpu_memory) {
		inspect_lock lk;
		if (ImGui::Begin("CPU Memory", &show_cpu_memory)) {
			for (int i = 0; i < 0x800; i += 0x10) {
				ImGui::Text("%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
					i,
					cpu_ram[i], cpu_ram[i+1], cpu_ram[i+2], cpu_ram[i+3],
					cpu_ram[i+4], cpu_ram[i+5], cpu_ram[i+6], cpu_ram[i+7],
					cpu_ram[i+8], cpu_ram[i+9], cpu_ram[i+10], cpu_ram[i+11],
					cpu_ram[i+12], cpu_ram[i+13], cpu_ram[i+14], cpu_ram[i+15]);
			}
		}
		ImGui::End();
	}
}
}
