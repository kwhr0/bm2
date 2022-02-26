/*
	�����x�[�V�b�N�}�X�^�[Jr.�G�~�����[�^
	�e�[�v, ����
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bm2.h"

#ifndef O_BINARY
#	define O_BINARY	0
#endif

static uint8 tapeReadBuffer[0x10000];	/* �e�[�v�ǂݍ��݃o�b�t�@ */
static int tapeReadPos = 0;	/* �e�[�v�̓ǂݍ��݈ʒu */
static int tapeReadBufferSize = -1;	/* �e�[�v�Ǎ��o�b�t�@�T�C�Y */
static int tapeReadStartStates;	/* �e�[�v�̓ǂݍ��݂��J�n�������_�̗ݐσX�e�[�g�� */

static uint8 tapeWriteBuffer[0x10000];	/* �e�[�v�����o�b�t�@ */
static int tapeWritePos;	/* �e�[�v�������݈ʒu */
static int tapeWriteLastStates;	/* �Ō�ɏ������񂾎��_�̗ݐσX�e�[�g�� */
static int tapeWriteTotalStates;	/* 1�r�b�g�̏������݂��J�n�������_����̗ݐσX�e�[�g�� */
static int tapeWriteChangeCount;	/* �������݂̃r�b�g���ω������� */

static uint8 lastWriteVol = 0;	/* �Ō�ɏo�͂����l */

/*
	�f�B�X�N����ǂݍ��� (startTape�̉�����)
*/
int readBin(const char *path, void *buf, int max_size)
{
	int fd, size;

	if(path == NULL)
		return 0;

	if((fd = open(path, O_RDONLY | O_BINARY)) < 0)
		return -1;
	size = read(fd, buf, max_size);
	close(fd);

	return size;
}

/*
	�f�B�X�N�ɏ�������(�V�K) (stopTape�̉�����)
*/
int writeBin(const char *path, const void *buf, int size)
{
	int fd, written_size;

	if(path == NULL)
		return 0;

	if((fd = open(path, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0664)) < 0)
		return -1;
	written_size = write(fd, buf, size);
	close(fd);

	return written_size;
}

/*
	�f�B�X�N�ɏ�������(�ǉ�) (stopTape�̉�����)
*/
int appendBin(const char *path, const void *buf, int size)
{
	int fd, written_size;

	if(path == NULL)
		return 0;

	if((fd = open(path, O_CREAT | O_APPEND | O_WRONLY | O_BINARY, 0664)) < 0)
		return -1;
	written_size = write(fd, buf, size);
	close(fd);

	return written_size;
}

/*
	�e�[�v�̓ǂݎ��ʒu��ݒ肷�� (������)
*/
static int setReadPos(struct Bm2stat *bm2, int pos)
{
	tapeReadStartStates = m68states(&bm2->cpu) - pos * 2433;
	return pos;
}

/*
	�e�[�v�̓ǂݎ��ʒu������������ (������)
*/
static int resetReadPos(struct Bm2stat *bm2)
{
	return setReadPos(bm2, 0);
}

/*
	�e�[�v�̓ǂݎ��ʒu�𓾂� (readSound�̉�����)
*/
static int getReadPos(const struct Bm2stat *bm2)
{
	return m68diff(m68states(&bm2->cpu), tapeReadStartStates) / 2433;
}

/*
	�e�[�v����ǂݍ���
*/
uint8 readSound(struct Bm2stat *bm2)
{
	uint8 vol;

	/* �e�[�v�̓ǂݎ��ʒu�𓾂� */
	tapeReadPos = getReadPos(bm2);

	/* �e�[�v�̏I�[�܂ŒB�����犪���߂� */
	if(tapeReadPos / 11 >= tapeReadBufferSize)
		tapeReadPos = resetReadPos(bm2);

	switch(tapeReadPos % 11) {
	case 0: /* �X�^�[�g�r�b�g */
		vol = 0;
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8: /* �f�[�^�r�b�g */
		vol = (tapeReadBuffer[tapeReadPos / 11] & (1 << (tapeReadPos % 11 - 1)) ? 0x80: 0);
		break;
	case 9:
	case 10: /* �X�g�b�v�r�b�g */
	default:
		vol = 0x80;
		break;
	}
	return vol;
}

