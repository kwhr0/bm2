/*
	Motorola M6800 Emulator Disassembler (m6800asm.c)
*/

#include <stdio.h>
#include "m6800.h"

#define CODE_NONE	0x00
#define CODE_ABA	0x01
#define CODE_ADC	0x02
#define CODE_ADD	0x03
#define CODE_AND	0x04
#define CODE_ASL	0x05
#define CODE_ASR	0x06
#define CODE_BCC	0x07
#define CODE_BCS	0x08
#define CODE_BEQ	0x09
#define CODE_BGE	0x0a
#define CODE_BGT	0x0b
#define CODE_BHI	0x0c
#define CODE_BLE	0x0d
#define CODE_BLS	0x0e
#define CODE_BLT	0x0f
#define CODE_BMI	0x10
#define CODE_BNE	0x11
#define CODE_BPL	0x12
#define CODE_BRA	0x13
#define CODE_BVC	0x14
#define CODE_BVS	0x15
#define CODE_BSR	0x16
#define CODE_BIT	0x17
#define CODE_CBA	0x18
#define CODE_CLC	0x19
#define CODE_CLI	0x1a
#define CODE_CLR	0x1b
#define CODE_CLV	0x1c
#define CODE_CMP	0x1d
#define CODE_COM	0x1e
#define CODE_CPX	0x1f
#define CODE_DAA	0x20
#define CODE_DEC	0x21
#define CODE_DES	0x22
#define CODE_DEX	0x23
#define CODE_EOR	0x24
#define CODE_INC	0x25
#define CODE_INS	0x26
#define CODE_INX	0x27
#define CODE_JMP	0x28
#define CODE_JSR	0x29
#define CODE_LDA	0x2a
#define CODE_LDS	0x2b
#define CODE_LDX	0x2c
#define CODE_LSR	0x2d
#define CODE_NEG	0x2e
#define CODE_NOP	0x2f
#define CODE_ORA	0x30
#define CODE_PSH	0x31
#define CODE_PUL	0x32
#define CODE_ROL	0x33
#define CODE_ROR	0x34
#define CODE_RTI	0x35
#define CODE_RTS	0x36
#define CODE_SBA	0x37
#define CODE_SBC	0x38
#define CODE_SEC	0x39
#define CODE_SEI	0x3a
#define CODE_SEV	0x3b
#define CODE_STA	0x3c
#define CODE_STS	0x3d
#define CODE_STX	0x3e
#define CODE_SUB	0x3f
#define CODE_SWI	0x40
#define CODE_TAB	0x41
#define CODE_TAP	0x42
#define CODE_TBA	0x43
#define CODE_TPA	0x44
#define CODE_TST	0x45
#define CODE_TSX	0x46
#define CODE_TXS	0x47
#define CODE_WAI	0x48
#define MASK_CODE	0x007f
#define SHIFT_CODE	0

#define MODE_A_IMM	0x0080
#define MODE_A_DIR	0x0100
#define MODE_A_EXT	0x0180
#define MODE_A_IND	0x0200
#define MODE_B_IMM	0x0280
#define MODE_B_DIR	0x0300
#define MODE_B_EXT	0x0380
#define MODE_B_IND	0x0400
#define MODE_REL	0x0480
#define MODE_A	0x0500
#define MODE_B	0x0580
#define MODE_IMMADR	0x0600
#define MODE_DIRADR	0x0680
#define MODE_EXTADR	0x0700
#define MODE_INDADR	0x0780
#define MODE_NONE	0x0800
#define MASK_MODE	0x0f80
#define SHIFT_MODE	7

#define NONE	(CODE_NONE | MODE_NONE)

