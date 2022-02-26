/*
	Motorola M6800 Emulator Header (m6800.c)
*/

#if !defined(M6800_H)
#define M6800_H

/* CPU��� */
#define M68_RUN	0	/* ���s�� */
#define M68_WAI	1	/* ���荞�ݑ҂� */

/* �^ */
typedef unsigned char	uint8;
typedef char	int8;
typedef unsigned short	uint16;
typedef short	int16;
typedef unsigned long	uint32;
typedef long	int32;

/* CPU��� */
struct M68stat {
	uint8 *m;		/* ������ */
	uint16 pc;		/* �v���O�����J�E���^ */
	uint16 sp;		/* �X�^�b�N�|�C���^ */
	uint16 x;		/* �C���f�b�N�X���W�X�^ */
	uint8 a;		/* �A�L�������[�^A */
	uint8 b;		/* �A�L�������[�^B */
	uint8 cc;		/* �R���f�B�V�����R�[�h���W�X�^ */
	uint8 wai;		/* ���荞�ݑ҂���Ԃ�? */
	int states;		/* �c��X�e�[�g�� */
	int total_states;	/* �ݐσX�e�[�g�� */
	int emulate_subroutine;	/* �T�u���[�`�����G�~�����[�g���邩? */
	int trace;		/* �g���[�X���[�h��? */
	void *tag;		/* ���̑��̏�� */
};

struct M68stat m68init(void *, void *);
int m68states(const struct M68stat *);
int m68diff(int, int);
int m68reset(struct M68stat *);
int m68nmi(struct M68stat *);
int m68irq(struct M68stat *);
int m68exec(struct M68stat *);

int m68disasm(char *, uint8 *);
char *m68regs(char *, const struct M68stat *);

uint8 m68read8(const struct M68stat *, uint16);
uint16 m68read16(const struct M68stat *, uint16);
void m68write8(struct M68stat *, uint16, uint8);
void m68write16(struct M68stat *, uint16, uint16);

#if defined(M68_SUB)
int m68subroutine(struct M68stat *, uint16);
#endif
#if defined(M68_TRACE)
void m68log(const struct M68stat *);
#endif

#endif

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
