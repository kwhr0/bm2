// HD6303
// Copyright 2022,2023 © Yasuo Kuwahara
// MIT License

#include "HD6303.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	LC, LV, LZ, LN, LI, LH
};

enum {
	MC = 1 << LC, MV = 1 << LV, MZ = 1 << LZ, MN = 1 << LN, MI = 1 << LI, MH = 1 << LH
};

enum {
	FB = 1, F0, F1, F8, F16, FADD, FADD16, FSUB, FSUB16, FLEFT, FLEFT16, FRIGHT, FRIGHT16, FDAA
};

#define F(flag, type)	flag##type = F##type << (L##flag << 2)
enum {
	F(C, B), F(C, 0), F(C, 1), F(C, ADD), F(C, ADD16), F(C, SUB), F(C, SUB16),
	F(C, LEFT), F(C, LEFT16), F(C, RIGHT), F(C, RIGHT16), F(C, DAA),
	F(V, B), F(V, 0), F(V, 1), F(V, ADD), F(V, ADD16), F(V, SUB), F(V, SUB16),
	F(V, LEFT), F(V, LEFT16), F(V, RIGHT), F(V, RIGHT16),
	F(Z, B), F(Z, 1), F(Z, 8), F(Z, 16),
	F(N, B), F(N, 0), F(N, 8), F(N, 16),
	F(H, B), F(H, ADD)
};

#define fmnt()			(++fp < fbuf + FBUFMAX ? 0 : ResolvFlags())
#define fclc()			(fp->dm = C0, fmnt())
#define fsec()			(fp->dm = C1, fmnt())
#define fclv()			(fp->dm = V0, fmnt())
#define fsev()			(fp->dm = V1, fmnt())
#define fclr()			(fp->dm = N0 | Z1 | V0 | C0, fmnt())
#define fld(x)			(fp->dm = N8 | Z8 | V0, fp->a = (x), fmnt())
#define fld16(x)		(fp->dm = N16 | Z16 | V0, fp->a = (x), fmnt())
#define ftst(x)			(fp->dm = N8 | Z8 | V0 | C0, fp->a = (x), fmnt())
#define fcom(x)			(fp->dm = N8 | Z8 | V0 | C1, fp->a = (x), fmnt())
#define fadd(x, y, z)	(fp->dm = HADD | N8 | Z8 | VADD | CADD, fp->b = (x), fp->s = (y), fp->a = (z), fmnt())
#define fadd16(x, y, z)	(fp->dm = N16 | Z16 | VADD16 | CADD16, fp->b = (x), fp->s = (y), fp->a = (z), fmnt())
#define fsub(x, y, z)	(fp->dm = N8 | Z8 | VSUB | CSUB, fp->b = (x), fp->s = (y), fp->a = (z), fmnt())
#define fsub16(x, y, z)	(fp->dm = N16 | Z16 | VSUB16 | CSUB16, fp->b = (x), fp->s = (y), fp->a = (z), fmnt())
#define fcpx(x, y, z)	(fp->dm = N16 | Z16 | VSUB16, fp->b = (x), fp->s = (y), fp->a = (z), fmnt())
#define finc(x, y)		(fp->dm = N8 | Z8 | VADD, fp->b = (x), fp->s = 1, fp->a = (y), fmnt())
#define fdec(x, y)		(fp->dm = N8 | Z8 | VSUB, fp->b = (x), fp->s = 1, fp->a = (y), fmnt())
#define fz16(x)			(fp->dm = Z16, fp->a = (x), fmnt())
#define fleft(x, y)		(fp->dm = N8 | Z8 | VLEFT | CLEFT, fp->b = (x), fp->a = (y), fmnt())
#define fleft16(x, y)	(fp->dm = N16 | Z16 | VLEFT16 | CLEFT16, fp->b = (x), fp->a = (y), fmnt())
#define fright(x, y)	(fp->dm = N8 | Z8 | VRIGHT | CRIGHT, fp->b = (x), fp->a = (y), fmnt())
#define fright16(x, y)	(fp->dm = N16 | Z16 | VRIGHT16 | CRIGHT16, fp->b = (x), fp->a = (y), fmnt())
#define fdaa(x, y)		(fp->dm = N8 | Z8 | CDAA, fp->s = (x), fp->a = (y), fmnt())

