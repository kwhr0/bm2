/*
	日立ベーシックマスターJr.エミュレータ
	テープ, 音声
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

static uint8 tapeReadBuffer[0x10000];	/* テープ読み込みバッファ */
static int tapeReadPos = 0;	/* テープの読み込み位置 */
static int tapeReadBufferSize = -1;	/* テープ読込バッファサイズ */
static int tapeReadStartStates;	/* テープの読み込みを開始した時点の累積ステート数 */

static uint8 tapeWriteBuffer[0x10000];	/* テープ書込バッファ */
static int tapeWritePos;	/* テープ書き込み位置 */
static int tapeWriteLastStates;	/* 最後に書き込んだ時点の累積ステート数 */
static int tapeWriteTotalStates;	/* 1ビットの書き込みを開始した時点からの累積ステート数 */
static int tapeWriteChangeCount;	/* 書き込みのビットが変化した回数 */

static uint8 lastWriteVol = 0;	/* 最後に出力した値 */

/*
	ディスクから読み込む (startTapeの下請け)
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
	ディスクに書き込む(新規) (stopTapeの下請け)
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
	ディスクに書き込む(追加) (stopTapeの下請け)
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
	テープの読み取り位置を設定する (下請け)
*/
static int setReadPos(struct Bm2stat *bm2, int pos)
{
	tapeReadStartStates = m68states(&bm2->cpu) - pos * 2433;
	return pos;
}

/*
	テープの読み取り位置を初期化する (下請け)
*/
static int resetReadPos(struct Bm2stat *bm2)
{
	return setReadPos(bm2, 0);
}

/*
	テープの読み取り位置を得る (readSoundの下請け)
*/
static int getReadPos(const struct Bm2stat *bm2)
{
	return m68diff(m68states(&bm2->cpu), tapeReadStartStates) / 2433;
}

/*
	テープから読み込む
*/
uint8 readSound(struct Bm2stat *bm2)
{
	uint8 vol;

	/* テープの読み取り位置を得る */
	tapeReadPos = getReadPos(bm2);

	/* テープの終端まで達したら巻き戻す */
	if(tapeReadPos / 11 >= tapeReadBufferSize)
		tapeReadPos = resetReadPos(bm2);

	switch(tapeReadPos % 11) {
	case 0: /* スタートビット */
		vol = 0;
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8: /* データビット */
		vol = (tapeReadBuffer[tapeReadPos / 11] & (1 << (tapeReadPos % 11 - 1)) ? 0x80: 0);
		break;
	case 9:
	case 10: /* ストップビット */
	default:
		vol = 0x80;
		break;
	}
	return vol;
}

/*
	テープの書き込み位置を初期化する
*/
static void resetWritePos(struct Bm2stat *bm2)
{
	tapeWriteLastStates = m68states(&bm2->cpu);
	tapeWriteTotalStates = tapeWriteChangeCount = 0;
}

/*
	テープに出力する (writeSoundの下請け)
*/
static void writeSoundTape(struct Bm2stat *bm2, uint8 vol)
{
	int interval, bit;

	/* 最後に出力してからのステート数を求める */
	interval = m68diff(m68states(&bm2->cpu), tapeWriteLastStates);
	tapeWriteLastStates = m68states(&bm2->cpu);

	/* 変化回数と1ビット書き込み開始からのステート数を加算する */
	if(interval > 470) { /* 無音か? */
		tapeWriteTotalStates = tapeWriteChangeCount = 0;
		return;
	}
	tapeWriteChangeCount++;
	tapeWriteTotalStates += interval;

	/* 1bitに満たないなら戻る */
	if(tapeWriteTotalStates < 2433)
		return;

	/* ビットを得る */
	if(tapeWritePos / 11 >= sizeof(tapeWriteBuffer)) /* バッファを超えたか? */
		return;
	bit = (tapeWriteChangeCount > 12);
	tapeWriteTotalStates = tapeWriteChangeCount = 0;

	/* データを書き込む */
	if(
	(tapeWritePos % 11 == 0 && bit) ||	/* スタートビットが1 (ずれている) */
	(tapeWritePos % 11 == 10 && !bit)	/* ストップビットが0 (ずれている) */
	)
		return;
	if(
	tapeWritePos % 11 == 0 ||	/* スタートビット */
	tapeWritePos % 11 == 9 ||	/* ストップビット */
	tapeWritePos % 11 == 10	/* ストップビット */
	)
		;
	else if(bit)	/* データビット 1 */
		tapeWriteBuffer[tapeWritePos / 11] |= (1 << (tapeWritePos % 11 - 1));
	else	/* データビット 0 */
		tapeWriteBuffer[tapeWritePos / 11] &= ~(1 << (tapeWritePos % 11 - 1));
	tapeWritePos++;
}

/*
	スピーカに出力する (writeSoundの下請け)
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
	テープまたはスピーカに出力する
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
	テープの読み取り/書き込みを開始する
*/
void startTape(struct Bm2stat *bm2)
{
	bm2->sound_tape = TRUE;
	bm2->fast = TRUE;

	/* 最後に読み込んだ位置に戻す */
	setReadPos(bm2, tapeReadPos);

	/* テープを読んでいない, または読み終えた場合はテープを読み込む */
	if(tapeReadBufferSize < 0 || tapeReadBufferSize <= getReadPos(bm2) / 11) {
		tapeReadBufferSize = readBin(bm2->tape_path, tapeReadBuffer, sizeof(tapeReadBuffer));
		resetReadPos(bm2);
		resetWritePos(bm2);
	}
}

/*
	テープの読み取り/書き込みを終了する
*/
void stopTape(struct Bm2stat *bm2)
{
	/* テープの内容をディスクに書き込む */
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
	テープを設定する
*/
void setTape(struct Bm2stat *bm2, const char *path)
{
	strcpy(bm2->tape_path, path);
	tapeReadBufferSize = -1;
	rewindTape(bm2);
}

/*
	テープを巻き戻す
*/
void rewindTape(struct Bm2stat *bm2)
{
	bm2->tape_mode = TAPE_MODE_OVERWRITE;
	resetReadPos(bm2);
	resetWritePos(bm2);

	updateCaption(bm2);
}

/*
	テープを終端まで早送りする
*/
void foreTape(struct Bm2stat *bm2)
{
	bm2->tape_mode = TAPE_MODE_APPEND;

	updateCaption(bm2);
}

/*
	サウンドバッファの大きさを求める
*/
int getSoundSampleSize(int io_freq)
{
	int sample_size;

	for(sample_size = 0x40000000; sample_size != 1 && 44100 / sample_size <= io_freq; sample_size >>= 1)
		;

	return sample_size;
}

/*
	読み込みバッファと書き込みバッファを入れ替える
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
