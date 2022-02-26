// HD6303
// Copyright 2022 © Yasuo Kuwahara
// MIT License

#include "test.h"	// bm2
#include <stdint.h>

#define HD6303_TRACE	0

#if HD6303_TRACE
#define HD6303_TRACE_LOG(adr, data, type) \
	if (tracep->index < ACSMAX) tracep->acs[tracep->index++] = { adr, (uint16_t)data, type }
#else
#define HD6303_TRACE_LOG(adr, data, type)
#endif

class HD6303 {
	enum { M_IRQ = 1, M_NMI };
	static constexpr int FBUFMAX = 128;
public:
	HD6303();
	void Reset();
	int Execute(int n);
	uint16_t GetPC() const { return pc; }
	void IRQ() { irq |= M_IRQ; }
	void NMI() { irq |= M_NMI; }
private:
	// customize for bm2 -- start
	uint8_t imm8() {
		uint8_t data = read8(pc++);
#if HD6303_TRACE
		if (tracep->opn < OPMAX) tracep->op[tracep->opn++] = data;
#endif
		return data;
	}
	uint16_t imm16() {
		uint8_t data0 = read8(pc++), data1 = read8(pc++);
#if HD6303_TRACE
		if (tracep->opn < OPMAX) tracep->op[tracep->opn++] = data0;
		if (tracep->opn < OPMAX) tracep->op[tracep->opn++] = data1;
#endif
		return data0 << 8 | data1;
	}
	int32_t ld8(uint16_t adr) {
		int32_t data = read8(adr);
		HD6303_TRACE_LOG(adr, data, acsLoad8);
		return data;
	}
	int32_t ld16(uint16_t adr) {
		int32_t data = read8(adr) << 8;
		data |= read8(adr + 1);
		HD6303_TRACE_LOG(adr, data, acsLoad16);
		return data;
	}
	void st8(uint16_t adr, uint8_t data) {
		write8(adr, data);
		HD6303_TRACE_LOG(adr, data, acsStore8);
	}
	void st16(uint16_t adr, uint16_t data) {
		write8(adr, data >> 8);
		write8(adr + 1, data);
		HD6303_TRACE_LOG(adr, data, acsStore16);
	}
	void st16r(uint16_t adr, uint16_t data) {
		write8(adr + 1, data);
		write8(adr, data >> 8);
		HD6303_TRACE_LOG(adr, data, acsStore16);
	}
	// customize for bm2 -- end
	template<typename F> void rimm(F func) { func(imm8()); clock += 2; };
	template<typename F> void rimm16(F func) { func(imm16()); clock += 3; };
	template<typename F> void ma(F func) { acc[1] = func(acc[1]); clock++; };
	template<typename F> void mb(F func) { acc[0] = func(acc[0]); clock++; };
	template<typename F> void rext(F func) { func(ld8(imm16())); clock += 4; };
	template<typename F> void rext16(F func) { func(ld16(imm16())); clock += 5; };
	template<typename F> void wext(F func) { st8(imm16(), func()); clock += 4; };
	template<typename F> void wext16(F func) { st16(imm16(), func()); clock += 5; };
	template<typename F> void mext(F func) { uint16_t t = imm16(); st8(t, func(ld8(t))); clock += 6; };
	template<typename F> void rdir(F func) { func(ld8(imm8())); clock += 3; };
	template<typename F> void rdirb(F func) { uint8_t i = imm8(); func(ld8(imm8()), i); clock += 4; };
	template<typename F> void rdir16(F func) { func(ld16(imm8())); clock += 4; };
	template<typename F> void wdir(F func) { st8(imm8(), func()); clock += 3; };
	template<typename F> void wdir16(F func) { st16(imm8(), func()); clock += 4; };
	template<typename F> void mdirb(F func) { uint8_t i = imm8(), t = imm8(); st8(t, func(ld8(t), i)); clock += 6; };
	template<typename F> void rind(F func) { func(ld8(imm8() + ix)); clock += 4; };
	template<typename F> void rindb(F func) { uint8_t i = imm8(); func(ld8(imm8() + ix), i); clock += 5; };
	template<typename F> void rind16(F func) { func(ld16(imm8() + ix)); clock += 5; };
	template<typename F> void wind(F func) { st8(imm8() + ix, func()); clock += 4; };
	template<typename F> void wind16(F func) { st16(imm8() + ix, func()); clock += 5; };
	template<typename F> void mind(F func) { uint16_t t = imm8() + ix; st8(t, func(ld8(t))); clock += 6; };
	template<typename F> void mindb(F func) { uint8_t i = imm8(); uint16_t t = imm8() + ix; st8(t, func(ld8(t), i)); clock += 7; };
	int ResolvC();
	int ResolvV();
	int ResolvZ();
	int ResolvN();
	int ResolvI();
	int ResolvH();
	int ResolvFlags();
	void SetupFlags(int x);
	struct FlagDecision {
		uint32_t dm;
		uint16_t s, b, a;
	};
	FlagDecision fbuf[FBUFMAX];
	FlagDecision *fp;
	uint8_t irq, waitflags, intflags;
	uint8_t acc[2];
	uint16_t pc, sp, ix;
	int clock;
#ifdef HD6303_TRACE
	static constexpr int TRACEMAX = 10000;
	static constexpr int ACSMAX = 2;
	static constexpr int OPMAX = 3;
	enum {
		acsStore8 = 4, acsStore16, acsLoad8, acsLoad16
	};
	struct Acs {
		uint16_t adr, data;
		uint8_t type;
	};
	struct TraceBuffer {
		uint8_t ccr;
		uint16_t acc, ix, sp, pc;
		Acs acs[ACSMAX];
		uint8_t op[OPMAX];
		uint8_t index, opn;
	};
	TraceBuffer tracebuf[TRACEMAX];
	TraceBuffer *tracep;
public:
	void StopTrace();
#endif
};