#define CY				(ResolvC())

#define A	acc[1]
#define B	acc[0]
#define D	((uint16_t &)acc[0])

enum { W_WAI = 1, W_SLEEP };

static void error() {
	fprintf(stderr, "internal error\n");
	exit(1);
}

HD6303::HD6303() {
#if HD6303_TRACE
	memset(tracebuf, 0, sizeof(tracebuf));
	tracep = tracebuf;
#endif
}

void HD6303::Reset() {
	irq = waitflags = 0;
	D = 0xffff;
	SetupFlags(MI);
	sp = ix = 0xffff;
	pc = ld16(0xfffe);
}

int HD6303::Execute(int n) {
	auto lda = [&](uint8_t d) { fld(A = d); };
	auto ldb = [&](uint8_t d) { fld(B = d); };
	auto ldd = [&](uint16_t d) { fld16(D = d); };
	auto lds = [&](uint16_t d) { fld16(sp = d); };
	auto ldx = [&](uint16_t d) { fld16(ix = d); };
	auto sta = [&] { fld(A); return A; };
	auto stb = [&] { fld(B); return B; };
	auto std = [&] { fld16(D); return D; };
	auto sts = [&] { fld16(sp); return sp; };
	auto stx = [&] { fld16(ix); return ix; };
	auto clr = [&] { fclr(); return 0; };
	auto adda = [&](uint8_t d) { fadd(A, d, A += d); };
	auto addb = [&](uint8_t d) { fadd(B, d, B += d); };
	auto addd = [&](uint16_t d) { fadd16(D, d, D += d); };
	auto adca = [&](uint8_t d) { fadd(A, d, A += d + CY); };
	auto adcb = [&](uint8_t d) { fadd(B, d, B += d + CY); };
	auto suba = [&](uint8_t d) { fsub(A, d, A -= d); };
	auto subb = [&](uint8_t d) { fsub(B, d, B -= d); };
	auto subd = [&](uint16_t d) { fsub16(D, d, D -= d); };
	auto sbca = [&](uint8_t d) { fsub(A, d, A -= d + CY); };
	auto sbcb = [&](uint8_t d) { fsub(B, d, B -= d + CY); };
	auto cmpa = [&](uint8_t d) { fsub(A, d, A - d); };
	auto cmpb = [&](uint8_t d) { fsub(B, d, B - d); };
	auto cpx = [&](uint16_t d) { fcpx(ix, d, ix - d); };
	auto inc = [&](uint8_t d) { finc(d, ++d); return d; };
	auto dec = [&](uint8_t d) { fdec(d, --d); return d; };
	auto anda = [&](uint8_t d) { fld(A &= d); };
	auto andb = [&](uint8_t d) { fld(B &= d); };
	auto aim = [&](uint8_t d, uint8_t imm) { fld(d &= imm); return d; };
	auto ora = [&](uint8_t d) { fld(A |= d); };
	auto orb = [&](uint8_t d) { fld(B |= d); };
	auto oim = [&](uint8_t d, uint8_t imm) { fld(d |= imm); return d; };
	auto eora = [&](uint8_t d) { fld(A ^= d); };
	auto eorb = [&](uint8_t d) { fld(B ^= d); };
	auto eim = [&](uint8_t d, uint8_t imm) { fld(d ^= imm); return d; };
	auto bita = [&](uint8_t d) { fld(A & d); };
	auto bitb = [&](uint8_t d) { fld(B & d); };
	auto tim = [&](uint8_t d, uint8_t imm) { fld(d & imm); };
	auto tst = [&](uint8_t d) { ftst(d); };
	auto com = [&](uint8_t d) { fcom(d = ~d); return d; };
	auto neg = [&](uint8_t d) { fsub(0, d, d = -d); return d; };
	auto asl = [&](uint8_t d) { fleft(d, d <<= 1); return d; };
	auto rol = [&](uint8_t d) { fleft(d, d = d << 1 | CY); return d; };
	auto asr = [&](int8_t d) { fright(d, d >>= 1); return d; };
	auto lsr = [&](uint8_t d) { fright(d, d >>= 1); return d; };
	auto ror = [&](uint8_t d) { fright(d, d = d >> 1 | CY << 7); return d; };
	auto br = [&](uint8_t cond) { pc += cond ? (int8_t)imm8() : 1; clock += 3; };
	auto pshs = [&] {
		st16r(--sp, pc); st16r(sp -= 2, ix); st8(--sp, A); st8(--sp, B); st8(--sp, ResolvFlags()); --sp;
	};
	auto puls = [&] {
		SetupFlags(ld8(++sp)); B = ld8(++sp); A = ld8(++sp); ix = ld16(++sp); pc = ld16(sp += 2); ++sp;
	};
	auto daa = [&] {
		int u = A >> 4, l = A & 0xf, c = CY, h = ResolvH();
		int d = c ? h ? u <= 3 && l <= 3 ? 0x66 : 0 : u <= 2 ? l <= 9 ? 0x60 : 0x66 : 0 :
			h ? l <= 3 ? u <= 9 ? 6 : 0x66 : 0 : l <= 9 ? u <= 9 ? 0 : 0x60 : u <= 8 ? 6 : 0x66;
		fdaa(d, A += d);
	};
	clock = 0;
	do {
		if (irq) {
			if (irq & M_NMI) {
				irq &= ~M_NMI;
				pshs();
				intflags |= MI;
				waitflags = 0;
				pc = ld16(0xfffc);
			}
			else if (irq & M_IRQ && !(intflags & MI)) {
				irq &= ~M_IRQ;
				pshs();
				intflags |= MI;
				waitflags = 0;
				pc = ld16(0xfff8);
			}
		}
		if (waitflags) return 0;
#if HD6303_TRACE
		tracep->pc = pc;
		tracep->index = tracep->opn = 0;
#endif
		uint8_t t8;
		uint16_t t16;
		switch (imm8()) {
			case 0x01: clock++; break;
			case 0x04: fright16(D, D >> 1); D >>= 1; clock++; break;
			case 0x05: fleft16(D, D << 1); D <<= 1; clock++; break;
			case 0x06: SetupFlags(A); clock++; break;
			case 0x07: A = ResolvFlags(); clock++; break;
			case 0x08: fz16(++ix); clock++; break;
			case 0x09: fz16(--ix); clock++; break;
			case 0x0a: fclv(); clock++; break;
			case 0x0b: fsev(); clock++; break;
			case 0x0c: fclc(); clock++; break;
			case 0x0d: fsec(); clock++; break;
			case 0x0e: intflags &= ~MI; clock++; break;
			case 0x0f: intflags |= MI; clock++; break;
			case 0x10: fsub(A, B, A -= B); clock++; break;
			case 0x11: fsub(A, B, A - B); clock++; break;
			case 0x16: fld(B = A); clock++; break;
			case 0x17: fld(A = B); clock++; break;
			case 0x18: t16 = D; D = ix; ix = t16; clock += 2; break;
			case 0x19: daa(); clock += 2; break;
			case 0x1a: waitflags |= W_SLEEP; clock += 4; break;
			case 0x1b: fadd(A, B, A += B); clock++; break;
			case 0x20: br(1); break;
			case 0x21: br(0); break;
			case 0x22: br(!CY && !ResolvZ()); break; // bhi
			case 0x23: br(CY || ResolvZ()); break; // bls
			case 0x24: br(!CY); break;
			case 0x25: br(CY); break;
			case 0x26: br(!ResolvZ()); break;
			case 0x27: br(ResolvZ()); break;
			case 0x28: br(!ResolvV()); break;
			case 0x29: br(ResolvV()); break;
			case 0x2a: br(!ResolvN()); break;
			case 0x2b: br(ResolvN()); break;
			case 0x2c: br(!((ResolvN() >> LN ^ ResolvV() >> LV) & 1)); break; // bge
			case 0x2d: br((ResolvN() >> LN ^ ResolvV() >> LV) & 1); break; // blt
			case 0x2e: br(~ResolvZ() >> LZ & ~(ResolvN() >> LN ^ ResolvV() >> LV) & 1); break; // bgt
			case 0x2f: br((ResolvZ() >> LZ | (ResolvN() >> LN ^ ResolvV() >> LV)) & 1); break; // ble
			case 0x30: ix = sp + 1; clock++; break;
			case 0x31: sp++; clock++; break;
			case 0x32: A = ld8(++sp); clock += 3; break;
			case 0x33: B = ld8(++sp); clock += 3; break;
			case 0x34: sp--; clock++; break;
			case 0x35: sp = ix - 1; clock++; break;
			case 0x36: st8(sp--, A); clock += 4; break;
			case 0x37: st8(sp--, B); clock += 4; break;
			case 0x38: ix = ld16(++sp); ++sp; clock += 4; break;
			case 0x39: pc = ld16(++sp); ++sp; clock += 5; break; // rts
			case 0x3a: ix += B; clock++; break;
			case 0x3b: puls(); clock += 10; break; // rti
			case 0x3c: st16r(--sp, ix); --sp; clock += 5; break;
			case 0x3d: D = A * B; clock += 7; break;
			case 0x3e: pshs(); waitflags = W_WAI; clock += 9; break;
			case 0x3f: pshs(); intflags |= MI; pc = ld16(0xfffa); clock += 12; break; // swi
			case 0x40: ma(neg); break;
			case 0x43: ma(com); break;
			case 0x44: ma(lsr); break;
			case 0x46: ma(ror); break;
			case 0x47: ma(asr); break;
			case 0x48: ma(asl); break;
			case 0x49: ma(rol); break;
			case 0x4a: ma(dec); break;
			case 0x4c: ma(inc); break;
			case 0x4d: ftst(A); clock++; break;
			case 0x4f: A = 0; fclr(); clock += 2; break;
			case 0x50: mb(neg); break;
			case 0x53: mb(com); break;
			case 0x54: mb(lsr); break;
			case 0x56: mb(ror); break;
			case 0x57: mb(asr); break;
			case 0x58: mb(asl); break;
			case 0x59: mb(rol); break;
			case 0x5a: mb(dec); break;
			case 0x5c: mb(inc); break;
			case 0x5d: ftst(B); clock++; break;
			case 0x5f: B = 0; fclr(); clock += 2; break;
			case 0x60: mind(neg); break;
			case 0x61: mindb(aim); break;
			case 0x62: mindb(oim); break;
			case 0x63: mind(com); break;
			case 0x64: mind(lsr); break;
			case 0x65: mindb(eim); break;
			case 0x66: mind(ror); break;
			case 0x67: mind(asr); break;
			case 0x68: mind(asl); break;
			case 0x69: mind(rol); break;
			case 0x6a: mind(dec); break;
			case 0x6b: rindb(tim); break;
			case 0x6c: mind(inc); break;
			case 0x6d: rind(tst); break;
			case 0x6e: pc = imm8() + ix; clock += 3; break;
			case 0x6f: wind(clr); clock++; break;
			case 0x70: mext(neg); break;
			case 0x71: mdirb(aim); break;
			case 0x72: mdirb(oim); break;
			case 0x73: mext(com); break;
			case 0x74: mext(lsr); break;
			case 0x75: mdirb(eim); break;
			case 0x76: mext(ror); break;
			case 0x77: mext(asr); break;
			case 0x78: mext(asl); break;
			case 0x79: mext(rol); break;
			case 0x7a: mext(dec); break;
			case 0x7b: rdirb(tim); break;
			case 0x7c: mext(inc); break;
			case 0x7d: rext(tst); break;
			case 0x7e: pc = imm16(); clock += 3; break;
			case 0x7f: wext(clr); clock++; break;
			case 0x80: rimm(suba); break;
			case 0x81: rimm(cmpa); break;
			case 0x82: rimm(sbca); break;
			case 0x83: rimm16(subd); break;
			case 0x84: rimm(anda); break;
			case 0x85: rimm(bita); break;
			case 0x86: rimm(lda); break;
			case 0x88: rimm(eora); break;
			case 0x89: rimm(adca); break;
			case 0x8a: rimm(ora); break;
			case 0x8b: rimm(adda); break;
			case 0x8c: rimm16(cpx); break;
			case 0x8d: t8 = imm8(); st16r(--sp, pc); --sp; pc += (int8_t)t8; clock += 5; break; // bsr
			case 0x8e: rimm16(lds); break;
			case 0x90: rdir(suba); break;
			case 0x91: rdir(cmpa); break;
			case 0x92: rdir(sbca); break;
			case 0x93: rdir16(subd); break;
			case 0x94: rdir(anda); break;
			case 0x95: rdir(bita); break;
			case 0x96: rdir(lda); break;
			case 0x97: wdir(sta); break;
			case 0x98: rdir(eora); break;
			case 0x99: rdir(adca); break;
			case 0x9a: rdir(ora); break;
			case 0x9b: rdir(adda); break;
			case 0x9c: rdir16(cpx); break;
			case 0x9d: t8 = imm8(); st16r(--sp, pc); --sp; pc = t8; clock += 5; break; // jsr dir
			case 0x9e: rdir16(lds); break;
			case 0x9f: wdir16(sts); break;
			case 0xa0: rind(suba); break;
			case 0xa1: rind(cmpa); break;
			case 0xa2: rind(sbca); break;
			case 0xa3: rind16(subd); break;
			case 0xa4: rind(anda); break;
			case 0xa5: rind(bita); break;
			case 0xa6: rind(lda); break;
			case 0xa7: wind(sta); break;
			case 0xa8: rind(eora); break;
			case 0xa9: rind(adca); break;
			case 0xaa: rind(ora); break;
			case 0xab: rind(adda); break;
			case 0xac: rind16(cpx); break;
			case 0xad: t8 = imm8(); st16r(--sp, pc); --sp; pc = t8 + ix; clock += 5; break; // jsr ind
			case 0xae: rind16(lds); break;
			case 0xaf: wind16(sts); break;
			case 0xb0: rext(suba); break;
			case 0xb1: rext(cmpa); break;
			case 0xb2: rext(sbca); break;
			case 0xb3: rext16(subd); break;
			case 0xb4: rext(anda); break;
			case 0xb5: rext(bita); break;
			case 0xb6: rext(lda); break;
			case 0xb7: wext(sta); break;
			case 0xb8: rext(eora); break;
			case 0xb9: rext(adca); break;
			case 0xba: rext(ora); break;
			case 0xbb: rext(adda); break;
			case 0xbc: rext16(cpx); break;
			case 0xbd: t16 = imm16(); st16r(--sp, pc); --sp; pc = t16; clock += 6; break; // jsr ext
			case 0xbe: rext16(lds); break;
			case 0xbf: wext16(sts); break;
			case 0xc0: rimm(subb); break;
			case 0xc1: rimm(cmpb); break;
			case 0xc2: rimm(sbcb); break;
			case 0xc3: rimm16(addd); break;
			case 0xc4: rimm(andb); break;
			case 0xc5: rimm(bitb); break;
			case 0xc6: rimm(ldb); break;
			case 0xc8: rimm(eorb); break;
			case 0xc9: rimm(adcb); break;
			case 0xca: rimm(orb); break;
			case 0xcb: rimm(addb); break;
			case 0xcc: rimm16(ldd); break;
			case 0xce: rimm16(ldx); break;
			case 0xd0: rdir(subb); break;
			case 0xd1: rdir(cmpb); break;
			case 0xd2: rdir(sbcb); break;
			case 0xd3: rdir16(addd); break;
			case 0xd4: rdir(andb); break;
			case 0xd5: rdir(bitb); break;
			case 0xd6: rdir(ldb); break;
			case 0xd7: wdir(stb); break;
			case 0xd8: rdir(eorb); break;
			case 0xd9: rdir(adcb); break;
			case 0xda: rdir(orb); break;
			case 0xdb: rdir(addb); break;
			case 0xdc: rdir16(ldd); break;
			case 0xdd: wdir16(std); break;
			case 0xde: rdir16(ldx); break;
			case 0xdf: wdir16(stx); break;
			case 0xe0: rind(subb); break;
			case 0xe1: rind(cmpb); break;
			case 0xe2: rind(sbcb); break;
			case 0xe3: rind16(addd); break;
			case 0xe4: rind(andb); break;
			case 0xe5: rind(bitb); break;
			case 0xe6: rind(ldb); break;
			case 0xe7: wind(stb); break;
			case 0xe8: rind(eorb); break;
			case 0xe9: rind(adcb); break;
			case 0xea: rind(orb); break;
			case 0xeb: rind(addb); break;
			case 0xec: rind16(ldd); break;
			case 0xed: wind16(std); break;
			case 0xee: rind16(ldx); break;
			case 0xef: wind16(stx); break;
			case 0xf0: rext(subb); break;
			case 0xf1: rext(cmpb); break;
			case 0xf2: rext(sbcb); break;
			case 0xf3: rext16(addd); break;
			case 0xf4: rext(andb); break;
			case 0xf5: rext(bitb); break;
			case 0xf6: rext(ldb); break;
			case 0xf7: wext(stb); break;
			case 0xf8: rext(eorb); break;
			case 0xf9: rext(adcb); break;
			case 0xfa: rext(orb); break;
			case 0xfb: rext(addb); break;
			case 0xfc: rext16(ldd); break;
			case 0xfd: wext16(std); break;
			case 0xfe: rext16(ldx); break;
			case 0xff: wext16(stx); break;
			default: clock++; break;
		}
#if HD6303_TRACE
		tracep->ccr = ResolvFlags();
		tracep->acc = D;
		tracep->ix = ix;
		tracep->sp = sp;
#if HD6303_TRACE > 1
		if (++tracep >= tracebuf + TRACEMAX - 1) StopTrace();
#else
		if (++tracep >= tracebuf + TRACEMAX) tracep = tracebuf;
#endif
#endif
	} while (!waitflags && clock < n);
	return waitflags ? 0 : clock - n;
}

