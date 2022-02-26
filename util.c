/*
	�����x�[�V�b�N�}�X�^�[Jr.�G�~�����[�^
	���[�e�B���e�B
*/

#include <stdio.h>
#include <string.h>
#include "bm2.h"

/* �������ق��̒l��߂� */
#define MIN(x, y)	((x) < (y) ? (x): (y))

static char *autoKeyBuffer = NULL;	/* �����L�[���͂̃o�b�t�@ */
static char *autoKeyPointer = NULL;	/* �����L�[���͂̓ǂݍ��݃|�C���^ */
static int autoKeyCount = 0;	/* �����L�[���͂̃J�E���^ */
static int autoKeyLastStates;	/* �Ō�ɃL�[���͂������_�̗ݐσX�e�[�g�� */

/*
	UTF-8��JIS X 0201�ɕϊ����� (startAutoKey�̉�����)
*/
static int convUtf8ToJisX0201(char *jis, const char *utf8)
{
	int utf16;

	/* ASCII�R�[�h�͂��̂܂ܖ߂� */
	if((unsigned int )utf8[0] < 0x80) {
		*jis = utf8[0];
		return 1;
	}

	/* CR+LF��LF�ɂ��� */
	if(utf8[0] == 0x0d && utf8[1] == 0x0a) {
		*jis = 0x0a;
		return 2;
	}

	/* ���p�J�i��ϊ����� */
	if((utf8[0] & 0xe0) == 0xc0) {
		*jis = 0x20;
		return 2;
	} else {
		utf16 = (utf8[0] & 0x0fU) << 12U | (utf8[1] & 0x3fU) << 6U | (utf8[2] & 0x3fU);
		if(0xff61 <= utf16 && utf16 <= 0xff9f)
			*jis = utf16 - 0xff60U + 0xa0;
		else
			*jis = 0x20;
		return 3;
	}
}

/*
	�����L�[���͂��n�߂�
*/
void startAutoKey(struct Bm2stat *bm2, const char *str, int utf8)
{
	int len = strlen(str);
	const char *p;
	char *q;

	len = MIN(len, 0x8000);

	if(autoKeyBuffer == NULL)
		autoKeyBuffer = malloc(len + 1);
	else
		autoKeyBuffer = realloc(autoKeyBuffer, len + 1);

	memset(autoKeyBuffer, 0, len + 1);
	if(utf8)
		for(p = str, q = autoKeyBuffer; *p != 0; p += convUtf8ToJisX0201(q++, p))
			;
	else
		memcpy(autoKeyBuffer, str, len);

	autoKeyPointer = autoKeyBuffer;
	autoKeyCount = 0;
	autoKeyLastStates = m68states(&bm2->cpu);
	bm2->fast = TRUE;
}

