// HD6303
// Copyright 2022-2024 © Yasuo Kuwahara
// MIT License

#include "HD6303.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

enum {
	LC, LV, LZ, LN, LI, LH
};

enum {
	MC = 1 << LC, MV = 1 << LV, MZ = 1 << LZ, MN = 1 << LN, MI = 1 << LI, MH = 1 << LH
};

enum {
	F0 = 1, F1, F8, F16, FADD, FADD16, FSUB, FSUB16, FLEFT, FLEFT16, FRIGHT, FRIGHT16, FDAA
};

#define F(flag, type)	flag##type = F##type << (L##flag << 2)
enum {
	F(C, 0), F(C, 1), F(C, ADD), F(C, ADD16), F(C, SUB), F(C, SUB16),
	F(C, LEFT), F(C, LEFT16), F(C, RIGHT), F(C, RIGHT16), F(C, DAA),
	F(V, 0), F(V, 1), F(V, ADD), F(V, ADD16), F(V, SUB), F(V, SUB16),
	F(V, LEFT), F(V, LEFT16), F(V, RIGHT), F(V, RIGHT16),
	F(Z, 1), F(Z, 8), F(Z, 16),
	F(N, 0), F(N, 8), F(N, 16),
	F(H, ADD)
};

#define fclc()			fset<C0>()
#define fsec()			fset<C1>()
#define fclv()			fset<V0>()
#define fsev()			fset<V1>()
#define fclr()			fset<N0 | Z1 | V0 | C0>()
#define fld(a)			fset<N8 | Z8 | V0>(a)
#define fld16(a)		fset<N16 | Z16 | V0>(a)
#define ftst(a)			fset<N8 | Z8 | V0 | C0>(a)
#define fcom(a)			fset<N8 | Z8 | V0 | C1>(a)
#define fadd(a, d, s)	fset<HADD | N8 | Z8 | VADD | CADD>(a, d, s)
#define fadd16(a, d, s)	fset<N16 | Z16 | VADD16 | CADD16>(a, d, s)
#define fsub(a, d, s)	fset<N8 | Z8 | VSUB | CSUB>(a, d, s)
#define fsub16(a, d, s)	fset<N16 | Z16 | VSUB16 | CSUB16>(a, d, s)
#define fcpx(a, d, s)	fset<N16 | Z16 | VSUB16>(a, d, s)
#define finc(a, d)		fset<N8 | Z8 | VADD>(a, d, 1)
#define fdec(a, d)		fset<N8 | Z8 | VSUB>(a, d, 1)
#define fz16(a)			fset<Z16>(a)
#define fleft(a, d)		fset<N8 | Z8 | VLEFT | CLEFT>(a, d)
#define fleft16(a, d)	fset<N16 | Z16 | VLEFT16 | CLEFT16>(a, d)
#define fright(a, d)	fset<N8 | Z8 | VRIGHT | CRIGHT>(a, d)
#define fright16(a, d)	fset<N16 | Z16 | VRIGHT16 | CRIGHT16>(a, d)
#define fdaa(a, d)		fset<N8 | Z8 | CDAA>(a, d)

#define CY				(ccr & 1)

#define A				acc[1]
#define B				acc[0]
#define D				((u16 &)acc[0])

enum { W_WAI = 1, W_SLEEP };

HD6303::HD6303() {
#if HD6303_TRACE
	memset(tracebuf, 0, sizeof(tracebuf));
	tracep = tracebuf;
#endif
}

void HD6303::Reset() {
	irq = waitflags = 0;
	D = 0xffff;
	ccr = 0xff;
	sp = ix = 0xffff;
	pc = ld16(0xfffe);
}