int HD6303::ResolvC() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf); p--)
		;
	if (p < fbuf) error();
	switch (sw) {
		case F0:
			break;
		case F1:
			return MC;
		case FB:
			return p->b & MC;
		case FADD:
			return ((p->s & p->b) | (~p->a & p->b) | (p->s & ~p->a)) >> 7 & MC;
		case FADD16:
			return ((p->s & p->b) | (~p->a & p->b) | (p->s & ~p->a)) >> 15 & MC;
		case FSUB:
			return ((p->s & ~p->b) | (p->a & ~p->b) | (p->s & p->a)) >> 7 & MC;
		case FSUB16:
			return ((p->s & ~p->b) | (p->a & ~p->b) | (p->s & p->a)) >> 15 & MC;
		case FLEFT:
			return (p->b >> 7 & 1) << LC;
		case FLEFT16:
			return p->b >> 15 << LC;
		case FRIGHT: case FRIGHT16:
			return (p->b & 1) << LC;
		case FDAA:
			return (p->s >= 0x60) << LC;
		default:
			error();
			break;
	}
	return 0;
}

int HD6303::ResolvV() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf0); p--)
		;
	if (p < fbuf) error();
	switch (sw >> 4) {
		case F0:
			break;
		case F1:
			return MV;
		case FB:
			return p->b & MV;
		case FADD:
			return ((p->b & p->s & ~p->a) | (~p->b & ~p->s & p->a)) >> (7 - LV) & MV;
		case FADD16:
			return ((p->b & p->s & ~p->a) | (~p->b & ~p->s & p->a)) >> (15 - LV) & MV;
		case FSUB:
			return ((p->b & ~p->s & ~p->a) | (~p->b & p->s & p->a)) >> (7 - LV) & MV;
		case FSUB16:
			return ((p->b & ~p->s & ~p->a) | (~p->b & p->s & p->a)) >> (15 - LV) & MV;
		case FLEFT:
			return (p->b ^ p->a) >> (7 - LV) & MV;
		case FLEFT16:
			return (p->b ^ p->a) >> (15 - LV) & MV;
		case FRIGHT:
			return (p->b << LV ^ p->a >> (7 - LV)) & MV;
		case FRIGHT16:
			return (p->b << LV ^ p->a >> (15 - LV)) & MV;
		default:
			error();
			break;
	}
	return 0;
}