/*
	�������̓L�[�𓾂�
*/
int getAutoKey(struct Bm2stat *bm2, int *press, char *ch, int *key)
{
	const struct {
		int mod;	/* ���f�t�@�C�A�L�[ */
		int key;	/* �L�[ */
	} table[] = {
		{ BMKEY_ALPHA, BMKEY_NONE },	/* NUL */
		{ 0 },	/* SOH */
		{ 0 },	/* STX */
		{ 0 },	/* ETX */
		{ 0 },	/* EOT */
		{ 0 },	/* ENQ */
		{ 0 },	/* ACK */
		{ 0 },	/* BEL */

		{ BMKEY_NONE, BMKEY_DELETE },	/* BS */
		{ 0 },	/* HT */
		{ BMKEY_NONE, BMKEY_RETURN },	/* LF */
		{ 0 },	/* VT */
		{ 0 },	/* FF */
		{ 0 },	/* CR */
		{ 0 },	/* SO */
		{ 0 },	/* SI */

		{ 0 },	/* DLE */
		{ 0 },	/* DC1 */
		{ 0 },	/* DC2 */
		{ 0 },	/* DC3 */
		{ 0 },	/* DC4 */
		{ 0 },	/* NAK */
		{ 0 },	/* SYN */
		{ 0 },	/* ETB */

		{ 0 },	/* CAN */
		{ 0 },	/* EM */
		{ 0 },	/* SUB */
		{ 0 },	/* ESC */
		{ 0 },	/* FS */
		{ 0 },	/* GS */
		{ 0 },	/* RS */
		{ 0 },	/* US */

		{ BMKEY_NONE, BMKEY_SPACE },	/* SPACE */
		{ BMKEY_ASHIFT, BMKEY_1 },	/* ! */
		{ BMKEY_ASHIFT, BMKEY_2 },	/* " */
		{ BMKEY_ASHIFT, BMKEY_3 },	/* # */
		{ BMKEY_ASHIFT, BMKEY_4 },	/* $ */
		{ BMKEY_ASHIFT, BMKEY_5 },	/* % */
		{ BMKEY_ASHIFT, BMKEY_6 },	/* & */
		{ BMKEY_ASHIFT, BMKEY_7 },	/* ' */

		{ BMKEY_ASHIFT, BMKEY_8 },		/* ( */
		{ BMKEY_ASHIFT, BMKEY_9 },		/* ) */
		{ BMKEY_ASHIFT, BMKEY_COLON },	/* * */
		{ BMKEY_ASHIFT, BMKEY_SEMICOLON },	/* + */
		{ BMKEY_ALPHA, BMKEY_COMMA },	/* , */
		{ BMKEY_ALPHA, BMKEY_MINUS },	/* - */
		{ BMKEY_ALPHA, BMKEY_PERIOD },	/* . */
		{ BMKEY_ALPHA, BMKEY_SLASH },	/* / */

		{ BMKEY_ALPHA, BMKEY_0 },	/* 0 */
		{ BMKEY_ALPHA, BMKEY_1 },	/* 1 */
		{ BMKEY_ALPHA, BMKEY_2 },	/* 2 */
		{ BMKEY_ALPHA, BMKEY_3 },	/* 3 */
		{ BMKEY_ALPHA, BMKEY_4 },	/* 4 */
		{ BMKEY_ALPHA, BMKEY_5 },	/* 5 */
		{ BMKEY_ALPHA, BMKEY_6 },	/* 6 */
		{ BMKEY_ALPHA, BMKEY_7 },	/* 7 */

		{ BMKEY_ALPHA, BMKEY_8 },	/* 8 */
		{ BMKEY_ALPHA, BMKEY_9 },	/* 9 */
		{ BMKEY_ALPHA, BMKEY_COLON },	/* : */
		{ BMKEY_ALPHA, BMKEY_SEMICOLON },	/* ; */
		{ BMKEY_ASHIFT, BMKEY_COMMA },	/* < */
		{ BMKEY_ASHIFT, BMKEY_MINUS },	/* = */
		{ BMKEY_ASHIFT, BMKEY_PERIOD },	/* > */
		{ BMKEY_ASHIFT, BMKEY_SLASH },	/* ? */

		{ BMKEY_ALPHA, BMKEY_AT },	/* @ */
		{ BMKEY_ALPHA, BMKEY_A },	/* A */
		{ BMKEY_ALPHA, BMKEY_B },	/* B */
		{ BMKEY_ALPHA, BMKEY_C },	/* C */
		{ BMKEY_ALPHA, BMKEY_D },	/* D */
		{ BMKEY_ALPHA, BMKEY_E },	/* E */
		{ BMKEY_ALPHA, BMKEY_F },	/* F */
		{ BMKEY_ALPHA, BMKEY_G },	/* G */

		{ BMKEY_ALPHA, BMKEY_H },	/* H */
		{ BMKEY_ALPHA, BMKEY_I },	/* I */
		{ BMKEY_ALPHA, BMKEY_J },	/* J */
		{ BMKEY_ALPHA, BMKEY_K },	/* K */
		{ BMKEY_ALPHA, BMKEY_L },	/* L */
		{ BMKEY_ALPHA, BMKEY_M },	/* M */
		{ BMKEY_ALPHA, BMKEY_N },	/* N */
		{ BMKEY_ALPHA, BMKEY_O },	/* O */

		{ BMKEY_ALPHA, BMKEY_P },	/* P */
		{ BMKEY_ALPHA, BMKEY_Q },	/* Q */
		{ BMKEY_ALPHA, BMKEY_R },	/* R */
		{ BMKEY_ALPHA, BMKEY_S },	/* S */
		{ BMKEY_ALPHA, BMKEY_T },	/* T */
		{ BMKEY_ALPHA, BMKEY_U },	/* U */
		{ BMKEY_ALPHA, BMKEY_V },	/* V */
		{ BMKEY_ALPHA, BMKEY_W },	/* W */

		{ BMKEY_ALPHA, BMKEY_X },	/* X */
		{ BMKEY_ALPHA, BMKEY_Y },	/* Y */
		{ BMKEY_ALPHA, BMKEY_Z },	/* Z */
		{ BMKEY_ALPHA, BMKEY_LEFTBRACKET },	/* [ */
		{ BMKEY_ALPHA, BMKEY_YEN },	/* \ */
		{ BMKEY_ALPHA, BMKEY_RIGHTBRACKET },	/* ] */
		{ BMKEY_ALPHA, BMKEY_CARET },	/* ^ */
		{ BMKEY_ASHIFT, BMKEY_UNDERBAR },	/* _ */

		{ 0 },	/* ` */
		{ BMKEY_ALPHA, BMKEY_A },	/* a */
		{ BMKEY_ALPHA, BMKEY_B },	/* b */
		{ BMKEY_ALPHA, BMKEY_C },	/* c */
		{ BMKEY_ALPHA, BMKEY_D },	/* d */
		{ BMKEY_ALPHA, BMKEY_E },	/* e */
		{ BMKEY_ALPHA, BMKEY_F },	/* f */
		{ BMKEY_ALPHA, BMKEY_G },	/* g */

		{ BMKEY_ALPHA, BMKEY_H },	/* h */
		{ BMKEY_ALPHA, BMKEY_I },	/* i */
		{ BMKEY_ALPHA, BMKEY_J },	/* j */
		{ BMKEY_ALPHA, BMKEY_K },	/* k */
		{ BMKEY_ALPHA, BMKEY_L },	/* l */
		{ BMKEY_ALPHA, BMKEY_M },	/* m */
		{ BMKEY_ALPHA, BMKEY_N },	/* n */
		{ BMKEY_ALPHA, BMKEY_O },	/* o */

		{ BMKEY_ALPHA, BMKEY_P },	/* p */
		{ BMKEY_ALPHA, BMKEY_Q },	/* q */
		{ BMKEY_ALPHA, BMKEY_R },	/* r */
		{ BMKEY_ALPHA, BMKEY_S },	/* s */
		{ BMKEY_ALPHA, BMKEY_T },	/* t */
		{ BMKEY_ALPHA, BMKEY_U },	/* u */
		{ BMKEY_ALPHA, BMKEY_V },	/* v */
		{ BMKEY_ALPHA, BMKEY_W },	/* w */

		{ BMKEY_ALPHA, BMKEY_X },	/* x */
		{ BMKEY_ALPHA, BMKEY_Y },	/* y */
		{ BMKEY_ALPHA, BMKEY_Z },	/* z */
		{ 0 },	/* { */
		{ 0 },	/* | */
		{ 0 },	/* } */
		{ 0 },	/* ~ */
		{ 0 },	/* DEL */

		{ 0 },	/* 0x80 */
		{ 0 },	/* 0x81 */
		{ 0 },	/* 0x82 */
		{ 0 },	/* 0x83 */
		{ 0 },	/* 0x84 */
		{ 0 },	/* 0x85 */
		{ 0 },	/* 0x86 */
		{ 0 },	/* 0x87 */

		{ 0 },	/* 0x88 */
		{ 0 },	/* 0x89 */
		{ 0 },	/* 0x8a */
		{ 0 },	/* 0x8b */
		{ 0 },	/* 0x8c */
		{ 0 },	/* 0x8d */
		{ 0 },	/* 0x8e */
		{ 0 },	/* 0x8f */

		{ 0 },	/* 0x90 */
		{ 0 },	/* 0x91 */
		{ 0 },	/* 0x92 */
		{ 0 },	/* 0x93 */
		{ 0 },	/* 0x94 */
		{ 0 },	/* 0x95 */
		{ 0 },	/* 0x96 */
		{ 0 },	/* 0x97 */

		{ 0 },	/* 0x98 */
		{ 0 },	/* 0x99 */
		{ 0 },	/* 0x9a */
		{ 0 },	/* 0x9b */
		{ 0 },	/* 0x9c */
		{ 0 },	/* 0x9d */
		{ 0 },	/* 0x9e */
		{ 0 },	/* 0x9f */

		{ 0 },	/* 0xa0 */
		{ BMKEY_KSHIFT, BMKEY_PERIOD },	/* �B */
		{ BMKEY_KSHIFT, BMKEY_LEFTBRACKET },	/* �u */
		{ BMKEY_KSHIFT, BMKEY_RIGHTBRACKET },	/* �v */
		{ BMKEY_KSHIFT, BMKEY_COMMA },	/* �A */
		{ BMKEY_KSHIFT, BMKEY_SLASH },	/* �E */
		{ BMKEY_KSHIFT, BMKEY_0 },	/* �� */
		{ BMKEY_KSHIFT, BMKEY_3 },	/* �@ */

		{ BMKEY_KSHIFT, BMKEY_E },	/* �B */
		{ BMKEY_KSHIFT, BMKEY_4 },	/* �D */
		{ BMKEY_KSHIFT, BMKEY_5 },	/* �F */
		{ BMKEY_KSHIFT, BMKEY_6 },	/* �H */
		{ BMKEY_KSHIFT, BMKEY_7 },	/* �� */
		{ BMKEY_KSHIFT, BMKEY_8 },	/* �� */
		{ BMKEY_KSHIFT, BMKEY_9 },	/* �� */
		{ BMKEY_KSHIFT, BMKEY_Z },	/* �b */

		{ BMKEY_KANA, BMKEY_YEN },	/* �[ */
		{ BMKEY_KANA, BMKEY_3 },	/* �A */
		{ BMKEY_KANA, BMKEY_E },	/* �C */
		{ BMKEY_KANA, BMKEY_4 },	/* �E */
		{ BMKEY_KANA, BMKEY_5 },	/* �G */
		{ BMKEY_KANA, BMKEY_6 },	/* �I */
		{ BMKEY_KANA, BMKEY_T },	/* �J */
		{ BMKEY_KANA, BMKEY_G },	/* �L */

		{ BMKEY_KANA, BMKEY_H },	/* �N */
		{ BMKEY_KANA, BMKEY_COLON },	/* �P */
		{ BMKEY_KANA, BMKEY_B },	/* �R */
		{ BMKEY_KANA, BMKEY_X },	/* �T */
		{ BMKEY_KANA, BMKEY_D },	/* �V */
		{ BMKEY_KANA, BMKEY_R },	/* �X */
		{ BMKEY_KANA, BMKEY_P },	/* �Z */
		{ BMKEY_KANA, BMKEY_C },	/* �\ */

		{ BMKEY_KANA, BMKEY_Q },	/* �^ */
		{ BMKEY_KANA, BMKEY_A },	/* �` */
		{ BMKEY_KANA, BMKEY_Z },	/* �c */
		{ BMKEY_KANA, BMKEY_W },	/* �e */
		{ BMKEY_KANA, BMKEY_S },	/* �g */
		{ BMKEY_KANA, BMKEY_U },	/* �i */
		{ BMKEY_KANA, BMKEY_I },	/* �j */
		{ BMKEY_KANA, BMKEY_1 },	/* �k */

		{ BMKEY_KANA, BMKEY_COMMA },	/* �l */
		{ BMKEY_KANA, BMKEY_K },	/* �m */
		{ BMKEY_KANA, BMKEY_F },	/* �n */
		{ BMKEY_KANA, BMKEY_V },	/* �q */
		{ BMKEY_KANA, BMKEY_2 },	/* �t */
		{ BMKEY_KANA, BMKEY_CARET },	/* �w */
		{ BMKEY_KANA, BMKEY_MINUS },	/* �z */
		{ BMKEY_KANA, BMKEY_J },	/* �} */

		{ BMKEY_KANA, BMKEY_N },	/* �~ */
		{ BMKEY_KANA, BMKEY_RIGHTBRACKET },	/* �� */
		{ BMKEY_KANA, BMKEY_SLASH },	/* �� */
		{ BMKEY_KANA, BMKEY_M },	/* �� */
		{ BMKEY_KANA, BMKEY_7 },	/* �� */
		{ BMKEY_KANA, BMKEY_8 },	/* �� */
		{ BMKEY_KANA, BMKEY_9 },	/* �� */
		{ BMKEY_KANA, BMKEY_O },	/* �� */

		{ BMKEY_KANA, BMKEY_L },	/* �� */
		{ BMKEY_KANA, BMKEY_PERIOD },	/* �� */
		{ BMKEY_KANA, BMKEY_SEMICOLON },	/* �� */
		{ BMKEY_KANA, BMKEY_UNDERBAR },	/* �� */
		{ BMKEY_KANA, BMKEY_0 },	/* �� */
		{ BMKEY_KANA, BMKEY_Y },	/* �� */
		{ BMKEY_KANA, BMKEY_AT },	/* �J */
		{ BMKEY_KANA, BMKEY_LEFTBRACKET },	/* �K */

		{ 0 },	/* 0xe0 */
		{ 0 },	/* 0xe1 */
		{ 0 },	/* 0xe2 */
		{ 0 },	/* 0xe3 */
		{ 0 },	/* 0xe4 */
		{ 0 },	/* 0xe5 */
		{ 0 },	/* 0xe6 */
		{ 0 },	/* 0xe7 */

		{ 0 },	/* 0xe8 */
		{ 0 },	/* 0xe9 */
		{ 0 },	/* 0xea */
		{ 0 },	/* 0xeb */
		{ 0 },	/* 0xec */
		{ 0 },	/* 0xed */
		{ 0 },	/* 0xee */
		{ 0 },	/* 0xef */

		{ 0 },	/* 0xf0 */
		{ 0 },	/* 0xf1 */
		{ 0 },	/* 0xf2 */
		{ 0 },	/* 0xf3 */
		{ 0 },	/* 0xf4 */
		{ 0 },	/* 0xf5 */
		{ 0 },	/* 0xf6 */
		{ 0 },	/* 0xf7 */

		{ 0 },	/* 0xf8 */
		{ 0 },	/* 0xf9 */
		{ 0 },	/* 0xfa */
		{ 0 },	/* 0xfb */
		{ 0 },	/* 0xfc */
		{ 0 },	/* 0xfd */
		{ 0 },	/* 0xfe */
		{ 0 }	/* 0xff */
	};

	/* �����L�[���͂��Ȃ���? */
	if(autoKeyPointer == NULL)
		return FALSE;

	/* �L�[���͌�̑҂����Ԃ�? */
	if(!bm2->cpu.emulate_subroutine)
		if(m68diff(m68states(&bm2->cpu), autoKeyLastStates) < 100000)
			return FALSE;
	autoKeyLastStates = m68states(&bm2->cpu);

	/* �����ƃL�[�𓾂� */
	*ch = *autoKeyPointer;

	switch(autoKeyCount++) {
	case 0:
		if(table[(unsigned char )*ch].mod == BMKEY_NONE)
			return FALSE;

		/* ���f�t�@�C�A�L�[������ */
		*press = TRUE;
		*key = table[(unsigned char )*ch].mod;
		*ch = 0;
		return TRUE;
	case 1:
		if(table[(unsigned char )*ch].key == BMKEY_NONE)
			return FALSE;

		/* �L�[������ */
		*press = TRUE;
		*key = table[(unsigned char )*ch].key;
		return TRUE;
	case 2:
		if(table[(unsigned char )*ch].key == BMKEY_NONE)
			return FALSE;

		/* �L�[����� */
		*press = FALSE;
		*key = table[(unsigned char )*ch].key;
		return TRUE;
	case 3:
		/* ���f�t�@�C�A�L�[����� */
		if(table[(unsigned char )*ch].mod == BMKEY_NONE)
			return FALSE;

		*press = FALSE;
		*key = table[(unsigned char )*ch].mod;
		*ch = 0;
		return TRUE;
	default:
		autoKeyCount = 0;

		if(*autoKeyPointer == 0) {
			/* �����L�[���͏I�� */
			autoKeyPointer = NULL;
			bm2->fast = FALSE;
		} else {
			/* ���̕����ֈڂ� */
			autoKeyPointer++;
		}
		return FALSE;
	}
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
