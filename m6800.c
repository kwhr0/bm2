/*
	Motorola M6800 Emulator Main (m6800.c)
*/

#include <stdio.h>
#include "m6800.h"

#if !defined(M68_SUB)
#	define m68subroutine(m68, p)	(-1)
#endif

#define FALSE	0
#define TRUE	1

#define MASK_C	0x01
#define MASK_V	0x02
#define MASK_Z	0x04
#define MASK_N	0x08
#define MASK_I	0x10
#define MASK_H	0x20

#define A	m68->a
#define B	m68->b
#define X	m68->x
#define PC	m68->pc
#define SP	m68->sp
#define CC	m68->cc
#define C	(m68->cc & MASK_C)
#define V	(m68->cc & MASK_V)
#define Z	(m68->cc & MASK_Z)
#define N	(m68->cc & MASK_N)
#define I	(m68->cc & MASK_I)
#define H	(m68->cc & MASK_H)

#define LOAD(p)	m68read8(m68, p)
#define LOAD16(p)	m68read16(m68, p)
#define STORE(p, x)	m68write8(m68, p, x)
#define STORE16(p, x)	m68write16(m68, p, x)

#define IMMADR	(m68->pc + 1)
#define DIRADR	m68read8(m68, m68->pc + 1)
#define EXTADR	m68read16(m68, m68->pc + 1)
#define INDADR	(m68read8(m68, m68->pc + 1) + m68->x)
#define RELADR	((int8 )m68read8(m68, m68->pc + 1))
#define IMM	m68read8(m68, IMMADR)
#define DIR	m68read8(m68, DIRADR)
#define EXT	m68read8(m68, EXTADR)
#define IND	m68read8(m68, INDADR)

#define SET_C(acc)	((acc) & 0x00000100L ? MASK_C: 0)
#define SET_CS(acc)	((acc) & 0x80000000L ? MASK_C: 0)

#define SET_V(acc, a, x)	(((a) ^ (x)) & 0x80 ? 0: (((a) ^ acc) & 0x80 ? MASK_V: 0))
#define SET_VS(acc, a, x)	(((a) ^ (x)) & 0x80 ? (((a) ^ acc) & 0x80 ? MASK_V: 0): 0)
#define SET_VR(c, n)	(((c) && !(n)) || (!(c) && (n)) ? MASK_V: 0)

#define SET_Z(acc)	((acc) & 0x000000ffL ? 0: MASK_Z)
#define SET_Z16(acc)	((acc) & 0x0000ffffL ? 0: MASK_Z)

#define SET_N(acc)	((acc) & 0x00000080L ? MASK_N: 0)
#define SET_N16(acc)	((acc) & 0x00008000L ? MASK_N: 0)

#define SET_H(x, y, c)	((((x) & 0x0f) + ((y) & 0x0f) + (c)) & 0x10 ? MASK_H: 0)
#define SET_HS(x, y, c)	((((x) & 0x0f) - ((y) & 0x0f) - (c)) & 0x10 ? MASK_H: 0)

#define ABA() \
	{ \
		uint32 _acc = (uint32 )A + B; \
		CC = SET_C(_acc) | SET_V(_acc, A, B) | SET_Z(_acc) | SET_N(_acc) | I | SET_H(A, B, 0); \
		A = _acc; \
		PC += _length; \
	}

#define ADC(a, m) \
	{ \
		uint32 _acc = (uint32 )(a) + (m) + C; \
		CC = SET_C(_acc) | SET_V(_acc, a, m) | SET_Z(_acc) | SET_N(_acc) | I | SET_H(a, m, C); \
		(a) = _acc; \
		PC += _length; \
	}

#define ADD(a, m) \
	{ \
		uint32 _acc = (uint32 )(a) + (m); \
		CC = SET_C(_acc) | SET_V(_acc, a, m) | SET_Z(_acc) | SET_N(_acc) | I | SET_H(a, m, 0); \
		(a) = _acc; \
		PC += _length; \
	}

#define AND(a, m) \
	{ \
		uint32 _acc = (uint32 )(a) & (m); \
		CC = C | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc; \
		PC += _length; \
	}

#define _ASL(a) \
	{ \
		uint32 _acc = (uint32 )(a) << 1; \
		uint8 _c = ((a) & 0x80 ? MASK_C: 0); \
		uint8 _n = SET_N(_acc); \
		CC = _c | SET_VR(_c, _n) | SET_Z(_acc) | _n | I | H; \
		(a) = _acc; \
	}
#define ASL_R(a) \
	{ \
		_ASL(a); \
		PC += _length; \
	}
#define ASL_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_ASL(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define _ASR(a) \
	{ \
		uint32 _acc = ((uint32 )(a) >> 1) | ((a) & 0x80); \
		uint8 _c = ((a) & 0x01 ? MASK_C: 0); \
		uint8 _n = SET_N(_acc); \
		CC = _c | SET_VR(_c, _n) | SET_Z(_acc) | _n | I | H; \
		(a) = _acc; \
	}
#define ASR_R(a) \
	{ \
		_ASR(a); \
		PC += _length; \
	}
#define ASR_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_ASR(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define BCC(r) \
	{ \
		PC += _length + (!C ? (r): 0); \
	}

#define BCS(r) \
	{ \
		PC += _length + (C ? (r): 0); \
	}