int HD6303::ResolvZ() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf00); p--)
		;
	if (p < fbuf) error();
	switch (sw >> 8) {
		case F1:
			return MZ;
		case FB:
			return p->b & MZ;
		case F8:
			return !(p->a & 0xff) << LZ;
		case F16:
			return !p->a << LZ;
		default:
			error();
			break;
	}
	return 0;
}

int HD6303::ResolvN() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf000); p--)
		;
	if (p < fbuf) error();
	switch (sw >> 12) {
		case F0:
			break;
		case FB:
			return p->b & MN;
		case F8:
			return ((p->a & 0x80) != 0) << LN;
		case F16:
			return ((p->a & 0x8000) != 0) << LN;
		default:
			error();
			break;
	}
	return 0;
}

int HD6303::ResolvH() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf00000); p--)
		;
	if (p < fbuf) error();
	switch (sw >> 20) {
		case FB:
			return p->b & MH;
		case FADD:
			return ((p->s & p->b) | (~p->a & p->b) | (p->s & ~p->a)) << (LH - 3) & MH;
		default:
			error();
			break;
	}
	return 0;
}

void HD6303::SetupFlags(int x) {
	fp = fbuf;
	fp->dm = HB | NB | ZB | VB | CB;
	fp++->b = intflags = x;
}

