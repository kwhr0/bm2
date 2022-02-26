/*
	�����x�[�V�b�N�}�X�^�[Jr.�G�~�����[�^
	�������EI/O
*/

#include <stdio.h>
#include <string.h>
#include "bm2.h"

/*
	��������ǂݍ���(8bit)
*/
uint8 m68read8(const struct M68stat *m68, uint16 p)
{
	struct Bm2stat *bm2;

	/* ������ */
	if(p <= 0xe7ff || p >= 0xf000)
		return m68->m[p];

	/* I/O */
	bm2 = m68->tag;
	switch(p) {
	case 0xee00:	/* �e�[�v���o�͏I�� */
		stopTape(bm2);
		return 0x01;
	case 0xee20:	/* �e�[�v���o�͊J�n */
		startTape(bm2);
		return 0x01;
	case 0xee80:	/* �e�[�v���� */
		return readSound(bm2);
	case 0xeec0:	/* �L�[���� */
#if 0
		if(m68->trace)
			printf("KEY=0x%02x\n", bm2->key_mod | bm2->key_mat[bm2->key_strobe & 0x0f]);
#endif
		return bm2->key_mod | bm2->key_mat[bm2->key_strobe & 0x0f];
	case 0xef00:	/* �^�C�} */
		return 0xff; /* ??? */
	case 0xefd0:	/* RAM/ROM�̑I���E�^�C�} */
		return bm2->ram_rom;
	case 0xef80:	/* BREAK�L�[ */
		return 0x80; /* ??? */
	default:
#if 0
		printf("READ $%04X\n", p);
		fflush(stdout);
#endif
		return 0;
	}
}

/*
	��������ǂݍ���(16bit)
*/
uint16 m68read16(const struct M68stat *m68, uint16 p)
{
	return ((uint16 )m68read8(m68, p + 0) << 8) | m68read8(m68, p + 1);
}

/*
	�������ɏ�������(8bit)
*/
void m68write8(struct M68stat *m68, uint16 p, uint8 x)
{
	struct Bm2stat *bm2 = m68->tag;
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
	} else if(p < 0xe000) { /* RAM �܂��� BASIC ROM */
		if(p <= bm2->ram_end && (bm2->ram_rom & 0x01))
			m68->m[p] = x;
		return;
	} else if(p < 0xe800) { /* RAM �܂��� �v�����^ROM */
		if(p <= bm2->ram_end && (bm2->ram_rom & 0x02))
			m68->m[p] = x;
		return;
	} else if(p < 0xf000) { /* I/O */
		
	} else { /* RAM �܂��� ���j�^ROM */
		if(p <= bm2->ram_end && (bm2->ram_rom & 0x04))
			m68->m[p] = x;
		return;
	}

	/* I/O */
	switch(p) {
	case 0xe890:	/* MP-1710 �����F�E�����w�i */
		if(!bm2->mp1710)
			return;
		bm2->colreg = x;
		break;
	case 0xe891:	/* MP-1710 �w�i�F */
		if(!bm2->mp1710)
			return;
		bm2->bckreg = x;
		break;
	case 0xe892:	/* MP-1710 �F��񏑂����� */
		if(!bm2->mp1710)
			return;
		bm2->mp1710on = TRUE;
		bm2->wtenbl = x & 1;
		break;
	case 0xee40:	/* ��ʔ��] */
		bm2->reverse = x & 0x80;
		break;
	case 0xee80:	/* �e�[�v�o�� */
		writeSound(bm2, x);
		break;
	case 0xeec0:	/* �L�[�X�g���[�u�ݒ� */
#if 0
		if(m68->trace)
			printf("STROBE=0x%02x\n", x);
#endif
		bm2->key_strobe = x;

		if(bm2->key_break && (bm2->key_strobe & 0x80)) {
			bm2->key_break = FALSE;
			bm2->fast = FALSE;
			bm2->sound_tape = FALSE;
			m68nmi(m68);
		}
		break;
	case 0xefd0:	/* RAM/ROM�̑I���E�^�C�} */
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
		else { /* �v�����^ROM */
			memcpy(&bm2->ram_b000_e7ff[0x3000], &bm2->cpu.m[0xe000], 0x800);
			memcpy(&bm2->cpu.m[0xe000], &bm2->rom_b000_e7ff[0x3000], 0x800);
		}

		/* $F000~$FFFF */
		if(bm2->ram_rom & 0x04) /* RAM */
			memcpy(&bm2->cpu.m[0xf000], bm2->ram_f000_ffff, 0x1000);
		else { /* ���j�^ROM */
			memcpy(bm2->ram_f000_ffff, &bm2->cpu.m[0xf000], 0x1000);
			memcpy(&bm2->cpu.m[0xf000], bm2->rom_f000_ffff, 0x1000);
		}
		break;
	case 0xefe0:
		/* �O���t�B�b�N���[�h�̕ύX */
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
	�������ɏ�������(16bit)
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