#define BEQ(r) \
	{ \
		PC += _length + (Z ? (r): 0); \
	}

#define BGE(r) \
	{ \
		PC += _length + ((N && V) || (!N && !V) ? (r): 0); \
	}

#define BGT(r) \
	{ \
		PC += _length + (!Z && ((N && V) || (!N && !V)) ? (r): 0); \
	}

#define BHI(r) \
	{ \
		PC += _length + (!C && !Z ? (r): 0); \
	}

#define BLE(r) \
	{ \
		PC += _length + (Z || (N && !V) || (!N && V) ? (r): 0); \
	}

#define BLS(r) \
	{ \
		PC += _length + (C || Z ? (r): 0); \
	}

#define BLT(r) \
	{ \
		PC += _length + ((N && !V) || (!N && V) ? (r): 0); \
	}

#define BMI(r) \
	{ \
		PC += _length + (N ? (r): 0); \
	}

#define BNE(r) \
	{ \
		PC += _length + (!Z ? (r): 0); \
	}

#define BPL(r) \
	{ \
		PC += _length + (!N ? (r): 0); \
	}

#define BRA(r) \
	{ \
		PC += _length + (r); \
	}

#define BVC(r) \
	{ \
		PC += _length + (!V ? (r): 0); \
	}

#define BVS(r) \
	{ \
		PC += _length + (V ? (r): 0); \
	}

#define BSR(r) \
	{ \
		uint16 _pc = PC + _length; \
		STORE(SP, _pc & 0x00ff); \
		SP--; \
		STORE(SP, _pc >> 8); \
		SP--; \
		PC += (r) + _length; \
	}

#define BIT(a, m) \
	{ \
		uint32 _acc = (a) & (m); \
		CC = C | SET_Z(_acc) | SET_N(_acc) | I | H; \
		PC += _length; \
	}

#define CBA() \
	{ \
		uint32 _acc = (uint32 )A - B; \
		CC = SET_CS(_acc) | SET_VS(_acc, A, B) | SET_Z(_acc) | SET_N(_acc) | I | H; \
		PC += _length; \
	}

#define CLC() \
	{ \
		CC = V | Z | N | I | H; \
		PC += _length; \
	}

#define CLI() \
	{ \
		CC = C | V | Z | N | H; \
		PC += _length; \
	}

#define _CLR(a) \
	{ \
		(a) = 0; \
		CC = MASK_Z | I | H; \
	}
#define CLR_R(a) \
	{ \
		_CLR(a); \
		PC += _length; \
	}