int HD6303::ResolvFlags() {
	int r = ResolvH() | ResolvN() | ResolvZ() | ResolvV() | ResolvC();
	r |= 0xc0 | (intflags & MI);
	SetupFlags(r);
	return r;
}

#if HD6303_TRACE
#include <string>
void HD6303::StopTrace() {
	TraceBuffer *endp = tracep;
	int i = 0, j;
	FILE *fo;
	if (!(fo = fopen((std::string(getenv("HOME")) + "/Desktop/trace.txt").c_str(), "w"))) exit(1);
	do {
		if (++tracep >= tracebuf + TRACEMAX) tracep = tracebuf;
		fprintf(fo, "%4d %04X  ", i++, tracep->pc);
		for (j = 0; j < tracep->opn; j++) fprintf(fo, "%02X ", tracep->op[j]);
		for (; j < OPMAX; j++) fprintf(fo, "   ");
		fprintf(fo, "%04X %04X %04X %c%c%c%c%c%c ",
				tracep->acc, tracep->ix, tracep->sp,
				tracep->ccr & 0x20 ? 'H' : '-',
				tracep->ccr & 0x10 ? 'I' : '-',
				tracep->ccr & 0x08 ? 'N' : '-',
				tracep->ccr & 0x04 ? 'Z' : '-',
				tracep->ccr & 0x02 ? 'V' : '-',
				tracep->ccr & 0x01 ? 'C' : '-');
		for (Acs *p = tracep->acs; p < tracep->acs + tracep->index; p++) {
			switch (p->type) {
				case acsLoad8:
					fprintf(fo, "L %04X %02X ", p->adr, p->data & 0xff);
					break;
				case acsLoad16:
					fprintf(fo, "L %04X %04X ", p->adr, p->data);
					break;
				case acsStore8:
					fprintf(fo, "S %04X %02X ", p->adr, p->data & 0xff);
					break;
				case acsStore16:
					fprintf(fo, "S %04X %04X ", p->adr, p->data);
					break;
			}
		}
		fprintf(fo, "\n");
	} while (tracep != endp);
	fclose(fo);
	fprintf(stderr, "trace dumped.\n");
	exit(1);
}
#endif	// HD6303_TRACE
