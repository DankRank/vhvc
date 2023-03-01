#include "cpu.hh"
#include "cpudebug.hh"
#include "bus.hh"
namespace vhvc::cpu {
uint8_t a = 0;
uint8_t x = 0;
uint8_t y = 0;
uint8_t s = 0;
uint16_t pc = 0;
uint8_t data_bus = 0;
uint8_t ane_magic = 0xEE;
uint8_t lax_magic = 0xEE;
bool rdy_happened = false;
bool jammed = false;
bool nmi_line = false;
bool nmi = false;
unsigned irq = 0;
bool resetting = false;
bool interrupt_pending = false;
bool C = false;
bool Z = false;
bool I = false;
bool D = false;
bool V = false;
bool N = false;
bool exit_requested = false;
void state(IState &st) {
	st
		<"a"> a
		<"x"> x
		<"y"> y
		<"s"> s
		<"pc"> pc;
}
uint8_t get_flags() {
	return int(C) | int(Z) << 1 | int(I) << 2 | int(D) << 3 | 1 << 5 | int(V) << 6 | int(N) << 7;
}
void set_flags(uint8_t f) {
	C = f & 1;
	Z = f & 2;
	I = f & 4;
	D = f & 8;
	V = f & 64;
	N = f & 128;
}
void set_nmi(bool state) {
	if (state && !nmi_line)
		nmi = true;
	nmi_line = state;
}
void poweron() {
	a = x = y = s = 0;
	C = Z = D = V = N = false;
	pc = 0; // set pc to some sane value for first two accesses
	reset();
}
void reset() {
	I = true;
	nmi = false;
	resetting = true;
	jammed = false;
}

#define READ(addr) (data_bus = cpu_read((addr)))
#define WRITE(addr, data) cpu_write((addr), (data_bus = (data)))
#define SETNZ(value) (Z = !value, N = value&0x80)
#define CIRQ() (interrupt_pending = !I && irq || nmi)
void step(int steps) {
	if (jammed) {
jam:
		while (!resetting && steps--)
			READ(0xFFFF);
		return;
	}
	uint16_t tmp;
	uint8_t b2;
	uint8_t val;
	exit_requested = false;
	while (steps-- && !exit_requested) {
		if (cpudebug::is_debugging)
			cpudebug::on_insn();
		uint8_t ir, b1;
		if (interrupt_pending && (irq || nmi) || resetting) {
			if (cpudebug::is_debugging)
				cpudebug::on_intr();
			READ(pc);
			ir = 0;
			b1 = READ(pc);
		} else {
			ir = READ(pc);
			// this selects opcodes which only have two cycles (incl branches and JAM)
			if ((ir & 0x1D) == 0x09 || !(ir & 0x05) && ir & 0x92)
				CIRQ();
			b1 = READ(++pc);
		}
#define RMW(addr, act) val = READ(addr); WRITE(addr, val); act; CIRQ(); WRITE(addr, val);
#define LOAD_IMM()  do { pc++; val = b1; } while (0)
#define LOAD_ZP()   do { pc++; CIRQ(); val = READ(b1); } while (0)
#define STORE_ZP()  do { pc++; CIRQ(); WRITE(b1, val); } while (0)
#define RMW_ZP(act) do { pc++;         RMW(b1, act);   } while (0)
#define LOAD_ZPi(index)     do { pc++; READ(b1); CIRQ(); val = READ(uint8_t(b1 + index)); } while (0)
#define STORE_ZPi(index)    do { pc++; READ(b1); CIRQ(); WRITE(uint8_t(b1 + index), val); } while (0)
#define RMW_ZPi(index, act) do { pc++; READ(b1);         RMW(uint8_t(b1 + index), act); } while (0)
#define LOAD_ABS()   do { pc++; b2 = READ(pc++); CIRQ(); val = READ(b2 << 8 | b1); } while (0)
#define STORE_ABS()  do { pc++; b2 = READ(pc++); CIRQ(); WRITE(b2 << 8 | b1, val); } while (0)
#define RMW_ABS(act) do { pc++; b2 = READ(pc++);         RMW(b2 << 8 | b1, act);   } while (0)
#define LOADSTORE_ABi(index, w) do { \
	pc++; \
	b2 = READ(pc++); \
	if (w || b1 + index > 0xFF) \
		READ(b2 << 8 | uint8_t(b1 + index)); \
} while (0)
#define UNSTABLESTORE(index) do { \
	rdy_happened = false; \
	READ(b2 << 8 | uint8_t(b1 + index)); \
	tmp = rdy_happened ? val : val & (b2+1); \
	CIRQ(); \
	if (b1 + index > 0xFF) \
		WRITE((b2 << 8 | b1) + index & (val<<8|0xFF), tmp); \
	else \
		WRITE((b2 << 8 | b1) + index,                 tmp); \
} while (0)
#define UNSTABLESTORE_ABi(index) do { \
	pc++; \
	b2 = READ(pc++); \
	UNSTABLESTORE(index); \
} while (0)
#define LOAD_ABi(index)     do{ LOADSTORE_ABi(index, false); CIRQ(); val = READ((b2 << 8 | b1) + index); } while (0)
#define STORE_ABi(index)    do{ LOADSTORE_ABi(index, true);  CIRQ(); WRITE((b2 << 8 | b1) + index, val); } while (0)
#define RMW_ABi(index, act) do{ LOADSTORE_ABi(index, true);          RMW((b2 << 8 | b1) + index, act);   } while (0)
#define LOADSTORE_IZX() do { \
	pc++; \
	READ(b1); \
	tmp = READ(uint8_t(b1 + x)); \
	tmp |= READ(uint8_t(b1 + x + 1)) << 8; \
} while (0)
#define LOADSTORE_IZY(w) do { \
	pc++; \
	tmp = READ(b1); \
	tmp |= READ(uint8_t(b1 + 1)) << 8; \
	if (w || (tmp&0xFF)+y > 0xFF) \
		READ(tmp&0xFF00 | (tmp+y)&0xFF); \
	tmp += y; \
} while (0)
#define UNSTABLESTORE_IZY() do { \
	pc++; \
	tmp = READ(b1); \
	b2 = READ(uint8_t(b1 + 1)); \
	b1 = tmp; \
	UNSTABLESTORE(y); \
} while (0)
#define LOAD_IZX()   do { LOADSTORE_IZX(); CIRQ(); val = READ(tmp); } while (0)
#define STORE_IZX()  do { LOADSTORE_IZX(); CIRQ(); WRITE(tmp, val); } while (0)
#define RMW_IZX(act) do { LOADSTORE_IZX();         RMW(tmp, act);   } while (0)
#define LOAD_IZY()   do { LOADSTORE_IZY(false); CIRQ(); val = READ(tmp); } while (0)
#define STORE_IZY()  do { LOADSTORE_IZY(true);  CIRQ(); WRITE(tmp, val); } while (0)
#define RMW_IZY(act) do { LOADSTORE_IZY(true);          RMW(tmp, act);   } while (0)
#define LOAD_ALL(base, operation) \
	case base | 0x01: LOAD_IZX();  operation; break; \
	case base | 0x05: LOAD_ZP();   operation; break; \
	case base | 0x09: LOAD_IMM();  operation; break; \
	case base | 0x0D: LOAD_ABS();  operation; break; \
	case base | 0x11: LOAD_IZY();  operation; break; \
	case base | 0x15: LOAD_ZPi(x); operation; break; \
	case base | 0x19: LOAD_ABi(y); operation; break; \
	case base | 0x1D: LOAD_ABi(x); operation; break