/*
	�e�[�v�̏������݈ʒu������������
*/
static void resetWritePos(struct Bm2stat *bm2)
{
	tapeWriteLastStates = m68states(&bm2->cpu);
	tapeWriteTotalStates = tapeWriteChangeCount = 0;
}

/*
	�e�[�v�ɏo�͂��� (writeSound�̉�����)
*/
static void writeSoundTape(struct Bm2stat *bm2, uint8 vol)
{
	int interval, bit;

	/* �Ō�ɏo�͂��Ă���̃X�e�[�g�������߂� */
	interval = m68diff(m68states(&bm2->cpu), tapeWriteLastStates);
	tapeWriteLastStates = m68states(&bm2->cpu);

	/* �ω��񐔂�1�r�b�g�������݊J�n����̃X�e�[�g�������Z���� */
	if(interval > 470) { /* ������? */
		tapeWriteTotalStates = tapeWriteChangeCount = 0;
		return;
	}
	tapeWriteChangeCount++;
	tapeWriteTotalStates += interval;

	/* 1bit�ɖ����Ȃ��Ȃ�߂� */
	if(tapeWriteTotalStates < 2433)
		return;

	/* �r�b�g�𓾂� */
	if(tapeWritePos / 11 >= sizeof(tapeWriteBuffer)) /* �o�b�t�@�𒴂�����? */
		return;
	bit = (tapeWriteChangeCount > 12);
	tapeWriteTotalStates = tapeWriteChangeCount = 0;

	/* �f�[�^���������� */
	if(
	(tapeWritePos % 11 == 0 && bit) ||	/* �X�^�[�g�r�b�g��1 (����Ă���) */
	(tapeWritePos % 11 == 10 && !bit)	/* �X�g�b�v�r�b�g��0 (����Ă���) */
	)
		return;
	if(
	tapeWritePos % 11 == 0 ||	/* �X�^�[�g�r�b�g */
	tapeWritePos % 11 == 9 ||	/* �X�g�b�v�r�b�g */
	tapeWritePos % 11 == 10	/* �X�g�b�v�r�b�g */
	)
		;
	else if(bit)	/* �f�[�^�r�b�g 1 */
		tapeWriteBuffer[tapeWritePos / 11] |= (1 << (tapeWritePos % 11 - 1));
	else	/* �f�[�^�r�b�g 0 */
		tapeWriteBuffer[tapeWritePos / 11] &= ~(1 << (tapeWritePos % 11 - 1));
	tapeWritePos++;
}

/*
	�X�s�[�J�ɏo�͂��� (writeSound�̉�����)
*/
static void writeSoundSpeaker(struct Bm2stat *bm2, uint8 vol)
{
	unsigned int pos, off;

	pos = bm2->cpu_freq / bm2->io_freq - bm2->cpu.states;
	if(pos & 0x80000000)
		return;
	else if(pos & 0xf0000000)
		off = 44100 * (pos >> 16) / (bm2->cpu_freq >> 16);
	else if(pos & 0xff000000)
		off = 44100 * (pos >> 12) / (bm2->cpu_freq >> 12);
	else if(pos & 0xfff00000)
		off = 44100 * (pos >> 8) / (bm2->cpu_freq >> 8);
	else if(pos & 0xffff0000)
		off = 44100 * (pos >> 4) / (bm2->cpu_freq >> 4);
	else
		off = 44100 * pos / bm2->cpu_freq;
	if(off < bm2->sound_sample_size)
		memset(bm2->sound_write_pointer + off, (int )vol / 2, bm2->sound_sample_size - off);
}