const static uint16 op_table[] = {
	/* 00 */
	NONE,
	CODE_NOP,
	NONE,
	NONE,
	NONE,
	NONE,
	CODE_TAP,
	CODE_TPA,
	
	/* 08 */
	CODE_INX,
	CODE_DEX,
	CODE_CLV,
	CODE_SEV,
	CODE_CLC,
	CODE_SEC,
	CODE_CLI,
	CODE_SEI,

	/* 10 */
	CODE_SBA,
	CODE_CBA,
	NONE,
	NONE,
	NONE,
	NONE,
	CODE_TAB,
	CODE_TBA,

	/* 18 */
	NONE,
	CODE_DAA,
	NONE,
	CODE_ABA,
	NONE,
	NONE,
	NONE,
	NONE,

	/* 20 */
	CODE_BRA | MODE_REL,
	NONE,
	CODE_BHI | MODE_REL,
	CODE_BLS | MODE_REL,
	CODE_BCC | MODE_REL,
	CODE_BCS | MODE_REL,
	CODE_BNE | MODE_REL,
	CODE_BEQ | MODE_REL,
	
	/* 28 */
	CODE_BVC | MODE_REL,
	CODE_BVS | MODE_REL,
	CODE_BPL | MODE_REL,
	CODE_BMI | MODE_REL,
	CODE_BGE | MODE_REL,
	CODE_BLT | MODE_REL,
	CODE_BGT | MODE_REL,
	CODE_BLE | MODE_REL,

	/* 30 */
	CODE_TSX,
	CODE_INS,
	CODE_PUL | MODE_A,
	CODE_PUL | MODE_B,
	CODE_DES,
	CODE_TXS,
	CODE_PSH | MODE_A,
	CODE_PSH | MODE_B,

	/* 38 */
	NONE,
	CODE_RTS,
	NONE,
	CODE_RTI,
	NONE,
	NONE,
	CODE_WAI,
	CODE_SWI,

	/* 40 */
	CODE_NEG | MODE_A,
	NONE,
	NONE,
	CODE_COM | MODE_A,
	CODE_LSR | MODE_A,
	NONE,
	CODE_ROR | MODE_A,
	CODE_ASR | MODE_A,
	
	/* 48 */
	CODE_ASL | MODE_A,
	CODE_ROL | MODE_A,
	CODE_DEC | MODE_A,
	NONE,
	CODE_INC | MODE_A,
	CODE_TST | MODE_A,
	NONE,
	CODE_CLR | MODE_A,

	/* 50 */
	CODE_NEG | MODE_B,
	NONE,
	NONE,
	CODE_COM | MODE_B,
	CODE_LSR | MODE_B,
	NONE,
	CODE_ROR | MODE_B,
	CODE_ASR | MODE_B,

	/* 58 */
	CODE_ASL | MODE_B,
	CODE_ROL | MODE_B,
	CODE_DEC | MODE_B,
	NONE,
	CODE_INC | MODE_B,
	CODE_TST | MODE_B,
	NONE,
	CODE_CLR | MODE_B,

	/* 60 */
	CODE_NEG | MODE_INDADR,
	NONE,
	NONE,
	CODE_COM | MODE_INDADR,
	CODE_LSR | MODE_INDADR,
	NONE,
	CODE_ROR | MODE_INDADR,
	CODE_ASR | MODE_INDADR,

	/* 68 */
	CODE_ASL | MODE_INDADR,
	CODE_ROL | MODE_INDADR,
	CODE_DEC | MODE_INDADR,
	NONE,
	CODE_INC | MODE_INDADR,
	CODE_TST | MODE_INDADR,
	CODE_JMP | MODE_INDADR,
	CODE_CLR | MODE_INDADR,

	/* 70 */
	CODE_NEG | MODE_EXTADR,
	NONE,
	NONE,
	CODE_COM | MODE_EXTADR,
	CODE_LSR | MODE_EXTADR,
	NONE,
	CODE_ROR | MODE_EXTADR,
	CODE_ASR | MODE_EXTADR,

	/* 78 */
	CODE_ASL | MODE_EXTADR,
	CODE_ROL | MODE_EXTADR,
	CODE_DEC | MODE_EXTADR,
	NONE,
	CODE_INC | MODE_EXTADR,
	CODE_TST | MODE_EXTADR,
	CODE_JMP | MODE_EXTADR,
	CODE_CLR | MODE_EXTADR,

	/* 80 */
	CODE_SUB | MODE_A_IMM,
	CODE_CMP | MODE_A_IMM,
	CODE_SBC | MODE_A_IMM,
	NONE,
	CODE_AND | MODE_A_IMM,
	CODE_BIT | MODE_A_IMM,
	CODE_LDA | MODE_A_IMM,
	NONE,

	/* 88 */
	CODE_EOR | MODE_A_IMM,
	CODE_ADC | MODE_A_IMM,
	CODE_ORA | MODE_A_IMM,
	CODE_ADD | MODE_A_IMM,
	CODE_CPX | MODE_IMMADR,
	CODE_BSR | MODE_REL,
	CODE_LDS | MODE_IMMADR,
	NONE,

	/* 90 */
	CODE_SUB | MODE_A_DIR,
	CODE_CMP | MODE_A_DIR,
	CODE_SBC | MODE_A_DIR,
	NONE,
	CODE_AND | MODE_A_DIR,
	CODE_BIT | MODE_A_DIR,
	CODE_LDA | MODE_A_DIR,
	CODE_STA | MODE_A_DIR,

	/* 98 */
	CODE_EOR | MODE_A_DIR,
	CODE_ADC | MODE_A_DIR,
	CODE_ORA | MODE_A_DIR,
	CODE_ADD | MODE_A_DIR,
	CODE_CPX | MODE_A_DIR,
	NONE,
	CODE_LDS | MODE_DIRADR,
	CODE_STS | MODE_DIRADR,

	/* a0 */
	CODE_SUB | MODE_A_IND,
	CODE_CMP | MODE_A_IND,
	CODE_SBC | MODE_A_IND,
	NONE,
	CODE_AND | MODE_A_IND,
	CODE_BIT | MODE_A_IND,
	CODE_LDA | MODE_A_IND,
	CODE_STA | MODE_A_IND,

	/* a8 */
	CODE_EOR | MODE_A_IND,
	CODE_ADC | MODE_A_IND,
	CODE_ORA | MODE_A_IND,
	CODE_ADD | MODE_A_IND,
	CODE_CPX | MODE_A_IND,
	CODE_JSR | MODE_INDADR,
	CODE_LDS | MODE_INDADR,
	CODE_STS | MODE_INDADR,

	/* b0 */
	CODE_SUB | MODE_A_EXT,
	CODE_CMP | MODE_A_EXT,
	CODE_SBC | MODE_A_EXT,
	NONE,
	CODE_AND | MODE_A_EXT,
	CODE_BIT | MODE_A_EXT,
	CODE_LDA | MODE_A_EXT,
	CODE_STA | MODE_A_EXT,

	/* b8 */
	CODE_EOR | MODE_A_EXT,
	CODE_ADC | MODE_A_EXT,
	CODE_ORA | MODE_A_EXT,
	CODE_ADD | MODE_A_EXT,
	CODE_CPX | MODE_A_EXT,
	CODE_JSR | MODE_EXTADR,
	CODE_LDS | MODE_EXTADR,
	CODE_STS | MODE_EXTADR,

	/* c0 */
	CODE_SUB | MODE_B_IMM,
	CODE_CMP | MODE_B_IMM,
	CODE_SBC | MODE_B_IMM,
	NONE,
	CODE_AND | MODE_B_IMM,
	CODE_BIT | MODE_B_IMM,
	CODE_LDA | MODE_B_IMM,
	NONE,

	/* c8 */
	CODE_EOR | MODE_B_IMM,
	CODE_ADC | MODE_B_IMM,
	CODE_ORA | MODE_B_IMM,
	CODE_ADD | MODE_B_IMM,
	NONE,
	NONE,
	CODE_LDX | MODE_IMMADR,
	NONE,

	/* d0 */
	CODE_SUB | MODE_B_DIR,
	CODE_CMP | MODE_B_DIR,
	CODE_SBC | MODE_B_DIR,
	NONE,
	CODE_AND | MODE_B_DIR,
	CODE_BIT | MODE_B_DIR,
	CODE_LDA | MODE_B_DIR,
	CODE_STA | MODE_B_DIR,

	/* d8 */
	CODE_EOR | MODE_B_DIR,
	CODE_ADC | MODE_B_DIR,
	CODE_ORA | MODE_B_DIR,
	CODE_ADD | MODE_B_DIR,
	NONE,
	NONE,
	CODE_LDX | MODE_DIRADR,
	CODE_STX | MODE_DIRADR,

	/* e0 */
	CODE_SUB | MODE_B_IND,
	CODE_CMP | MODE_B_IND,
	CODE_SBC | MODE_B_IND,
	NONE,
	CODE_AND | MODE_B_IND,
	CODE_BIT | MODE_B_IND,
	CODE_LDA | MODE_B_IND,
	CODE_STA | MODE_B_IND,

	/* e8 */
	CODE_EOR | MODE_B_IND,
	CODE_ADC | MODE_B_IND,
	CODE_ORA | MODE_B_IND,
	CODE_ADD | MODE_B_IND,
	NONE,
	NONE,
	CODE_LDX | MODE_INDADR,
	CODE_STX | MODE_INDADR,

	/* f0 */
	CODE_SUB | MODE_B_EXT,
	CODE_CMP | MODE_B_EXT,
	CODE_SBC | MODE_B_EXT,
	NONE,
	CODE_AND | MODE_B_EXT,
	CODE_BIT | MODE_B_EXT,
	CODE_LDA | MODE_B_EXT,
	CODE_STA | MODE_B_EXT,

	/* f8 */
	CODE_EOR | MODE_B_EXT,
	CODE_ADC | MODE_B_EXT,
	CODE_ORA | MODE_B_EXT,
	CODE_ADD | MODE_B_EXT,
	NONE,
	NONE,
	CODE_LDX | MODE_EXTADR,
	CODE_STX | MODE_EXTADR
};