#define RMW_ALL(base, act, operation) \
	case base | 0x03:                   RMW_IZX(act);                  operation;   break; \
	case base | 0x07: case base | 0x06: RMW_ZP(act);     if (ir & 1) { operation; } break; \
	case base | 0x0F: case base | 0x0E: RMW_ABS(act);    if (ir & 1) { operation; } break; \
	case base | 0x13:                   RMW_IZY(act);                  operation;   break; \
	case base | 0x17: case base | 0x16: RMW_ZPi(x, act); if (ir & 1) { operation; } break; \
	case base | 0x1B:                   RMW_ABi(y, act);               operation;   break; \
	case base | 0x1F: case base | 0x1E: RMW_ABi(x, act); if (ir & 1) { operation; } break

#define FIXUP_READ(i, j) if ((i+j ^ i) & 0x100) READ(i&0xFF00 | (i+j)&0xFF)
#define BRANCH(cond) do { \
	pc++; \
	if (cond) { \
		READ(pc); \
		if ((pc+int8_t(b1) ^ pc) & 0x100) { \
			READ(pc&0xFF00 | (pc+int8_t(b1))&0xFF); \
			CIRQ(); \
		} else { \
			interrupt_pending = false; /* FIXME: this is almost certainly wrong */ \
			CIRQ(); \
		} \
		pc += int8_t(b1); \
	} \
} while(0)
#define ADDER(i, j, k) do { \
	tmp = i + j + (int)k; \
	C = tmp & 0x100; \
	tmp &= 0xFF; \
	SETNZ(tmp); \
} while (0)
#define SETV(i, j) do { \
	V = (tmp^i)&(tmp^j)&0x80; \
} while (0)
#define ADC()  do {              ADDER(a,   val, C); SETV(a, val); a = tmp; } while (0)
#define SBC()  do { val ^= 0xFF; ADDER(a,   val, C); SETV(a, val); a = tmp; } while (0)
#define SBX()  do { val ^= 0xFF; ADDER(a&x, val, 1);               x = tmp; } while (0)
#define CMP(i) do { val ^= 0xFF; ADDER(i,   val, 1);                        } while (0)
#define BIT()  do { N = val & 0x80; V = val & 0x40; Z = !(a & val); } while (0)
#define ASL(i) do {               C = i&0x80; i <<= 1;               SETNZ(i); } while (0)
#define LSR(i) do {               C = i&0x01; i >>= 1;               SETNZ(i); } while (0)
#define ROL(i) do { int oldc = C; C = i&0x80; i <<= 1; i |= oldc;    SETNZ(i); } while (0)
#define ROR(i) do { int oldc = C; C = i&0x01; i >>= 1; i |= oldc<<7; SETNZ(i); } while (0)
#define DEC(i) do { i--; SETNZ(i); } while (0)
#define INC(i) do { i++; SETNZ(i); } while (0)
#define ORA()  do { a |= val; SETNZ(a); } while (0)
#define AND()  do { a &= val; SETNZ(a); } while (0)
#define EOR()  do { a ^= val; SETNZ(a); } while (0)
#define LDi(i) do { i  = val; SETNZ(i); } while (0)
		switch (ir) {
		LOAD_ALL(0x00, ORA());  // ORA 0x01 0x05 0x09 0x0D 0x11 0x15 0x19 0x1D
		LOAD_ALL(0x20, AND());  // AND 0x21 0x25 0x29 0x2D 0x31 0x35 0x39 0x3D
		LOAD_ALL(0x40, EOR());  // EOR 0x41 0x45 0x49 0x4D 0x51 0x55 0x59 0x5D
		LOAD_ALL(0x60, ADC());  // ADC 0x61 0x65 0x69 0x6D 0x71 0x75 0x79 0x7D
		LOAD_ALL(0xA0, LDi(a)); // LDA 0xA1 0xA5 0xA9 0xAD 0xB1 0xB5 0xB9 0xBD
		LOAD_ALL(0xC0, CMP(a)); // CMP 0xC1 0xC5 0xC9 0xCD 0xD1 0xD5 0xD9 0xDD
		LOAD_ALL(0xE0, SBC());  // SBC 0xE1 0xE5 0xE9 0xED 0xF1 0xF5 0xF9 0xFD
		case 0xA0: LOAD_IMM();  LDi(y); break; // LDY imm
		case 0xA4: LOAD_ZP();   LDi(y); break; // LDY zp
		case 0xAC: LOAD_ABS();  LDi(y); break; // LDY abs
		case 0xB4: LOAD_ZPi(x); LDi(y); break; // LDY zpx
		case 0xBC: LOAD_ABi(x); LDi(y); break; // LDY abx
		case 0xA2: LOAD_IMM();  LDi(x); break; // LDX imm
		case 0xA6: LOAD_ZP();   LDi(x); break; // LDX zp
		case 0xAE: LOAD_ABS();  LDi(x); break; // LDX abs
		case 0xB6: LOAD_ZPi(y); LDi(x); break; // LDX zpy
		case 0xBE: LOAD_ABi(y); LDi(x); break; // LDX aby
		case 0xC0: LOAD_IMM();  CMP(y); break; // CPY imm
		case 0xC4: LOAD_ZP();   CMP(y); break; // CPY zp
		case 0xCC: LOAD_ABS();  CMP(y); break; // CPY abs
		case 0xE0: LOAD_IMM();  CMP(x); break; // CPX imm
		case 0xE4: LOAD_ZP();   CMP(x); break; // CPX zp
		case 0xEC: LOAD_ABS();  CMP(x); break; // CPX abs
		case 0x24: LOAD_ZP();   BIT();  break; // BIT zp
		case 0x2C: LOAD_ABS();  BIT();  break; // BIT abs
		case 0xEA: // NOP (official)
		case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: // NOP
			break;
		case 0x80: case 0x82: case 0xC2: case 0xE2: case 0x89: // NOP imm
			LOAD_IMM(); break;
		case 0x04: case 0x44: case 0x64: // NOP zp
			LOAD_ZP(); break;
		case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4: // NOP zp, x
			LOAD_ZPi(x); break;
		case 0x0C: // NOP abs
			LOAD_ABS(); break;
		case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: // NOP abs, x
			LOAD_ABi(x); break;

		case 0x0B: [[fallthrough]]; // ANC #imm
		case 0x2B: LOAD_IMM();  a &= val; SETNZ(a); C = N; break; // ANC #imm
		case 0x4B: LOAD_IMM();  a &= val; LSR(a); break; // ALR imm
		case 0x6B: LOAD_IMM();  a &= val; a = int(C) << 7 | a >> 1; C = a & 0x40; V = (a>>1 ^ a) & 0x20; SETNZ(a); break; // ARR imm
		case 0xCB: LOAD_IMM();  SBX(); break; // SBX imm
		case 0xEB: LOAD_IMM();  SBC(); break; // SBC imm (unofficial)
		case 0xBB: LOAD_ABi(y); a = x = s = val & s; SETNZ(a); break; // LAS aby

		case 0xA3: LOAD_IZX();  a = x = val; SETNZ(a); break; // LAX izx
		case 0xA7: LOAD_ZP();   a = x = val; SETNZ(a); break; // LAX zp
		case 0xAF: LOAD_ABS();  a = x = val; SETNZ(a); break; // LAX abs
		case 0xB3: LOAD_IZY();  a = x = val; SETNZ(a); break; // LAX izy
		case 0xB7: LOAD_ZPi(y); a = x = val; SETNZ(a); break; // LAX zpy
		case 0xBF: LOAD_ABi(y); a = x = val; SETNZ(a); break; // LAX aby

		case 0x84: val = y; STORE_ZP();   break; // STY zp
		case 0x8C: val = y; STORE_ABS();  break; // STY abs
		case 0x94: val = y; STORE_ZPi(x); break; // STY zpx
		case 0x81: val = a; STORE_IZX();  break; // STA izx
		case 0x85: val = a; STORE_ZP();   break; // STA zp
		case 0x8D: val = a; STORE_ABS();  break; // STA abs
		case 0x91: val = a; STORE_IZY();  break; // STA izy
		case 0x95: val = a; STORE_ZPi(x); break; // STA zpx
		case 0x99: val = a; STORE_ABi(y); break; // STA aby
		case 0x9D: val = a; STORE_ABi(x); break; // STA abx
		case 0x86: val = x; STORE_ZP();   break; // STX zp
		case 0x8E: val = x; STORE_ABS();  break; // STX abs
		case 0x96: val = x; STORE_ZPi(y); break; // STX zpy
		case 0x83: val = a & x; STORE_IZX();  break; // SAX izx
		case 0x87: val = a & x; STORE_ZP();   break; // SAX zp
		case 0x8F: val = a & x; STORE_ABS();  break; // SAX abs
		case 0x97: val = a & x; STORE_ZPi(y); break; // SAX zpy

		case 0x93: val = a & x;          UNSTABLESTORE_IZY();  break; // SHA izy
		case 0x9F: val = a & x;          UNSTABLESTORE_ABi(y); break; // SHA aby
		case 0x9B: val = a & x; s = val; UNSTABLESTORE_ABi(y); break; // TAS aby
		case 0x9C: val = y;              UNSTABLESTORE_ABi(x); break; // SHY abx
		case 0x9E: val = x;              UNSTABLESTORE_ABi(y); break; // SHX aby

		// unstable ops
		case 0x8B: SETNZ(a); a = (a | ane_magic) & x & b1; break; // ANE imm
		case 0xAB: SETNZ(a); a = x = (a | lax_magic) & b1; break; // LAX imm

		RMW_ALL(0x00, ASL(val), ORA());  // ASL 0x06 0x0E 0x16 0x1E SLO 0x03 0x07 0x0F 0x13 0x17 0x1B 0x1F
		RMW_ALL(0x20, ROL(val), AND());  // ROL 0x26 0x2E 0x36 0x3E RLA 0x23 0x27 0x2F 0x33 0x37 0x3B 0x3F
		RMW_ALL(0x40, LSR(val), EOR());  // LSR 0x46 0x4E 0x56 0x5E SRE 0x43 0x47 0x4F 0x53 0x57 0x5B 0x5F
		RMW_ALL(0x60, ROR(val), ADC());  // ROR 0x66 0x6E 0x76 0x7E RRA 0x63 0x67 0x6F 0x73 0x77 0x7B 0x7F
		RMW_ALL(0xC0, DEC(val), CMP(a)); // DEC 0xC6 0xCE 0xD6 0xDE DCP 0xC3 0xC7 0xCF 0xD3 0xD7 0xDB 0xDF
		RMW_ALL(0xE0, INC(val), SBC());  // INC 0xE6 0xEE 0xF6 0xFE ISC 0xE3 0xE7 0xEF 0xF3 0xF7 0xFB 0xFF
		case 0xC8: INC(y); break; // INY
		case 0x88: DEC(y); break; // DEY
		case 0xE8: INC(x); break; // INX
		case 0xCA: DEC(x); break; // DEX
		case 0x0A: ASL(a); break; // ASL a
		case 0x2A: ROL(a); break; // ROL a
		case 0x4A: LSR(a); break; // LSR a
		case 0x6A: ROR(a); break; // ROR a

		case 0x18: C = false; break; // CLC
		case 0x38: C = true;  break; // SEC
		case 0x58: I = false; break; // CLI
		case 0x78: I = true;  break; // SEI
		case 0xB8: V = false; break; // CLV
		case 0xD8: D = false; break; // CLD
		case 0xF8: D = true;  break; // SED

		case 0x8A: a = x; SETNZ(a); break; // TXA
		case 0x98: a = y; SETNZ(a); break; // TYA
		case 0x9A: s = x;           break; // TXS
		case 0xA8: y = a; SETNZ(y); break; // TAY
		case 0xAA: x = a; SETNZ(x); break; // TAX
		case 0xBA: x = s; SETNZ(x); break; // TSX

		case 0x10: BRANCH(!N); break; // BPL
		case 0x30: BRANCH(N);  break; // BMI
		case 0x50: BRANCH(!V); break; // BVC
		case 0x70: BRANCH(V);  break; // BVS
		case 0x90: BRANCH(!C); break; // BCC
		case 0xB0: BRANCH(C);  break; // BCS
		case 0xD0: BRANCH(!Z); break; // BNE
		case 0xF0: BRANCH(Z);  break; // BEQ
		case 0x4C: // JMP abs
			CIRQ();
			b2 = READ(++pc);
			pc = b2 << 8 | b1;
			break;
		case 0x6C: // JMP ind
			b2 = READ(++pc);
			tmp = READ(b2 << 8 | b1);
			CIRQ();
			pc = READ(b2 << 8 | uint8_t(b1+1)) << 8 | tmp;
			break;
		case 0x20: // JSR
			READ(0x100 | s);
			pc++;
			WRITE(0x100 | s--, pc >> 8);
			WRITE(0x100 | s--, pc);
			CIRQ();
			pc = READ(pc) << 8 | b1;
			break;
		case 0x60: // RTS
			READ(0x100 | s++);
			b1 = READ(0x100 | s++);
			b2 = READ(0x100 | s);
			pc = b2 << 8 | b1;
			CIRQ();
			READ(pc++);
			break;
		case 0x40: // RTI
			READ(0x100 | s++);
			set_flags(READ(0x100 | s++));
			b1 = READ(0x100 | s++);
			CIRQ();
			b2 = READ(0x100 | s);
			pc = b2 << 8 | b1;
			break;
		case 0x08: CIRQ(); WRITE(0x100 | s--, get_flags() | B_FLAG); break; // PHP
		case 0x48: CIRQ(); WRITE(0x100 | s--, a);                    break; // PHA
		case 0x28: READ(0x100 | s++); CIRQ(); set_flags(READ(0x100 | s));    break; // PLP
		case 0x68: READ(0x100 | s++); CIRQ(); a = READ(0x100 | s); SETNZ(a); break; // PLA
		case 0x00: { // BRK
			if (!interrupt_pending)
				pc++;
			else
				interrupt_pending = false;
			if (resetting) {
				READ(0x100 | s--);
				READ(0x100 | s--);
				READ(0x100 | s--);
			} else {
				WRITE(0x100 | s--, pc >> 8);
				WRITE(0x100 | s--, pc);
				WRITE(0x100 | s--, get_flags() | (!irq && !nmi ? B_FLAG : 0));
			}
			uint16_t vector = resetting ? 0xFFFC : nmi ? 0xFFFA : 0xFFFE;
			b1 = READ(vector);
			CIRQ();
			b2 = READ(vector + 1);
			pc = b2 << 8 | b1;
			if (resetting || nmi || irq) {
				I = true;
				// Here we must check immediately after setting I to avoid double IRQ
				// FIXME: perhaps this is a sign that the I flag should be updated earlier?
				CIRQ();
			}
			if (resetting)
				resetting = false;
			if (nmi)
				nmi = false;
			break;
		}
		// JAM
		case 0x02: case 0x12: case 0x22: case 0x32:
		case 0x42: case 0x52: case 0x62: case 0x72:
		case 0x92: case 0xB2: case 0xD2: case 0xF2:
			jammed = true;
			goto jam;
		}
	}
}
}
