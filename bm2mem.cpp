/*
	日立ベーシックマスターJr.エミュレータ
	メモリ・I/O
*/

#include <stdio.h>
#include <string.h>
#include "bm2.h"

#include "CompareProcess.h"
#include "HD6303.h"
#include "memory.h"

extern HD6303 hd6303;

struct CP : CompareProcess {
#if HD6303_TRACE
	void Stop() { hd6303.StopTrace(); }
#endif
} cp;

void setCompare(int f) {
	cp.setCompare(f);
}

/*
	メモリを読み込む(8bit)
*/
uint8 m68read8(const struct M68stat *m68, uint16 p)
{
	uint32_t d = 0;
#if defined(CURMPU) && defined(NEWMPU)
		if (cp.ReadStart(p, d)) return d;
#endif
	struct Bm2stat *bm2;

	/* メモリ */
	if(p <= 0xe7ff || p >= 0xf000)
	{ d = m68->m[p]; goto RET; }

	/* I/O */
	bm2 = (Bm2stat *)m68->tag;
	switch(p) {
	case 0xee00:	/* テープ入出力終了 */
		stopTape(bm2);
		d = 0x01;
			break;
	case 0xee20:	/* テープ入出力開始 */
		startTape(bm2);
		d = 0x01;
			break;
	case 0xee80:	/* テープ入力 */
		d = readSound(bm2);
			break;
	case 0xeec0:	/* キー入力 */
#if 0
		if(m68->trace)
			printf("KEY=0x%02x\n", bm2->key_mod | bm2->key_mat[bm2->key_strobe & 0x0f]);
#endif
		d = bm2->key_mod | bm2->key_mat[bm2->key_strobe & 0x0f];
			break;
	case 0xef00:	/* タイマ */
		d = 0xff; /* ??? */
			break;
	case 0xefd0:	/* RAM/ROMの選択・タイマ */
		d = bm2->ram_rom;
			break;
	case 0xef80:	/* BREAKキー */
		d = 0x80; /* ??? */
			break;
	default:
#if 0
		printf("READ $%04X\n", p);
		fflush(stdout);
#endif
			break;
	}
RET:
#if defined(CURMPU) && defined(NEWMPU)
cp.ReadEnd(d);
#endif
	return d;
}

/*
	メモリを読み込む(16bit)
*/
uint16 m68read16(const struct M68stat *m68, uint16 p)
{
	return ((uint16 )m68read8(m68, p + 0) << 8) | m68read8(m68, p + 1);
}

/*
	メモリに書き込む(8bit)
*/
void m68write8(struct M68stat *m68, uint16 p, uint8 x)
{
#if defined(CURMPU) && defined(NEWMPU)
		if (cp.WriteStart(p, x)) return;
#endif
	struct Bm2stat *bm2 = (Bm2stat *)m68->tag;
	int off;

	if(p < 0xb000) { /* RAM */
		if(p <= bm2->ram_end)
			m68->m[p] = x;

		if(!bm2->mp1710 || !bm2->wtenbl)
			return;
		if(0x100 <= p && p < 0x400) {
			off = p - 0x100;
			bm2->color_map[off] = bm2->colreg;
		}
		return;
	} else if(p < 0xe000) { /* RAM または BASIC ROM */
		if(p <= bm2->ram_end && (bm2->ram_rom & 0x01))
			m68->m[p] = x;
		return;
	} else if(p < 0xe800) { /* RAM または プリンタROM */
		if(p <= bm2->ram_end && (bm2->ram_rom & 0x02))
			m68->m[p] = x;
		return;
	} else if(p < 0xf000) { /* I/O */
		if (p == 0xe800) putc(x, stderr); // custom output: print to the host
	} else { /* RAM または モニタROM */
		if(p <= bm2->ram_end && (bm2->ram_rom & 0x04))
			m68->m[p] = x;
		return;
	}

	/* I/O */
	switch(p) {
	case 0xe890:	/* MP-1710 文字色・文字背景 */
		if(!bm2->mp1710)
			return;
		bm2->colreg = x;
		break;
	case 0xe891:	/* MP-1710 背景色 */
		if(!bm2->mp1710)
			return;
		bm2->bckreg = x;
		break;
	case 0xe892:	/* MP-1710 色情報書き込み */
		if(!bm2->mp1710)
			return;
		bm2->mp1710on = TRUE;
		bm2->wtenbl = x & 1;
		break;
	case 0xee40:	/* 画面反転 */
		bm2->reverse = x & 0x80;
		break;
	case 0xee80:	/* テープ出力 */
		writeSound(bm2, x);
		break;
	case 0xeec0:	/* キーストローブ設定 */
#if 0
		if(m68->trace)
			printf("STROBE=0x%02x\n", x);
#endif
		bm2->key_strobe = x;

		if(bm2->key_break && (bm2->key_strobe & 0x80)) {
			bm2->key_break = FALSE;
			bm2->fast = FALSE;
			bm2->sound_tape = FALSE;
			extern bool gNMI;
			gNMI = true;//m68nmi(m68);
		}
			break;
	case 0xefd0:	/* RAM/ROMの選択・タイマ */
		bm2->ram_rom = x;

		/* $B000~$DFFF */
		if(bm2->ram_rom & 0x01) /* RAM */
			memcpy(&bm2->cpu.m[0xb000], &bm2->ram_b000_e7ff[0x0000], 0x3000);
		else { /* BASIC ROM */
			memcpy(&bm2->ram_b000_e7ff[0x0000], &bm2->cpu.m[0xb000], 0x3000);
			memcpy(&bm2->cpu.m[0xb000], &bm2->rom_b000_e7ff[0x0000], 0x3000);
		}

		/* $E000~$E7FF */
		if(bm2->ram_rom & 0x02) /* RAM */
			memcpy(&bm2->cpu.m[0xe000], &bm2->ram_b000_e7ff[0x3000], 0x800);
		else { /* プリンタROM */
			memcpy(&bm2->ram_b000_e7ff[0x3000], &bm2->cpu.m[0xe000], 0x800);
			memcpy(&bm2->cpu.m[0xe000], &bm2->rom_b000_e7ff[0x3000], 0x800);
		}

		/* $F000~$FFFF */
		if(bm2->ram_rom & 0x04) /* RAM */
			memcpy(&bm2->cpu.m[0xf000], bm2->ram_f000_ffff, 0x1000);
		else { /* モニタROM */
			memcpy(bm2->ram_f000_ffff, &bm2->cpu.m[0xf000], 0x1000);
			memcpy(&bm2->cpu.m[0xf000], bm2->rom_f000_ffff, 0x1000);
		}
		break;
	case 0xefe0:
		/* グラフィックモードの変更 */
		bm2->screen_mode = x & 0xcc;
		break;
#if 0
	default:
		printf("WRITE $%04X $%02X\n", p, x);
		fflush(stdout);
		break;
#endif
	}
}

/*
	メモリに書き込む(16bit)
*/
void m68write16(struct M68stat *m68, uint16 p, uint16 x)
{
	m68write8(m68, p + 0, x >> 8);
	m68write8(m68, p + 1, x & 0xff);
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