int HD6303::Execute(int n) {
	auto lda = [&](u8 s) { fld(A = s); };
	auto ldb = [&](u8 s) { fld(B = s); };
	auto ldd = [&](u16 s) { fld16(D = s); };
	auto lds = [&](u16 s) { fld16(sp = s); };
	auto ldx = [&](u16 s) { fld16(ix = s); };
	auto sta = [&] { fld(A); return A; };
	auto stb = [&] { fld(B); return B; };
	auto std = [&] { fld16(D); return D; };
	auto sts = [&] { fld16(sp); return sp; };
	auto stx = [&] { fld16(ix); return ix; };
	auto clr = [&] { fclr(); return 0; };
	auto adda = [&](u8 s) { A = fadd(A + s, A, s); };
	auto addb = [&](u8 s) { B = fadd(B + s, B, s); };
	auto addd = [&](u16 s) { D = fadd16(D + s, D, s); };
	auto adca = [&](u8 s) { A = fadd(A + s + CY, A, s); };
	auto adcb = [&](u8 s) { B = fadd(B + s + CY, B, s); };
	auto suba = [&](u8 s) { A = fsub(A - s, A, s); };
	auto subb = [&](u8 s) { B = fsub(B - s, B, s); };
	auto subd = [&](u16 s) { D = fsub16(D - s, D, s); };
	auto sbca = [&](u8 s) { A = fsub(A - s - CY, A, s); };
	auto sbcb = [&](u8 s) { B = fsub(B - s - CY, B, s); };
	auto cmpa = [&](u8 s) { fsub(A - s, A, s); };
	auto cmpb = [&](u8 s) { fsub(B - s, B, s); };
	auto cpx = [&](u16 s) { u16 t = (((ix >> 8) - (s >> 8)) << 8) | (ix - s & 0xff); fcpx(t, ix, s); };
	auto inc = [&](u8 s) { return finc(s + 1, s); };
	auto dec = [&](u8 s) { return fdec(s - 1, s); };
	auto anda = [&](u8 s) { fld(A &= s); };
	auto andb = [&](u8 s) { fld(B &= s); };
	auto aim = [&](u8 s, u8 imm) { fld(s &= imm); return s; };
	auto ora = [&](u8 s) { fld(A |= s); };
	auto orb = [&](u8 s) { fld(B |= s); };
	auto oim = [&](u8 s, u8 imm) { fld(s |= imm); return s; };
	auto eora = [&](u8 s) { fld(A ^= s); };
	auto eorb = [&](u8 s) { fld(B ^= s); };
	auto eim = [&](u8 s, u8 imm) { fld(s ^= imm); return s; };
	auto bita = [&](u8 s) { fld(A & s); };
	auto bitb = [&](u8 s) { fld(B & s); };
	auto tim = [&](u8 s, u8 imm) { fld(s & imm); };
	auto tst = [&](u8 s) { ftst(s); };
	auto com = [&](u8 s) { fcom(s = ~s); return s; };
	auto neg = [&](u8 s) { return fsub(-s, 0, s); };
	auto asl = [&](u8 s) { return fleft(s << 1, s); };
	auto rol = [&](u8 s) { return fleft(s << 1 | CY, s); };
	auto asr = [&](s8 s) { return fright(s >> 1, s); };
	auto lsr = [&](u8 s) { return fright(s >> 1, s); };
	auto ror = [&](u8 s) { return fright(s >> 1 | CY << 7, s); };
	auto br = [&](u8 cond) { pc += cond ? (s8)imm8() : 1; clock += 3; };
	auto pshs = [&] {
		st16r(--sp, pc); st16r(sp -= 2, ix); st8(--sp, A); st8(--sp, B); st8(--sp, ccr); --sp;
	};
	auto puls = [&] {
		ccr = ld8(++sp) | 0xc0; B = ld8(++sp); A = ld8(++sp); ix = ld16(++sp); pc = ld16(sp += 2); ++sp;
	};
	auto daa = [&] {
		int u = A >> 4, l = A & 0xf, c = CY, h = ccr & MH;
		int d = c ? h ? u <= 3 && l <= 3 ? 0x66 : 0 : u <= 2 ? l <= 9 ? 0x60 : 0x66 : 0 :
			h ? l <= 3 ? u <= 9 ? 6 : 0x66 : 0 : l <= 9 ? u <= 9 ? 0 : 0x60 : u <= 8 ? 6 : 0x66;
		A = fdaa(A + d, d);
	};
	clock = 0;
	do {
		if (irq) {
			if (irq & M_NMI) {
				irq &= ~M_NMI;
				pshs();
				ccr |= MI;
				waitflags = 0;
				pc = ld16(0xfffc);
			}
			else if (irq & M_IRQ && !(ccr & MI)) {
				irq &= ~M_IRQ;
				pshs();
				ccr |= MI;
				waitflags = 0;
				pc = ld16(0xfff8);
			}
		}
#if HD6303_TRACE
		tracep->pc = pc;
		tracep->index = tracep->opn = 0;
#endif
		u8 t8;
		u16 t16;
		switch (imm8()) {
			case 0x01: clock++; break;
			case 0x04: D = fright16(D >> 1, D); clock++; break;
			case 0x05: D = fleft16(D << 1, D); clock++; break;
			case 0x06: ccr = A | 0xc0; clock++; break;
			case 0x07: A = ccr; clock++; break;
			case 0x08: fz16(++ix); clock++; break;
			case 0x09: fz16(--ix); clock++; break;
			case 0x0a: fclv(); clock++; break;
			case 0x0b: fsev(); clock++; break;
			case 0x0c: fclc(); clock++; break;
			case 0x0d: fsec(); clock++; break;
			case 0x0e: ccr &= ~MI; clock++; break;
			case 0x0f: ccr |= MI; clock++; break;
			case 0x10: A = fsub(A - B, A, B); clock++; break;
			case 0x11: fsub(A - B, A, B); clock++; break;
			case 0x16: fld(B = A); clock++; break;
			case 0x17: fld(A = B); clock++; break;
			case 0x18: t16 = D; D = ix; ix = t16; clock += 2; break;
			case 0x19: daa(); clock += 2; break;
			case 0x1a: waitflags |= W_SLEEP; pc--; return 0;
			case 0x1b: A = fadd(A + B, A, B); clock++; break;
			case 0x20: br(1); break;
			case 0x21: br(0); break;
			case 0x22: br(!CY && !(ccr & MZ)); break; // bhi
			case 0x23: br(CY || ccr & MZ); break; // bls
			case 0x24: br(!CY); break;
			case 0x25: br(CY); break;
			case 0x26: br(!(ccr & MZ)); break;
			case 0x27: br(ccr & MZ); break;
			case 0x28: br(!(ccr & MV)); break;
			case 0x29: br(ccr & MV); break;
			case 0x2a: br(!(ccr & MN)); break;
			case 0x2b: br(ccr & MN); break;
			case 0x2c: br(!((ccr ^ ccr << (LN - LV)) & MN)); break; // bge
			case 0x2d: br((ccr ^ ccr << (LN - LV)) & MN); break; // blt
			case 0x2e: br(!((ccr << (LN - LZ) | (ccr ^ ccr << (LN - LV))) & MN)); break; // bgt
			case 0x2f: br((ccr << (LN - LZ) | (ccr ^ ccr << (LN - LV))) & MN); break; // ble
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
			case 0x3e: if (!(waitflags & W_WAI)) { pshs(); waitflags |= W_WAI; }
				pc--; return 0;
			case 0x3f: pshs(); ccr |= MI; pc = ld16(0xfffa); clock += 12; break; // swi
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
			case 0x8d: t8 = imm8(); st16r(--sp, pc); --sp; pc += (s8)t8; clock += 5; break; // bsr
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
		tracep->ccr = ccr;
		tracep->acc = D;
		tracep->ix = ix;
		tracep->sp = sp;
#if HD6303_TRACE > 1
		if (++tracep >= tracebuf + TRACEMAX - 1) StopTrace();
#else
		if (++tracep >= tracebuf + TRACEMAX) tracep = tracebuf;
#endif
#endif
	} while (clock < n);
	return clock - n;
}

template<int M> HD6303::u16 HD6303::fset(u16 a, u16 d, u16 s) {
	if constexpr ((M & 0xf) == C0)
		ccr &= ~MC;
	if constexpr ((M & 0xf) == C1)
		ccr |= MC;
	if constexpr ((M & 0xf) == CADD) {
		if (((s & d) | (~a & d) | (s & ~a)) & 0x80) ccr |= MC;
		else ccr &= ~MC;
	}
	if constexpr ((M & 0xf) == CADD16) {
		if (((s & d) | (~a & d) | (s & ~a)) & 0x8000) ccr |= MC;
		else ccr &= ~MC;
	}
	if constexpr ((M & 0xf) == CSUB) {
		if (((s & ~d) | (a & ~d) | (s & a)) & 0x80) ccr |= MC;
		else ccr &= ~MC;
	}
	if constexpr ((M & 0xf) == CSUB16) {
		if (((s & ~d) | (a & ~d) | (s & a)) & 0x8000) ccr |= MC;
		else ccr &= ~MC;
	}
	if constexpr ((M & 0xf) == CLEFT) {
		if (d & 0x80) ccr |= MC;
		else ccr &= ~MC;
	}
	if constexpr ((M & 0xf) == CLEFT16) {
		if (d & 0x8000) ccr |= MC;
		else ccr &= ~MC;
	}
	if constexpr ((M & 0xf) == CRIGHT || (M & 0xf) == CRIGHT16) {
		if (d & 1) ccr |= MC;
		else ccr &= ~MC;
	}
	if constexpr ((M & 0xf) == CDAA) {
		if (d >= 0x60) ccr |= MC;
		else ccr &= ~MC;
	}
	if constexpr ((M & 0xf0) == V0)
		ccr &= ~MV;
	if constexpr ((M & 0xf0) == V1)
		ccr |= MV;
	if constexpr ((M & 0xf0) == VADD) {
		if (((d & s & ~a) | (~d & ~s & a)) & 0x80) ccr |= MV;
		else ccr &= ~MV;
	}
	if constexpr ((M & 0xf0) == VADD16) {
		if (((d & s & ~a) | (~d & ~s & a)) & 0x8000) ccr |= MV;
		else ccr &= ~MV;
	}
	if constexpr ((M & 0xf0) == VSUB) {
		if (((d & ~s & ~a) | (~d & s & a)) & 0x80) ccr |= MV;
		else ccr &= ~MV;
	}
	if constexpr ((M & 0xf0) == VSUB16) {
		if (((d & ~s & ~a) | (~d & s & a)) & 0x8000) ccr |= MV;
		else ccr &= ~MV;
	}
	if constexpr ((M & 0xf0) == VLEFT) {
		if ((d ^ a) & 0x80) ccr |= MV;
		else ccr &= ~MV;
	}
	if constexpr ((M & 0xf0) == VLEFT16) {
		if ((d ^ a) & 0x8000) ccr |= MV;
		else ccr &= ~MV;
	}
	if constexpr ((M & 0xf0) == VRIGHT) {
		if ((d ^ a >> 7) & 1) ccr |= MV;
		else ccr &= ~MV;
	}
	if constexpr ((M & 0xf0) == VRIGHT16) {
		if ((d ^ a >> 15) & 1) ccr |= MV;
		else ccr &= ~MV;
	}
	if constexpr ((M & 0xf00) == Z1)
		ccr |= MZ;
	if constexpr ((M & 0xf00) == Z8) {
		if (a & 0xff) ccr &= ~MZ;
		else ccr |= MZ;
	}
	if constexpr ((M & 0xf00) == Z16) {
		if (a) ccr &= ~MZ;
		else ccr |= MZ;
	}
	if constexpr ((M & 0xf000) == N0)
		ccr &= ~MN;
	if constexpr ((M & 0xf000) == N8) {
		if (a & 0x80) ccr |= MN;
		else ccr &= ~MN;
	}
	if constexpr ((M & 0xf000) == N16) {
		if (a & 0x8000) ccr |= MN;
		else ccr &= ~MN;
	}
	if constexpr ((M & 0xf00000) == HADD) {
		if (((s & d) | (~a & d) | (s & ~a)) & 8) ccr |= MH;
		else ccr &= ~MH;
	}
	return a;
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
