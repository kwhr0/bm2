/*
	�����x�[�V�b�N�}�X�^�[Jr.�G�~�����[�^
	���C��
*/

#include <stdio.h>
#include "bm2.h"

int main(int argc, char *argv[])
{
	struct Bm2stat *bm2 = malloc(sizeof(struct Bm2stat));
	int timer = 0;

	/* ���������� */
	if(!init(bm2, argc, argv))
		return 1;

	/* CPU�����Z�b�g���� */
	m68reset(&bm2->cpu);

	for(;;) {
		/* �R�[�h�����s���� */
		m68exec(&bm2->cpu);

		/* ��ʂ��X�V���� */
		updateScreen(bm2);

		/* �L�[���X�V���� */
		updateKey(bm2);

		/* BREAK�L�[�ɂ��NMI */
		if(bm2->key_break && (bm2->key_strobe & 0x80)) {
			bm2->key_break = FALSE;
			bm2->fast = FALSE;
			bm2->sound_tape = FALSE;
			m68nmi(&bm2->cpu);
		}

		/* IRQ(�^�C�}) */
		if(timer >= 1000 / 60) {
			if(!(bm2->ram_rom & 0x10))
				m68irq(&bm2->cpu);
			timer -= 1000 / 60;
		}

		/* ���Z�b�g */
		if(bm2->key_break && !(bm2->key_mod & 0x40))
			m68reset(&bm2->cpu);

		do {
			timer += 1000 / bm2->io_freq;
			bm2->cpu.states += bm2->cpu_freq / bm2->io_freq;

			/* �҂� */
			delay(bm2->fast ? 0: 1000 / bm2->io_freq);

			/* �T�E���h�o�b�t�@�����ւ��� */
			if(bm2->use_sound)
				flipSoundBuffer(bm2);
		} while(bm2->cpu.states < 0);
	}

	return 0;
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
