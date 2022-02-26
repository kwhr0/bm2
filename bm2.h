/*
	日立ベーシックマスターJr.(MB-6885)エミュレータ ヘッダ
*/

#ifndef BM2_H

#include <stdio.h>
#include <limits.h>
#include "test.h"
#ifdef CURMPU
#include "m6800.h"
#else
#include "m68alt.h"
#endif
#include "srecord.h"
#include "conf.h"
#include "depend.h"

/* 真偽 */
#define FALSE	0	/* 偽 */
#define TRUE	1	/* 真 */

/* ファイル選択のフィルタ */
#define FILTER_TAPE	0x0001
#define FILTER_SOUND	0x0002
#define FILTER_BINARY	0x0004
#define FILTER_TEXT	0x0008

/* MB-6885の仮想キーコード */
#define BMKEY_NONE	0x00
#define BMKEY_Z	0x10
#define BMKEY_X	0x11
#define BMKEY_C	0x12
#define BMKEY_V	0x13
#define BMKEY_B	0x14
#define BMKEY_N	0x15
#define BMKEY_M	0x16
#define BMKEY_COMMA	0x17
#define BMKEY_PERIOD	0x18
#define BMKEY_SLASH	0x19
#define BMKEY_UNDERBAR	0x1a
#define BMKEY_SPACE	0x1b
#define BMKEY_A	0x20
#define BMKEY_S	0x21
#define BMKEY_D	0x22
#define BMKEY_F	0x23
#define BMKEY_G	0x24
#define BMKEY_H	0x25
#define BMKEY_J	0x26
#define BMKEY_K	0x27
#define BMKEY_L	0x28
#define BMKEY_SEMICOLON	0x29
#define BMKEY_COLON	0x2a
#define BMKEY_RIGHTBRACKET	0x2b
#define BMKEY_RETURN	0x2c
#define BMKEY_Q	0x40
#define BMKEY_W	0x41
#define BMKEY_E	0x42
#define BMKEY_R	0x43
#define BMKEY_T	0x44
#define BMKEY_Y	0x45
#define BMKEY_U	0x46
#define BMKEY_I	0x47
#define BMKEY_O	0x48
#define BMKEY_P	0x49
#define BMKEY_AT	0x4a
#define BMKEY_LEFTBRACKET	0x4b
#define BMKEY_DELETE	0x4c
#define BMKEY_1	0x80
#define BMKEY_2	0x81
#define BMKEY_3	0x82
#define BMKEY_4	0x83
#define BMKEY_5	0x84
#define BMKEY_6	0x85
#define BMKEY_7	0x86
#define BMKEY_8	0x87
#define BMKEY_9	0x88
#define BMKEY_0	0x89
#define BMKEY_MINUS	0x8a
#define BMKEY_CARET	0x8b
#define BMKEY_YEN	0x8c
#define BMKEY_ALPHA	0x0110	/* 英数 */
#define BMKEY_ASHIFT	0x0120	/* 英記号 */
#define BMKEY_KSHIFT	0x0140	/* カナ記号 */
#define BMKEY_KANA	0x0180	/* カナ */
#define BMKEY_BREAK	0x0200	/* BREAK RESET */
#define BMKEY_OVERWRITE	0x0201	/* 巻き戻し */
#define BMKEY_APPEND	0x0202	/* 早送り */

/* エミュレータの状態 */
struct Bm2stat {
	/* CPU */
	struct M68stat cpu;		/* CPUの状態 */
	int cpu_freq;			/* CPUのクロック数(Hz) */

	/* メモリ */
	uint8 memory[0x10000];		/* メモリ */
	uint8 ram_rom;			/* RAM/ROMの選択 */
	uint16 ram_end;			/* RAMエリアの最終アドレス */

	/* RAM */
	uint8 ram_b000_e7ff[0x3800];	/* RAM $B000〜$E7FF */
	uint8 ram_f000_ffff[0x1000];	/* RAM $F000〜$FFFF */