#define CLR_M(p) \
	{ \
		uint8 _m; \
		_CLR(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define CLV() \
	{ \
		CC = C | Z | N | I | H; \
		PC += _length; \
	}

#define CMP(a, m) \
	{ \
		uint32 _acc = (uint32 )(a) - (m); \
		CC = SET_CS(_acc) | SET_VS(_acc, a, m) | SET_Z(_acc) | SET_N(_acc) | I | H; \
		PC += _length; \
	}

#define _COM(a) \
	{ \
		uint32 _acc = ~(a); \
		CC = MASK_C | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc; \
	}
#define COM_R(a) \
	{ \
		_COM(a); \
		PC += _length; \
	}
#define COM_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_COM(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define CPX(p) \
	{ \
		uint32 _h = (X >> 8) - LOAD((p) + 0); \
		uint32 _l = (X & 0xff) - LOAD((p) + 1); \
		CC = C | SET_VS(_h, X >> 8, LOAD((p) + 0)) | (_h == 0 && _l == 0 ? MASK_Z: 0) | SET_N(_h) | I | H; \
		PC += _length; \
	}

#define DAA() \
	{ \
		uint32 _acc; \
		uint8 _x, _c; \
		daa_result(&_x, &_c, A, CC); \
		_acc = (uint32 )A + _x; \
		CC = _c | SET_V(_acc, A, _x) | SET_Z(_acc) | SET_N(_acc) | I | H; \
		A = _acc; \
		PC += _length; \
	}

#define _DEC(a) \
	{ \
		uint32 _acc = (uint32 )(a) - 1; \
		CC = C | SET_VS(_acc, a, 1) | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc; \
	}
#define DEC_R(a) \
	{ \
		_DEC(a); \
		PC += _length; \
	}
#define DEC_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_DEC(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define DES() \
	{ \
		SP--; \
		PC += _length; \
	}

#define DEX() \
	{ \
		X--; \
		CC = C | V | SET_Z16(X) | N | I | H; \
		PC += _length; \
	}

#define EOR(a, m) \
	{ \
		uint32 _acc = (a) ^ (m); \
		CC = C | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc; \
		PC += _length; \
	}

#define _INC(a) \
	{ \
		uint32 _acc = (uint32 )(a) + 1; \
		CC = C | SET_V(_acc, a, 1) | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc; \
	}
#define INC_R(a) \
	{ \
		_INC(a); \
		PC += _length; \
	}
#define INC_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_INC(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define INS() \
	{ \
		SP++; \
		PC += _length; \
	}

#define INX() \
	{ \
		X++; \
		CC = C | V | SET_Z16(X) | N | I | H; \
		PC += _length; \
	}

#define JMP(p) \
	{ \
		int s; \
		PC = (p); \
		if(m68->emulate_subroutine && (s = m68subroutine(m68, PC)) >= 0) { \
			SP++; \
			PC = (uint16 )LOAD(SP) << 8; \
			SP++; \
			PC = PC | LOAD(SP); \
			_states += s; \
		} \
	}

#define JSR(p) \
	{ \
		int s; \
		uint16 _pc = PC + _length; \
		STORE(SP, _pc & 0x00ff); \
		SP--; \
		STORE(SP, _pc >> 8); \
		SP--; \
		PC = (p); \
		if(m68->emulate_subroutine && (s = m68subroutine(m68, PC)) >= 0) { \
			SP++; \
			PC = (uint16 )LOAD(SP) << 8; \
			SP++; \
			PC = PC | LOAD(SP); \
			_states += s; \
		} \
	}

#define LDA(a, m) \
	{ \
		uint32 _acc = (m); \
		CC = C | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc;  \
		PC += _length; \
	}

#define LDS(p) \
	{ \
		SP = LOAD16(p); \
		CC = C | SET_Z16(SP) | SET_N16(SP) | I | H; \
		PC += _length; \
	}

#define LDX(p) \
	{ \
		X = LOAD16(p); \
		CC = C | SET_Z16(X) | SET_N16(X) | I | H; \
		PC += _length; \
	}

#define _LSR(a) \
	{ \
		uint32 _acc = (uint32 )(a) >> 1; \
		uint8 _c = (a) & 0x01; \
		uint8 _n = SET_N(_acc); \
		CC = _c | SET_VR(_c, _n) | SET_Z(_acc) | I | H; \
		(a) = _acc; \
	}
#define LSR_R(a) \
	{ \
		_LSR(a); \
		PC += _length; \
	}
#define LSR_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_LSR(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define _NEG(a) \
	{ \
		uint32 _acc = -(uint32 )(a); \
		CC = (_acc != 0x00 ? MASK_C: 0) | (_acc == 0x80 ? MASK_V: 0) | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc; \
	}
#define NEG_R(a) \
	{ \
		_NEG(a); \
		PC += _length; \
	}
#define NEG_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_NEG(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define NOP() \
	{ \
		PC += _length; \
	}

#define ORA(a, m) \
	{ \
		uint32 _acc = (a) | (m); \
		CC = C | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc; \
		PC += _length; \
	}

#define PSH(a) \
	{ \
		STORE(SP, a); \
		SP--; \
		PC += _length; \
	}

#define PUL(a) \
	{ \
		SP++; \
		(a) = LOAD(SP); \
		PC += _length; \
	}

#define _ROL(a) \
	{ \
		uint32 _acc = ((uint32 )(a) << 1) | C; \
		uint8 _c = ((a) & 0x80 ? MASK_C: 0); \
		uint8 _n = SET_N(_acc); \
		CC = _c | SET_VR(_c, _n) | SET_Z(_acc) | _n | I | H; \
		(a) = _acc; \
	}
#define ROL_R(a) \
	{ \
		_ROL(a); \
		PC += _length; \
	}
#define ROL_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_ROL(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define _ROR(a) \
	{ \
		uint32 _acc = ((uint32 )(a) >> 1) | (C ? 0x80: 0); \
		uint8 _c = (a) & 0x01; \
		uint8 _n = SET_N(_acc); \
		CC = _c | SET_VR(_c, _n) | SET_Z(_acc) | _n | I | H; \
		(a) = _acc; \
	}
#define ROR_R(a) \
	{ \
		_ROR(a); \
		PC += _length; \
	}
#define ROR_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_ROR(_m); \
		STORE(p, _m); \
		PC += _length; \
	}

#define RTI() \
	{ \
		SP++; \
		CC = LOAD(SP) & 0x3f; \
		SP++; \
		B = LOAD(SP); \
		SP++; \
		A = LOAD(SP); \
		SP++; \
		X = LOAD(SP) << 8; \
		SP++; \
		X = X | LOAD(SP); \
		SP++; \
		PC = LOAD(SP) << 8; \
		SP++; \
		PC = PC | LOAD(SP); \
	}

#define RTS() \
	{ \
		SP++; \
		PC = (uint16 )LOAD(SP) << 8; \
		SP++; \
		PC = PC | LOAD(SP); \
	}

#define SBA() \
	{ \
		uint32 _acc = (uint32 )A - B; \
		CC = SET_CS(_acc) | SET_VS(_acc, A, B) | SET_Z(_acc) | SET_N(_acc) | I | H; \
		A = _acc; \
		PC += _length; \
	}

#define SBC(a, m) \
	{ \
		uint32 _acc = (uint32 )(a) - (m) - C; \
		CC = SET_CS(_acc) | SET_VS(_acc, a, m) | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc; \
		PC += _length; \
	}

#define SEC() \
	{ \
		CC = MASK_C | V | Z | N | I | H; \
		PC += _length; \
	}

#define SEI() \
	{ \
		CC = C | V | Z | N | MASK_I | H; \
		PC += _length; \
	}

#define SEV() \
	{ \
		CC = C | MASK_V | Z | N | I | H; \
		PC += _length; \
	}

#define STA(a, p) \
	{ \
		uint32 _acc = a; \
		CC = C | SET_Z(_acc) | SET_N(_acc) | I | H; \
		STORE(p, _acc); \
		PC += _length; \
	}

#define STS(p) \
	{ \
		STORE16(p, SP); \
		CC = C | SET_Z16(SP) | SET_N16(SP) | I | H; \
		PC += _length; \
	}

#define STX(p) \
	{ \
		STORE16(p, X); \
		CC = C | SET_Z16(X) | SET_N16(X) | I | H; \
		PC += _length; \
	}

#define SUB(a, m) \
	{ \
		uint32 _acc = (uint32 )(a) - (m); \
		CC = SET_CS(_acc) | SET_VS(_acc, a, m) | SET_Z(_acc) | SET_N(_acc) | I | H; \
		(a) = _acc; \
		PC += _length; \
	}

#define SWI() \
	{ \
		uint16 _pc = PC + _length; \
		STORE(SP, _pc & 0xff); \
		SP--; \
		STORE(SP, _pc >> 8); \
		SP--; \
		STORE(SP, X & 0xff); \
		SP--; \
		STORE(SP, X >> 8); \
		SP--; \
		STORE(SP, A); \
		SP--; \
		STORE(SP, B); \
		SP--; \
		STORE(SP, CC | 0xc0); \
		SP--; \
		CC = C | V | Z | N | MASK_I | H; \
		PC = LOAD16(0xfffa); \
	}

#define TAB() \
	{ \
		uint32 _acc = A; \
		CC = C | SET_Z(_acc) | SET_N(_acc) | I | H; \
		B = _acc; \
		PC += _length; \
	}

#define TAP() \
	{ \
		CC = A & 0x3f; \
		PC += _length; \
	}

#define TBA() \
	{ \
		uint32 _acc = B; \
		CC = C | SET_Z(_acc) | SET_N(_acc) | I | H; \
		A = _acc; \
		PC += _length; \
	}

#define TPA() \
	{ \
		A = CC | 0xc0; \
		PC += _length; \
	}

#define _TST(a) \
	{ \
		uint32 _acc = (a); \
		CC = SET_Z(_acc) | SET_N(_acc) | I | H; \
	}
#define TST_R(a) \
	{ \
		_TST(a); \
		PC += _length; \
	}
#define TST_M(p) \
	{ \
		uint8 _m = LOAD(p); \
		_TST(_m); \
		PC += _length; \
	}

#define TSX() \
	{ \
		X = SP + 1; \
		PC += _length; \
	}

#define TXS() \
	{ \
		SP = X - 1; \
		PC += _length; \
	}

#define WAI() \
	{ \
		uint16 _pc = PC + _length; \
		STORE(SP, _pc >> 8); \
		SP--; \
		STORE(SP, _pc & 0xff); \
		SP--; \
		STORE(SP, X >> 8); \
		SP--; \
		STORE(SP, X & 0xff); \
		SP--; \
		STORE(SP, A); \
		SP--; \
		STORE(SP, B); \
		SP--; \
		STORE(SP, CC | 0xc0); \
		SP--; \
		m68->wai = 1; \
		PC = _pc; \
	}

/* 実行時間表 */
static const int states[] = {
/*      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f */
	2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2,	/* 00 */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 10 */
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,	/* 20 */
	4, 3, 4, 4, 4, 4, 4, 4, 4, 5, 4, 10,4, 4, 9,12,	/* 30 */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 40 */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 50 */
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 4, 7,	/* 60 */
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 3, 6,	/* 70 */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 8, 3, 2,	/* 80 */
	3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3, 3, 4, 3, 4, 5,	/* 90 */
	5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 6, 8, 6, 7,	/* a0 */
	4, 4, 4, 4, 4, 4, 4, 5, 4, 4, 4, 4, 5, 4, 5, 6,	/* b0 */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2,	/* c0 */
	3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 4, 5,	/* d0 */
	5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6, 7,	/* e0 */
	4, 4, 4, 0, 4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 5, 6	/* f0 */
};

/* 命令長表 */
static const int length[] = {
/*      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f */
	0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* 00 */
	1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0,	/* 10 */
	2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* 20 */
	1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1,	/* 30 */
	1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1,	/* 40 */
	1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1,	/* 50 */
	2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2,	/* 60 */
	3, 0, 0, 3, 3, 0, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3,	/* 70 */
	2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 3, 2, 3, 0,	/* 80 */
	2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 2,	/* 90 */
	2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* a0 */
	3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	/* b0 */
	2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 0, 0, 3, 0,	/* c0 */
	2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2,	/* d0 */
	2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2,	/* e0 */
	3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 3, 3	/* f0 */
};

/*
	DAA実行後にAに加算される値とキャリーを求める (下請け)
*/
static inline void daa_result(uint8 *x, uint8 *c, uint8 a, uint8 cc)
{
	switch(cc & (MASK_C | MASK_H)) {
	case 0:
		if(a < 0x9a) {
			if((a & 0x0f) < 0x0a)
				*x = 0x00, *c = 0;
			else
				*x = 0x06, *c = 0;
		} else {
			if((a & 0x0f) < 0x0a)
				*x = 0x60, *c = MASK_C;
			else
				*x = 0x66, *c = MASK_C;
		}
		break;
	case MASK_C:
		if((a & 0x0f) < 0x0a)
			*x = 0x60, *c = MASK_C;
		else
			*x = 0x66, *c = MASK_C;
		break;
	case MASK_H:
		if(a < 0x9a)
			*x = 0x06, *c = 0;
		else
			*x = 0x66, *c = MASK_C;
		break;
	case MASK_C | MASK_H:
		*x = 0x66, *c = MASK_C;
		break;
	default:
		*x = 0, *c = 0;
		break;
	}
}

/*
	M6800エミュレータを初期化する
*/
struct M68stat m68init(void *memory, void *tag)
{
	struct M68stat m68;

	m68.m = memory;
	m68.sp = 0xffff;
	m68.pc = 0xffff;
	m68.x = 0xffff;
	m68.a = 0xff;
	m68.b = 0xff;
	m68.cc = MASK_C | MASK_V | MASK_Z | MASK_N | MASK_I | MASK_H;
	m68.wai = 0;
	m68.states = 0;
	m68.total_states = 0;
	m68.emulate_subroutine = 0;
	m68.trace = 0;
	m68.tag = tag;
	return m68;
}

/*
	累積ステート数を得る
*/
int m68states(const struct M68stat *m68)
{
	return m68->total_states - m68->states;
}

/*
	ステート数の差を得る
*/
int m68diff(int x, int y)
{
	x &= 0x7fffffff;
	y &= 0x7fffffff;

	if(x < y && y - x > 0x40000000)
		return 0x80000000 - y + x;
	else
		return x - y;
}

/*
	リセット信号を送る
*/
int m68reset(struct M68stat *m68)
{
	int s;

	m68->cc |= MASK_I;
	m68->pc = m68read16(m68, 0xfffe);
	m68->wai = 0;

	if(m68->emulate_subroutine)
		s = m68subroutine(m68, m68->pc);
	return TRUE;
}

/*
	NMI信号を送る
*/
int m68nmi(struct M68stat *m68)
{
	int s;

	m68write8(m68, m68->sp, m68->pc & 0xff);
	m68->sp--;
	m68write8(m68, m68->sp, m68->pc >> 8);
	m68->sp--;
	m68write8(m68, m68->sp, m68->x & 0xff);
	m68->sp--;
	m68write8(m68,m68->sp, m68->x >> 8);
	m68->sp--;
	m68write8(m68, m68->sp, m68->a);
	m68->sp--;
	m68write8(m68, m68->sp, m68->b);
	m68->sp--;
	m68write8(m68, m68->sp, m68->cc | 0xc0);
	m68->sp--;

	m68->cc |= MASK_I;
	m68->pc = m68read16(m68, 0xfffc);
	m68->wai = 0;

	if(m68->emulate_subroutine && (s = m68subroutine(m68, m68->pc)) >= 0) {
		m68->sp++;
		m68->cc = m68read8(m68, m68->sp) & 0x3f;
		m68->sp++;
		m68->b = m68read8(m68, m68->sp);
		m68->sp++;
		m68->a = m68read8(m68, m68->sp);
		m68->sp++;
		m68->x = (uint16 )m68read8(m68, m68->sp) << 8;
		m68->sp++;
		m68->x = m68->x | m68read8(m68, m68->sp);
		m68->sp++;
		m68->pc = (uint16 )m68read8(m68, m68->sp) << 8;
		m68->sp++;
		m68->pc = m68->pc | m68read8(m68, m68->sp);
	}
	return TRUE;
}

/*
	IRQ信号を送る
*/
int m68irq(struct M68stat *m68)
{
	int s;

	if(m68->cc & MASK_I)
		return FALSE;

	m68write8(m68, m68->sp, m68->pc & 0xff);
	m68->sp--;
	m68write8(m68, m68->sp, m68->pc >> 8);
	m68->sp--;
	m68write8(m68, m68->sp, m68->x & 0xff);
	m68->sp--;
	m68write8(m68,m68->sp, m68->x >> 8);
	m68->sp--;
	m68write8(m68, m68->sp, m68->a);
	m68->sp--;
	m68write8(m68, m68->sp, m68->b);
	m68->sp--;
	m68write8(m68, m68->sp, m68->cc | 0xc0);
	m68->sp--;

	m68->cc |= MASK_I;
	m68->pc = m68read16(m68, 0xfff8);
	m68->wai = 0;

	if(m68->emulate_subroutine && (s = m68subroutine(m68, m68->pc)) >= 0) {
		m68->sp++;
		m68->cc = m68read8(m68, m68->sp) & 0x3f;
		m68->sp++;
		m68->b = m68read8(m68, m68->sp);
		m68->sp++;
		m68->a = m68read8(m68, m68->sp);
		m68->sp++;
		m68->x = (uint16 )m68read8(m68, m68->sp) << 8;
		m68->sp++;
		m68->x = m68->x | m68read8(m68, m68->sp);
		m68->sp++;
		m68->pc = (uint16 )m68read8(m68, m68->sp) << 8;
		m68->sp++;
		m68->pc = m68->pc | m68read8(m68, m68->sp);
	}
	return TRUE;
}

/*
	プログラムを実行する
*/
int m68exec(struct M68stat *m68)
{
	int _states;
	int _length;
	uint8 _op;

	m68->total_states = (m68->total_states & 0x7fffffff) + m68->states;

	if(m68->wai) {
		m68->states = 0;
		return M68_WAI;
	}

	do {
		_op = m68read8(m68, PC);
		_length = length[_op];
		_states = states[_op];

#if defined(M68_TRACE)
		if(m68->trace)
			m68log(m68);
#endif

		switch(_op) {
		case 0x01: NOP(); break;	/* NOP */
		case 0x06: TAP(); break;	/* TAP */
		case 0x07: TPA(); break;	/* TPA */
		case 0x08: INX(); break;	/* INX */
		case 0x09: DEX(); break;	/* DEX */
		case 0x0a: CLV(); break;	/* CLV */
		case 0x0b: SEV(); break;	/* SEV */
		case 0x0c: CLC(); break;	/* CLC */
		case 0x0d: SEC(); break;	/* SEC */
		case 0x0e: CLI(); break;	/* CLI */
		case 0x0f: SEI(); break;	/* SEI */

		case 0x10: SBA(); break;	/* SBA */
		case 0x11: CBA(); break;	/* CBA */
		case 0x16: TAB(); break;	/* TAB */
		case 0x17: TBA(); break;	/* TBA */
		case 0x19: DAA(); break;	/* DAA */
		case 0x1b: ABA(); break;	/* ABA */

		case 0x20: BRA(RELADR); break;	/* BRA */
		case 0x22: BHI(RELADR); break;	/* BHI */
		case 0x23: BLS(RELADR); break;	/* BLS */
		case 0x24: BCC(RELADR); break;	/* BCC */
		case 0x25: BCS(RELADR); break;	/* BCS */
		case 0x26: BNE(RELADR); break;	/* BNE */
		case 0x27: BEQ(RELADR); break;	/* BEQ */
		case 0x28: BVC(RELADR); break;	/* BVC */
		case 0x29: BVS(RELADR); break;	/* BVS */
		case 0x2a: BPL(RELADR); break;	/* BPL */
		case 0x2b: BMI(RELADR); break;	/* BMI */
		case 0x2c: BGE(RELADR); break;	/* BGE */
		case 0x2d: BLT(RELADR); break;	/* BLT */
		case 0x2e: BGT(RELADR); break;	/* BGT */
		case 0x2f: BLE(RELADR); break;	/* BLE */

		case 0x30: TSX(); break;	/* TSX */
		case 0x31: INS(); break;	/* INS */
		case 0x32: PUL(A); break;	/* PUL A */
		case 0x33: PUL(B); break;	/* PUL B */
		case 0x34: DES(); break;	/* DES */
		case 0x35: TXS(); break;	/* TXS */
		case 0x36: PSH(A); break;	/* PSH A */
		case 0x37: PSH(B); break;	/* PSH B */
		case 0x39: RTS(); break;	/* RTS */
		case 0x3b: RTI(); break;	/* RTI */
		case 0x3e: WAI(); m68->total_states -= m68->states; m68->states = 0; return M68_WAI;	/* WAI */
		case 0x3f: SWI(); break;	/* SWI */

		case 0x40: NEG_R(A); break;	/* NEG A */
		case 0x43: COM_R(A); break;	/* COM A */
		case 0x44: LSR_R(A); break;	/* LSR A */
		case 0x46: ROR_R(A); break;	/* ROR A */
		case 0x47: ASR_R(A); break;	/* ASR A */
		case 0x48: ASL_R(A); break;	/* ASL A */
		case 0x49: ROL_R(A); break;	/* ROL A */
		case 0x4a: DEC_R(A); break;	/* DEC A */
		case 0x4c: INC_R(A); break;	/* INC A */
		case 0x4d: TST_R(A); break;	/* TST A */
		case 0x4f: CLR_R(A); break;	/* CLR A */

		case 0x50: NEG_R(B); break;	/* NEG B */
		case 0x53: COM_R(B); break;	/* COM B */
		case 0x54: LSR_R(B); break;	/* LSR B */
		case 0x56: ROR_R(B); break;	/* ROR B */
		case 0x57: ASR_R(B); break;	/* ASR B */
		case 0x58: ASL_R(B); break;	/* ASL B */
		case 0x59: ROL_R(B); break;	/* ROL B */
		case 0x5a: DEC_R(B); break;	/* DEC B */
		case 0x5c: INC_R(B); break;	/* INC B */
		case 0x5d: TST_R(B); break;	/* TST B */
		case 0x5f: CLR_R(B); break;	/* CLR B */

		case 0x60: NEG_M(INDADR); break;	/* NEG (indexed address) */
		case 0x63: COM_M(INDADR); break;	/* COM (indexed address) */
		case 0x64: LSR_M(INDADR); break;	/* LSR (indexed address) */
		case 0x66: ROR_M(INDADR); break;	/* ROR (indexed address) */
		case 0x67: ASR_M(INDADR); break;	/* ASR (indexed address) */
		case 0x68: ASL_M(INDADR); break;	/* ASL (indexed address) */
		case 0x69: ROL_M(INDADR); break;	/* ROL (indexed address) */
		case 0x6a: DEC_M(INDADR); break;	/* DEC (indexed address) */
		case 0x6c: INC_M(INDADR); break;	/* INC (indexed address) */
		case 0x6d: TST_M(INDADR); break;	/* TST (indexed address) */
		case 0x6e: JMP(INDADR); break;	/* JMP (indexed address) */
		case 0x6f: CLR_M(INDADR); break;	/* CLR (indexed address) */

		case 0x70: NEG_M(EXTADR); break;	/* NEG (extended address) */
		case 0x73: COM_M(EXTADR); break;	/* COM (extended address) */
		case 0x74: LSR_M(EXTADR); break;	/* LSR (extended address) */
		case 0x76: ROR_M(EXTADR); break;	/* ROR (extended address) */
		case 0x77: ASR_M(EXTADR); break;	/* ASR (extended address) */
		case 0x78: ASL_M(EXTADR); break;	/* ASL (extended address) */
		case 0x79: ROL_M(EXTADR); break;	/* ROL (extended address) */
		case 0x7a: DEC_M(EXTADR); break;	/* DEC (extended address) */
		case 0x7c: INC_M(EXTADR); break;	/* INC (extended address) */
		case 0x7d: TST_M(EXTADR); break;	/* TST (extended address) */
		case 0x7e: JMP(EXTADR); break;	/* JMP (extended address) */
		case 0x7f: CLR_M(EXTADR); break;	/* CLR (extended address) */

		case 0x80: SUB(A, IMM); break;	/* SUB (A immediate) */
		case 0x81: CMP(A, IMM); break;	/* CMP (A immediate) */
		case 0x82: SBC(A, IMM); break;	/* SBC (A immediate) */
		case 0x84: AND(A, IMM); break;	/* AND (A immediate) */
		case 0x85: BIT(A, IMM); break;	/* BIT (A immediate) */
		case 0x86: LDA(A, IMM); break;	/* LDA (A immediate) */
		case 0x88: EOR(A, IMM); break;	/* EOR (A immediate) */
		case 0x89: ADC(A, IMM); break;	/* ADC (A immediate) */
		case 0x8a: ORA(A, IMM); break;	/* ORA (A immediate) */
		case 0x8b: ADD(A, IMM); break;	/* ADD (A immediate) */
		case 0x8c: CPX(IMMADR); break;	/* CPX (immediate address) */
		case 0x8d: BSR(RELADR); break;	/* BSR */
		case 0x8e: LDS(IMMADR); break;	/* LDS (immediate address) */

		case 0x90: SUB(A, DIR); break;	/* SUB (A direct) */
		case 0x91: CMP(A, DIR); break;	/* CMP (A direct) */
		case 0x92: SBC(A, DIR); break;	/* SBC (A direct) */
		case 0x94: AND(A, DIR); break;	/* AND (A direct) */
		case 0x95: BIT(A, DIR); break;	/* BIT (A direct) */
		case 0x96: LDA(A, DIR); break;	/* LDA (A direct) */
		case 0x97: STA(A, DIRADR); break;	/* STA (A direct) */
		case 0x98: EOR(A, DIR); break;	/* EOR (A direct) */
		case 0x99: ADC(A, DIR); break;	/* ADC (A direct) */
		case 0x9a: ORA(A, DIR); break;	/* ORA (A direct) */
		case 0x9b: ADD(A, DIR); break;	/* ADD (A direct) */
		case 0x9c: CPX(DIRADR); break;	/* CPX (direct address) */
		case 0x9e: LDS(DIRADR); break;	/* LDS (direct address) */
		case 0x9f: STS(DIRADR); break;	/* STS (direct address) */

		case 0xa0: SUB(A, IND); break;	/* SUB (A indexed) */
		case 0xa1: CMP(A, IND); break;	/* CMP (A indexed) */
		case 0xa2: SBC(A, IND); break;	/* SBC (A indexed) */
		case 0xa4: AND(A, IND); break;	/* AND (A indexed) */
		case 0xa5: BIT(A, IND); break;	/* BIT (A indexed) */
		case 0xa6: LDA(A, IND); break;	/* LDA (A indexed) */
		case 0xa7: STA(A, INDADR); break;	/* STA (A indexed) */
		case 0xa8: EOR(A, IND); break;	/* EOR (A indexed) */
		case 0xa9: ADC(A, IND); break;	/* ADC (A indexed) */
		case 0xaa: ORA(A, IND); break;	/* ORA (A indexed) */
		case 0xab: ADD(A, IND); break;	/* ADD (A indexed) */
		case 0xac: CPX(INDADR); break;	/* CPX (indexed address) */
		case 0xad: JSR(INDADR); break;	/* JSR (indexed address) */
		case 0xae: LDS(INDADR); break;	/* LDS (indexed address) */
		case 0xaf: STS(INDADR); break;	/* STS (indexed address) */

		case 0xb0: SUB(A, EXT); break;	/* SUB (A extended) */
		case 0xb1: CMP(A, EXT); break;	/* CMP (A extended) */
		case 0xb2: SBC(A, EXT); break;	/* SBC (A extended) */
		case 0xb4: AND(A, EXT); break;	/* AND (A extended) */
		case 0xb5: BIT(A, EXT); break;	/* BIT (A extended) */
		case 0xb6: LDA(A, EXT); break;	/* LDA (A extended) */
		case 0xb7: STA(A, EXTADR); break;	/* STA (A extended) */
		case 0xb8: EOR(A, EXT); break;	/* EOR (A extended) */
		case 0xb9: ADC(A, EXT); break;	/* ADC (A extended) */
		case 0xba: ORA(A, EXT); break;	/* ORA (A extended) */
		case 0xbb: ADD(A, EXT); break;	/* ADD (A extended) */
		case 0xbc: CPX(EXTADR); break;	/* CPX (extended address) */
		case 0xbd: JSR(EXTADR); break;	/* JSR (extended address) */
		case 0xbe: LDS(EXTADR); break;	/* LDS (extended address) */
		case 0xbf: STS(EXTADR); break;	/* STS (extended address) */

		case 0xc0: SUB(B, IMM); break;	/* SUB (B immediate) */
		case 0xc1: CMP(B, IMM); break;	/* CMP (B immediate) */
		case 0xc2: SBC(B, IMM); break;	/* SBC (B immediate) */
		case 0xc4: AND(B, IMM); break;	/* AND (B immediate) */
		case 0xc5: BIT(B, IMM); break;	/* BIT (B immediate) */
		case 0xc6: LDA(B, IMM); break;	/* LDA (B immediate) */
		case 0xc8: EOR(B, IMM); break;	/* EOR (B immediate) */
		case 0xc9: ADC(B, IMM); break;	/* ADC (B immediate) */
		case 0xca: ORA(B, IMM); break;	/* ORA (B immediate) */
		case 0xcb: ADD(B, IMM); break;	/* ADD (B immediate) */
		case 0xce: LDX(IMMADR); break;	/* LDX (immediate address) */

		case 0xd0: SUB(B, DIR); break;	/* SUB (B direct) */
		case 0xd1: CMP(B, DIR); break;	/* CMP (B direct) */
		case 0xd2: SBC(B, DIR); break;	/* SBC (B direct) */
		case 0xd4: AND(B, DIR); break;	/* AND (B direct) */
		case 0xd5: BIT(B, DIR); break;	/* BIT (B direct) */
		case 0xd6: LDA(B, DIR); break;	/* LDA (B direct) */
		case 0xd7: STA(B, DIRADR); break;	/* STA (B direct) */
		case 0xd8: EOR(B, DIR); break;	/* EOR (B direct) */
		case 0xd9: ADC(B, DIR); break;	/* ADC (B direct) */
		case 0xda: ORA(B, DIR); break;	/* ORA (B direct) */
		case 0xdb: ADD(B, DIR); break;	/* ADD (B direct) */
		case 0xde: LDX(DIRADR); break;	/* LDX (direct address) */
		case 0xdf: STX(DIRADR); break;	/* STX (direct address) */

		case 0xe0: SUB(B, IND); break;	/* SUB (B indexed) */
		case 0xe1: CMP(B, IND); break;	/* CMP (B indexed) */
		case 0xe2: SBC(B, IND); break;	/* SBC (B indexed) */
		case 0xe4: AND(B, IND); break;	/* AND (B indexed) */
		case 0xe5: BIT(B, IND); break;	/* BIT (B indexed) */
		case 0xe6: LDA(B, IND); break;	/* LDA (B indexed) */
		case 0xe7: STA(B, INDADR); break;	/* STA (B indexed) */
		case 0xe8: EOR(B, IND); break;	/* EOR (B indexed) */
		case 0xe9: ADC(B, IND); break;	/* ADC (B indexed) */
		case 0xea: ORA(B, IND); break;	/* ORA (B indexed) */
		case 0xeb: ADD(B, IND); break;	/* ADD (B indexed) */
		case 0xee: LDX(INDADR); break;	/* LDX (indexed address) */
		case 0xef: STX(INDADR); break;	/* STX (indexed address) */

		case 0xf0: SUB(B, EXT); break;	/* SUB (B extended) */
		case 0xf1: CMP(B, EXT); break;	/* CMP (B extended) */
		case 0xf2: SBC(B, EXT); break;	/* SBC (B extended) */
		case 0xf4: AND(B, EXT); break;	/* AND (B extended) */
		case 0xf5: BIT(B, EXT); break;	/* BIT (B extended) */
		case 0xf6: LDA(B, EXT); break;	/* LDA (B extended) */
		case 0xf7: STA(B, EXTADR); break;	/* STA (B extended) */
		case 0xf8: EOR(B, EXT); break;	/* EOR (B extended) */
		case 0xf9: ADC(B, EXT); break;	/* ADC (B extended) */
		case 0xfa: ORA(B, EXT); break;	/* ORA (B extended) */
		case 0xfb: ADD(B, EXT); break;	/* ADD (B extended) */
		case 0xfe: LDX(EXTADR); break;	/* LDX (extended address) */
		case 0xff: STX(EXTADR); break;	/* STX (extended address) */
		/*default: m68->total_states -= m68->states; m68->states = 0; return M68_RUN;*/
		default: PC++; break;	/* ??? */
		}
		m68->states -= _states;
	} while(m68->states > 0);

	m68->total_states -= m68->states;
	return M68_RUN;
}

/*
	Copyright 2008, 2014 maruhiro
	All rights reserved. 

	Redistribution and use in source and binary forms, 
	with or without modification, are permitted provided that 
	the following conditions are met: 

	 1. Redistributions of source code must retain the above copyright notice, 
	    this list of conditions and the following disclaimer. 

	 2. Redistributions in binary form must reproduce the above copyright notice, 
	    this list of conditions and the following disclaimer in the documentation 
	    and/or other materials provided with the distribution. 

	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, 
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
	FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
	THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
	OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* eof */
