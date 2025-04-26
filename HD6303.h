// HD6303
// Copyright 2022-2025 © Yasuo Kuwahara
// MIT License

#include "test.h"	// bm2
#include <cstdint>

#define HD6303_TRACE	0

#if HD6303_TRACE
#define HD6303_TRACE_LOG(adr, data, type) \
	if (tracep->index < ACSMAX) tracep->acs[tracep->index++] = { adr, (u16)data, type }
#else
#define HD6303_TRACE_LOG(adr, data, type)
#endif

class HD6303 {
	using s8 = int8_t;
	using u8 = uint8_t;
	using u16 = uint16_t;
	enum { W_WAI = 1, W_SLP };
	enum { M_IRQ = 1, M_NMI };
	enum {
		LC, LV, LZ, LN, LI, LH
	};
	enum {
		MC = 1 << LC, MV = 1 << LV, MZ = 1 << LZ, MN = 1 << LN, MI = 1 << LI, MH = 1 << LH
	};
	enum {
		F0 = 1, F1, FDEF, FADD, FSUB, FLEFT, FRIGHT, FDAA
	};
#define F(flag, type)	flag##type = F##type << (L##flag << 2)
	enum {
		F(C, 0), F(C, 1), F(C, ADD), F(C, SUB),
		F(C, LEFT), F(C, RIGHT), F(C, DAA),
		F(V, 0), F(V, 1), F(V, ADD), F(V, SUB),
		F(V, LEFT), F(V, RIGHT),
		F(Z, 1), F(Z, DEF),
		F(N, 0), F(N, DEF),
		F(H, ADD)
	};
#undef F
public:
	HD6303();
	void Reset();
	int Execute(int n);
	bool isWaiting() const { return waitflags; }
	u16 GetPC() const { return pc; }
	void IRQ() { irq |= M_IRQ; }
	void NMI() { irq |= M_NMI; }
private:
	// customize for bm2 -- start
	u8 imm8() {
		u8 data = read8(pc++);
#if HD6303_TRACE
		if (tracep->opn < OPMAX) tracep->op[tracep->opn++] = data;
#endif
		return data;
	}
	u16 imm16() {
		u8 data0 = read8(pc++), data1 = read8(pc++);
#if HD6303_TRACE
		if (tracep->opn < OPMAX) tracep->op[tracep->opn++] = data0;
		if (tracep->opn < OPMAX) tracep->op[tracep->opn++] = data1;
#endif
		return data0 << 8 | data1;
	}
	u8 ld8(u16 adr) {
		u8 data = read8(adr);
		HD6303_TRACE_LOG(adr, data, acsLoad8);
		return data;
	}
	u16 ld16(u16 adr) {
		u16 data = read8(adr) << 8;
		data |= read8(adr + 1);
		HD6303_TRACE_LOG(adr, data, acsLoad16);
		return data;
	}
	void st8(u16 adr, u8 data) {
		write8(adr, data);
		HD6303_TRACE_LOG(adr, data, acsStore8);
	}
	void st16(u16 adr, u16 data) {
		write8(adr, data >> 8);
		write8(adr + 1, data);
		HD6303_TRACE_LOG(adr, data, acsStore16);
	}
	void st16r(u16 adr, u16 data) {
		write8(adr + 1, data);
		write8(adr, data >> 8);
		HD6303_TRACE_LOG(adr, data, acsStore16);
	}
	// customize for bm2 -- end
	template<int S = 0> void fld(u16 a) { fset<NDEF | ZDEF | V0, S>(a); }
	template<int S = 0> u16 fsub(u16 a, u16 d, u16 s) { return fset<NDEF | ZDEF | VSUB | CSUB, S>(a, d, s); }
	template<int S = 0> u16 fleft(u16 a, u16 d) { return fset<NDEF | ZDEF | VLEFT | CLEFT, S>(a, d); }
	template<int S = 0> u16 fright(u16 a, u16 d) { return fset<NDEF | ZDEF | VRIGHT | CRIGHT, S>(a, d); }
	//
	template<typename F> void rimm(F func) { func(imm8()); clock += 2; }
	template<typename F> void rimm16(F func) { func(imm16()); clock += 3; }
	template<typename F> void ma(F func) { acc[1] = func(acc[1]); clock++; }
	template<typename F> void mb(F func) { acc[0] = func(acc[0]); clock++; }
	template<typename F> void rext(F func) { func(ld8(imm16())); clock += 4; }
	template<typename F> void rext16(F func) { func(ld16(imm16())); clock += 5; }
	template<typename F> void wext(F func) { st8(imm16(), func()); clock += 4; }
	template<typename F> void wext16(F func) { st16(imm16(), func()); clock += 5; }
	template<typename F> void mext(F func) { u16 t = imm16(); st8(t, func(ld8(t))); clock += 6; }
	template<typename F> void rdir(F func) { func(ld8(imm8())); clock += 3; }
	template<typename F> void rdirb(F func) { u8 i = imm8(); func(ld8(imm8()), i); clock += 4; }
	template<typename F> void rdir16(F func) { func(ld16(imm8())); clock += 4; }
	template<typename F> void wdir(F func) { st8(imm8(), func()); clock += 3; }
	template<typename F> void wdir16(F func) { st16(imm8(), func()); clock += 4; }
	template<typename F> void mdirb(F func) { u8 i = imm8(), t = imm8(); st8(t, func(ld8(t), i)); clock += 6; }
	template<typename F> void rind(F func) { func(ld8(imm8() + ix)); clock += 4; }
	template<typename F> void rindb(F func) { u8 i = imm8(); func(ld8(imm8() + ix), i); clock += 5; }
	template<typename F> void rind16(F func) { func(ld16(imm8() + ix)); clock += 5; }
	template<typename F> void wind(F func) { st8(imm8() + ix, func()); clock += 4; }
	template<typename F> void wind16(F func) { st16(imm8() + ix, func()); clock += 5; }
	template<typename F> void mind(F func) { u16 t = imm8() + ix; st8(t, func(ld8(t))); clock += 6; }
	template<typename F> void mindb(F func) { u8 i = imm8(); u16 t = imm8() + ix; st8(t, func(ld8(t), i)); clock += 7; }
	template<int M, int S = 0> u16 fset(u16 a = 0, u16 d = 0, u16 s = 0);
	u8 ccr, irq, waitflags;
	u8 acc[2];
	u16 pc, sp, ix;
	int clock;
#if HD6303_TRACE
	static constexpr int TRACEMAX = 10000;
	static constexpr int ACSMAX = 5;
	static constexpr int OPMAX = 3;
	enum {
		acsStore8 = 4, acsStore16, acsLoad8, acsLoad16
	};
	struct Acs {
		u16 adr, data;
		u8 type;
	};
	struct TraceBuffer {
		u8 ccr;
		u16 acc, ix, sp, pc;
		Acs acs[ACSMAX];
		u8 op[OPMAX];
		u8 index, opn;
	};
	TraceBuffer tracebuf[TRACEMAX];
	TraceBuffer *tracep;
public:
	void StopTrace();
#endif
};