	/* ROM */
	uint8 rom_b000_e7ff[0x3800];	/* BASIC・プリンタROM */
	uint8 rom_f000_ffff[0x1000];	/* モニタROM */
	uint8 rom_font[0x100][8];	/* フォント */

	/* スクリーン */
	uint8 screen_mode;		/* スクリーンモード */
	uint8 reverse;			/* 反転状態 */
	int zoom;			/* 画面の倍率 */

	/* キー */
	uint8 key_strobe;		/* キーストローブ */
	uint8 key_mod;			/* キーモデファイア */
	uint8 key_mat[256];		/* キー状態 */
	uint8 key_break;		/* BREAKキー状態 */
	int keyconv[KEY_LAST + 1];	/* PC→MB-6885キー変換表 */

	/* テープ */
	char tape_path[PATH_MAX];	/* テープファイルパス名 */
	int tape_mode;			/* テープモード */
#define TAPE_MODE_APPEND	0	/* 追加 */
#define TAPE_MODE_OVERWRITE	1	/* 上書き */
#define TAPE_MODE_READONLY	2	/* 書込み禁止 */
	
	/* 音声 */
	int sound_sample_size;		/* 音声1/60秒あたりのbyte数 */
	int sound_buffer_size;		/* 音声バッファのサイズ */
	uint8 *sound_buffer;		/* 音声バッファ */
	uint8 *sound_read_pointer;	/* 音声読み込みポインタ */
	uint8 *sound_write_pointer;	/* 音声書き込みポインタ */
	int sound_tape;			/* テープ出力か? */
	int use_sound;			/* 音声を出力するか? */

	/* ディスプレイ */
	uint32 display;			/* ディスプレイの種類 */

	/* カラーアダプタ */
	int mp1710;			/* カラーアダプタがあるか? */
	int mp1710on;			/* カラーアダプタが有効か? */
	uint8 colreg;			/* 文字色 */
	uint8 bckreg;			/* 背景色 */
	uint8 wtenbl;			/* カラーか? */
	uint8 color_map[32 * 24];	/* カラーマップ */

	/* その他 */
	int fast;			/* 高速か? */
	int io_freq;			/* I/O更新周期 */
	int menu;			/* メニューか? */
	int full_line;			/* 全ライン表示か? */
};

#ifdef __cplusplus
extern "C" {
#endif

/* menu.c */
int inputFileName(struct Bm2stat *, char *, unsigned int);
void menu(struct Bm2stat *);

/* init.c */
int init(struct Bm2stat *, int, char *[]);
void loadBinary(struct Bm2stat *bm2);

/* bm2sub.c */
void locate(struct Bm2stat *, int, int);
void print(struct Bm2stat *, const char *, ...);
char getkey(struct Bm2stat *);
char getstr(struct Bm2stat *, char *);
char getkeystr(struct Bm2stat *, char *);

/* sound.c */
int readBin(const char *, void *, int);
int writeBin(const char *, const void *, int);
int getSoundSampleSize(int);
uint8 readSound(struct Bm2stat *);
void writeSound(struct Bm2stat *, uint8);
void startTape(struct Bm2stat *);
void stopTape(struct Bm2stat *);
void setTape(struct Bm2stat *, const char *);
void rewindTape(struct Bm2stat *);
void foreTape(struct Bm2stat *);
void flipSoundBuffer(struct Bm2stat *);

/* util.c */
void startAutoKey(struct Bm2stat *, const char *, int);
int getAutoKey(struct Bm2stat *, int *, char *, int *);

/* depend.c */
void updateScreen(const struct Bm2stat *);
int updateKey(struct Bm2stat *);
void delay(int);
void updateCaption(const struct Bm2stat *);
void popup(const char *, ...);
int loadFontBmp(struct Bm2stat *, const char *);
int initDepend(const struct Bm2stat *, int, char *[]);

void reset(struct Bm2stat *bm2);
int m68states(const struct M68stat *m68);
int m68diff(int x, int y);

#ifdef __cplusplus
}
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