const static char *op_txt[] = {
	"?",
	"ABA",
	"ADC",
	"ADD",
	"AND",
	"ASL",
	"ASR",
	"BCC",
	"BCS",
	"BEQ",
	"BGE",
	"BGT",
	"BHI",
	"BLE",
	"BLS",
	"BLT",
	"BMI",
	"BNE",
	"BPL",
	"BRA",
	"BVC",
	"BVS",
	"BSR",
	"BIT",
	"CBA",
	"CLC",
	"CLI",
	"CLR",
	"CLV",
	"CMP",	
	"COM",
	"CPX",
	"DAA",
	"DEC",
	"DES",
	"DEX",
	"EOR",
	"INC",
	"INS",
	"INX",
	"JMP",
	"JSR",
	"LDA",
	"LDS",
	"LDX",
	"LSR",
	"NEG",
	"NOP",
	"ORA",
	"PSH",
	"PUL",
	"ROL",
	"ROR",
	"RTI",
	"RTS",
	"SBA",
	"SBC",
	"SEC",
	"SEI",
	"SEV",
	"STA",
	"STS",
	"STX",
	"SUB",
	"SWI",
	"TAB",
	"TAP",
	"TBA",
	"TPA",
	"TST",
	"TSX",
	"TXS",
	"WAI"
};