/*
	�e�[�v�܂��̓X�s�[�J�ɏo�͂���
*/
void writeSound(struct Bm2stat *bm2, uint8 vol)
{
	lastWriteVol = vol;

	if(bm2->sound_tape)
		writeSoundTape(bm2, vol);
	else
		writeSoundSpeaker(bm2, vol);
}

/*
	�e�[�v�̓ǂݎ��/�������݂��J�n����
*/
void startTape(struct Bm2stat *bm2)
{
	bm2->sound_tape = TRUE;
	bm2->fast = TRUE;

	/* �Ō�ɓǂݍ��񂾈ʒu�ɖ߂� */
	setReadPos(bm2, tapeReadPos);

	/* �e�[�v��ǂ�ł��Ȃ�, �܂��͓ǂݏI�����ꍇ�̓e�[�v��ǂݍ��� */
	if(tapeReadBufferSize < 0 || tapeReadBufferSize <= getReadPos(bm2) / 11) {
		tapeReadBufferSize = readBin(bm2->tape_path, tapeReadBuffer, sizeof(tapeReadBuffer));
		resetReadPos(bm2);
		resetWritePos(bm2);
	}
}

/*
	�e�[�v�̓ǂݎ��/�������݂��I������
*/
void stopTape(struct Bm2stat *bm2)
{
	/* �e�[�v�̓��e���f�B�X�N�ɏ������� */
	if(tapeWritePos / 11 > 0) {
		writeSoundTape(bm2, 0);
		if(bm2->tape_mode == TAPE_MODE_APPEND)
			appendBin(bm2->tape_path, tapeWriteBuffer, tapeWritePos / 11);
		else if(bm2->tape_mode == TAPE_MODE_OVERWRITE)
			writeBin(bm2->tape_path, tapeWriteBuffer, tapeWritePos / 11);
		tapeWritePos = 0;
	}

	bm2->tape_mode = TAPE_MODE_APPEND;
	bm2->sound_tape = FALSE;
	bm2->fast = FALSE;

	updateCaption(bm2);
}

/*
	�e�[�v��ݒ肷��
*/
void setTape(struct Bm2stat *bm2, const char *path)
{
	strcpy(bm2->tape_path, path);
	tapeReadBufferSize = -1;
	rewindTape(bm2);
}

/*
	�e�[�v�������߂�
*/
void rewindTape(struct Bm2stat *bm2)
{
	bm2->tape_mode = TAPE_MODE_OVERWRITE;
	resetReadPos(bm2);
	resetWritePos(bm2);

	updateCaption(bm2);
}

/*
	�e�[�v���I�[�܂ő����肷��
*/
void foreTape(struct Bm2stat *bm2)
{
	bm2->tape_mode = TAPE_MODE_APPEND;

	updateCaption(bm2);
}

/*
	�T�E���h�o�b�t�@�̑傫�������߂�
*/
int getSoundSampleSize(int io_freq)
{
	int sample_size;

	for(sample_size = 0x40000000; sample_size != 1 && 44100 / sample_size <= io_freq; sample_size >>= 1)
		;

	return sample_size;
}

/*
	�ǂݍ��݃o�b�t�@�Ə������݃o�b�t�@�����ւ���
*/
void flipSoundBuffer(struct Bm2stat *bm2)
{
	uint8 *next_write_pointer;

	next_write_pointer = bm2->sound_write_pointer + bm2->sound_sample_size;
	if(next_write_pointer >= bm2->sound_buffer + bm2->sound_buffer_size)
		next_write_pointer = bm2->sound_buffer;
	if(bm2->sound_read_pointer <= next_write_pointer && next_write_pointer < bm2->sound_read_pointer + bm2->sound_sample_size)
		return;
	bm2->sound_write_pointer = next_write_pointer;

	lastWriteVol = lastWriteVol / 2;
	memset(bm2->sound_write_pointer, lastWriteVol, bm2->sound_sample_size);
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