const static char *mode_txt[] = {
	"",
	"A #$%02x",
	"A $%02x",
	"A $%02x%02x",
	"A $%02x,X",
	"B #$%02x",
	"B $%02x",
	"B $%02x%02x",
	"B $%02x,X",
	"$%02x",
	"A",
	"B",
	"#$%02x%02x",
	"$%02x",
	"$%02x%02x",
	"$%02x,X",
	""
};

/*
	‹tƒAƒZƒ“ƒuƒ‹‚·‚é
*/
int m68disasm(char *buf, uint8 *p)
{
	char format[32];
	int code = (op_table[*p] & MASK_CODE) >> SHIFT_CODE;
	int mode = (op_table[*p] & MASK_MODE) >> SHIFT_MODE;

	sprintf(format, "%s %s", op_txt[code], mode_txt[mode]);
	sprintf(buf, format, *(p + 1), *(p + 2));
	
	switch(op_table[*p] & MASK_MODE) {
	default:
	case 0:
	case MODE_NONE:
	case MODE_A:
	case MODE_B:
		return 1;
	case MODE_A_IMM:
	case MODE_A_DIR:
	case MODE_A_IND:
	case MODE_B_IMM:
	case MODE_B_DIR:
	case MODE_B_IND:
	case MODE_REL:
	case MODE_DIRADR:
	case MODE_INDADR:
		return 2;
	case MODE_A_EXT:
	case MODE_B_EXT:
	case MODE_IMMADR:
	case MODE_EXTADR:
		return 3;
	}
}

/*
	CPU‚Ìó‘Ô‚ð•¶Žš—ñ‚Å“¾‚é
*/
char *m68regs(char *buf, const struct M68stat *m68)
{
	int len;
	uint8 mem[3];
	char dump[32], op[32];

	mem[0] = m68read8(m68, m68->pc + 0);
	mem[1] = m68read8(m68, m68->pc + 1);
	mem[2] = m68read8(m68, m68->pc + 2);
	len = m68disasm(op, mem);
	sprintf(dump, "%02x%02x%02x", mem[0], mem[1], mem[2]);
	sprintf(buf, "A=%02x B=%02x X=%04x SP=%04x PC=%04x CC=%02x(%c%c%c%c%c%c)    %.*s%*s%s", m68->a, m68->b, m68->x, m68->sp, m68->pc, m68->cc | 0xc0, (m68->cc & 0x20 ? 'H': '-'), (m68->cc & 0x10 ? 'I': '-'), (m68->cc & 0x08 ? 'N': '-'), (m68->cc & 0x04 ? 'Z': '-'), (m68->cc & 0x02 ? 'V': '-'), (m68->cc & 0x01 ? 'C': '-'), len * 2, dump, 7 - len * 2, " ", op);

	return buf;
}

/*
	Copyright 2008 maruhiro
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
