/*
	�����x�[�V�b�N�}�X�^�[Jr.�G�~�����[�^
	�T�u���[�`��
*/

#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include "bm2.h"

#define MSTTOP	0x003b
#define MSTEND	0x003d
#define CPYTOP	0x003f
#define CPYEND	0x0041

/*
	�e�L�X�g���������� ($e14f) (m68subroutine�̉�����)
*/
static int clrtxt(struct M68stat *m68)
{
	for(m68->x = 0x100; m68->x < 0x400; m68->x++)
		m68write8(m68, m68->x, 0);
	return 6500;
}

/*
	�O���t�B�b�N�y�[�W1���������� ($e38d) (m68subroutine�̉�����)
*/
static int clrgrp1(struct M68stat *m68)
{
	for(m68->x = 0x900; m68->x < 0x2100; m68->x++)
		m68write8(m68, m68->x, 0);
	return 141000;
}

/*
	�O���t�B�b�N�y�[�W2���������� ($e39c) (m68subroutine�̉�����)
*/
static int clrgrp2(struct M68stat *m68)
{
	for(m68->x = 0x2100; m68->x < 0x3900; m68->x++)
		m68write8(m68, m68->x, 0);
	return 141000;
}

/*
	X := X + B ($f003) (m68subroutine�̉�����)
*/
static int addixb(struct M68stat *m68)
{
	m68->x += m68->b;
	return 40;
}

/*
	�u���b�N�]�� ($f009) (m68subroutine�̉�����)
*/
static int movblk(struct M68stat *m68)
{
	uint16 src, dst;

	for(
	src = m68read16(m68, MSTTOP), dst = m68read16(m68, CPYTOP);
	src != m68read16(m68, MSTEND) + 1;
	src++, dst++)
		m68write8(m68, dst, m68read8(m68, src));

	m68write16(m68, CPYEND, dst - 1);
	m68->x = dst - 1;

	return (int )(m68read16(m68, MSTEND) - m68read16(m68, MSTEND)) * 100;
}
	
/*
	�����o�͂��� ($f00c) (m68subroutine�̉�����)
*/
static int music(struct M68stat *m68)
{
	struct Bm2stat *bm2 = m68->tag;
	static int tempo = 80, len = 8;
	uint8 *p = &m68->m[m68->x];

	/*
	for(p = &m68->m[m68->x]; *p != 0 && *p != 0x0d; p++)
		printf("%c(%02x)", *p, *p);
	printf("\n");
	return 0;
	*/

	memset(bm2->sound_buffer, 0, bm2->sound_buffer_size);
	updateScreen(bm2);

	while(*p != 0x0d && *p != 0x00) {
		switch(*p++) {
		case '#':	/* �V���[�v */
		case 'B':	/* �t���b�g */
		case 'D':	/* ����D */
		case 'U':	/* ����U */
			break;
		case 'M':	/* �ڒ� */
			switch(*p++) {
			case 0:
				p--; break;
			case '#':
			case 'B':
				p++; break;
			}
			break;
		case 'V':	/* �{�����[�� */
		case 'Q':	/* ���F */
			switch(*p++) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
				break;
			default:
				p--; break;
			}
			break;
		case 'T':	/* �e���| */
			switch(*p++) {
			case 0:
				p--; break;
			case '1':
				tempo = 200; break;
			case '2':
				tempo = 133; break;
			case '3':
				tempo = 100; break;
			case '4':
				tempo = 80; break;
			case '5':
				tempo = 67; break;
			case '6':
				tempo = 57; break;
			case '7':
				tempo = 50; break;
			default:
				tempo = 80; p--; break;
			}
			break;
		case 'P':	/* ���̒��� */
			switch(*p++) {
			case 0:
				p--; break;
			case '0':
				len = 1; break;
			case '1':
				len = 2; break;
			case '2':
				len = 3; break;
			case '3':
				len = 4; break;
			case '4':
				len = 6; break;
			case '5':
				len = 8; break;
			case '6':
				len = 12; break;
			case '7':
				len = 16; break;
			case '8':
				len = 24; break;
			case '9':
				len = 32; break;
			default:
				len = 8; p--; break;
			}
			break;
		case 0xc4:	/* �h */
		case 0xda:	/* �� */
		case 0xd0:	/* �~ */
		case 0xcc:	/* �t�@ */
		case 0xbf:	/* �\ */
		case 0xd7:	/* �� */
		case 0xbc:	/* �V */
		case 'R':	/* �x�� */
			delay(bm2->fast ? 0: 60 * 1000 * len / 8 / tempo);
			break;
		}
	}
	return 0;
}

/*
	�L�[�𓾂� ($f00f) (m68subroutine�̉�����)
*/
static int kbin(struct M68stat *m68)
{
	struct Bm2stat *bm2 = m68->tag;
	int strobe, mask;
	const char *p;
	static const char key_normal[] = {
		'Z', 'A', 'Q', '1',
		'X', 'S', 'W', '2',
		'C', 'D', 'E', '3',
		'V', 'F', 'R', '4',
		'B', 'G', 'T', '5',
		'N', 'H', 'Y', '6',
		'M', 'J', 'U', '7',
		',', 'K', 'I', '8',
		'.', 'L', 'O', '9',
		'/', ';', 'P', '0',
		0,   ':', '@', '-',
		' ', ']', '[', '^',
		0, 0x0d, 0x7f, '\\'
	};
	static const char key_shift[] = {
		0,   0,   0, '!',
		0,   0,   0, '"',
		0,   0,   0, '#',
		0,   0,   0, '$',
		0,   0,   0, '%',
		0,   0,   0, '&',
		0,   0,   0, '\'',
		0,   0,   0, '(',
		0,   0,   0, ')',
		'?', '+', 0, 0,
		'_', '*', 0x0a, '=',
		0,   0,   0x0b, 0x09,
		0,   0,   0,    0x08
	};
	static const char key_kana[] = {
		0xc2, 0xc1, 0xc0, 0xc7,
		0xbb, 0xc4, 0xc3, 0xcb,
		0xbf, 0xbc, 0xb2, 0xb1,
		0xcb, 0xca, 0xbd, 0xb3,
		0xba, 0xb7, 0xb6, 0xb4,
		0xd0, 0xb8, 0xdd, 0xb5,
		0xd3, 0xcf, 0xc5, 0xd4,
		0xc8, 0xc9, 0xc6, 0xd5,
		0xd9, 0xd8, 0xd7, 0xd5,
		0xd2, 0xda, 0xbe, 0xd4,
		0xdb, 0xb9, 0xde, 0xce,
		0x20, 0xd1, 0xdf, 0xcd,
		0x00, 0x0d, 0x7f, 0xb0
	};
	static const char key_kana_shift[] = {
		0xaf, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xa8, 0xa7,
		0x00, 0x00, 0x00, 0xa9,
		0x00, 0x00, 0x00, 0xaa,
		0x00, 0x00, 0x00, 0xab,
		0x00, 0x00, 0x00, 0xac,
		0xa4, 0x00, 0x00, 0xad,
		0xa1, 0x00, 0x00, 0xae,
		0xa5, 0x00, 0x00, 0xa6,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	static const char *key = NULL;

	if(key == NULL)
		key = key_normal;
	if(!(bm2->key_mod & 0x10))
		key = key_normal;
	else if(!(bm2->key_mod & 0x20))
		key = key_shift;
	else if(!(bm2->key_mod & 0x40))
		key = key_kana_shift;
	else if(!(bm2->key_mod & 0x80))
		key = key_kana;
	p = key;

	for(strobe = 0x00; strobe <= 0x0c; strobe++)
		for(mask = 1; mask != 0x10; mask <<= 1) {
			if(*p != 0 && !(bm2->key_mat[strobe] & mask)) {
				m68->a = *p;
				m68->cc &= ~0x01;
				return 10000;
			}
			p++;
		}

	m68->a = 0x34;
	m68->cc |= 0x01;
	return 10000;
}

static int chrout(struct M68stat *);

/*
	�L�[�𓾂ĉ�ʂɏo�͂��� ($f012) (m68subroutine�̉�����)
*/
static int chrget(struct M68stat *m68)
{
	struct Bm2stat *bm2 = m68->tag;

	memset(bm2->sound_buffer, 0, bm2->sound_buffer_size);
	updateScreen(bm2);

	do {
		while(updateKey(bm2) == 0)
			;
		kbin(m68);
		delay(bm2->fast ? 0: 1000 / bm2->io_freq);
	} while(!(m68->cc & 0x01));

	do {
		while(updateKey(bm2) == 0)
			;
		kbin(m68);
		delay(bm2->fast ? 0: 1000 / bm2->io_freq);
	} while(m68->cc & 0x01);
	
	chrout(m68);
	return 0;
}

/*
	���ɃX�N���[������ (������)
*/
static void _scrollDown(struct M68stat *m68)
{
	uint16 p;

	for(p = 0x400 - 0x20; p >= 0x100; p--)
		m68write8(m68, p, m68read8(m68, p - 0x20));
	for(p = 0x100; p < 0x100 + 0x20; p++)
		m68write8(m68, p, 0x20);
}

/*
	��ɃX�N���[������ (������)
*/
static void _scrollUp(struct M68stat *m68)
{
	uint16 p;

	for(p = 0x100; p < 0x400 - 0x20; p++)
		m68write8(m68, p, m68read8(m68, p + 0x20));
	for(p = 0x400 - 0x20; p < 0x400; p++)
		m68write8(m68, p, 0x20);
}

/*
	��ɃX�N���[������ (������)
*/
static void scrollUp(struct M68stat *m68, uint16 *p)
{
	while(*p >= 0x400) {
		*p -= 0x20;
		_scrollUp(m68);
	}
}

/*
	��ʂ��������� (������)
*/
static void clear(struct M68stat *m68)
{
	uint16 p;

	for(p = 0x100; p < 0x400; p++)
		m68write8(m68, p, m68read8(m68, 0x20));
}

/*
	1�����\������ ($f015) (m68subroutine�̉�����)
*/
static int chrout(struct M68stat *m68)
{
	uint16 p;

	switch(m68->a) {
	case 0x01:	/* $00~$0F���o�� */
		return 0;
	case 0x02:	/* ��X�N���[�� */
		_scrollUp(m68);
		return 0;
	case 0x03:	/* ���X�N���[�� */
		_scrollDown(m68);
		return 0;
	case 0x00:
	case 0x04:	/* �������Ȃ� */
		return 0;
	case 0x05:	/* �J�[�\���\�� */
	case 0x06:	/* �J�[�\������ */
	case 0x07:	/* �x�� */
		return 0;
	case 0x0e:	/* ���] */
		m68write8(m68, 0xee40, 0xff);
		return 0;
	case 0x0f:	/* ���]���� */
		m68write8(m68, 0xee40, 0x00);
		return 0;
	}

	p = 0x100 + ((uint16 )m68read8(m68, 0x000f) + (uint16 )m68read8(m68, 0x0010) * 0x20) % 0x300;
	scrollUp(m68, &p);

	switch(m68->a) {
	case 0x08:	/* �J�[�\�����ړ� */
		if(p > 0x100)
			p--;
		break;
	case 0x09:	/* �J�[�\���E�ړ� */
		if(p < 0x400)
			p++;
		break;
	case 0x0a:	/* �J�[�\����ړ� */
		if(p < 0x400 - 0x20)
			p += 0x20;
		break;
	case 0x0b:	/* �J�[�\�����ړ� */
		if(p > 0x100 + 0x20)
			p -= 0x20;
		break;
	case 0x0c:	/* �N���A */
		clear(m68);
		p = 0x100;
		break;
	case 0x0d:	/* �J�[�\�������̍s�̐擪�Ɉړ����� */
		p = (((p - 0x100) / 0x20) * 0x20) + 0x120;
		break;
	case 0x7f:	/* DEL */
		if(p > 0x100)
			p--;
		m68write8(m68, p, 0x20);
		break;
	default:	/* �����o�� */
		m68write8(m68, p, m68->a);
		p++;
		break;
	}
	scrollUp(m68, &p);

	p -= 0x100;
	m68write8(m68, 0x000f, p % 32);
	m68write8(m68, 0x0010, p / 32);
	return 0;
}

/*
	�t�@�C�����𓾂� (������)
*/
static char *getFileName(struct M68stat *m68, uint16 address, char *file)
{
	uint16 p;
	char *q = file;

	for(p = address; p < address + 8; p++)
		if(m68read8(m68, p) != ' ')
			*q++ = m68read8(m68, p);
	*q = 0;

	return file;
}

/*
	�e�[�v����ǂݍ��� ($f018) (m68subroutine�̉�����)
*/
static int load(struct M68stat *m68)
{
	off_t last;
	uint16 start = m68read16(m68, 0x003b);
	uint16 name  = m68read16(m68, 0x0043);
	char buf[8 + 1];

	if(readSRecord(getFileName(m68, name, buf), &m68->m[start], &last, 0x10000, FALSE) < 0)
		return 0;
	m68write16(m68, 0x003d, last & 0xffff);

	m68->a = rand() & 0xff;
	m68->b = rand() & 0xff;
	m68->x = rand() & 0xff;

	return 0;
}

/*
	�e�[�v�ɏ������� ($f01b) (m68subroutine�̉�����)
*/
static int save(struct M68stat *m68)
{
	uint16 start = m68read16(m68, 0x003b);
	uint16 last  = m68read16(m68, 0x003d);
	uint16 name  = m68read16(m68, 0x0043);
	char buf[8 + 1];

	writeSRecord(getFileName(m68, name, buf), m68->m, start, last - start + 1);

	m68->a = rand() & 0xff;
	m68->b = rand() & 0xff;
	m68->x = rand() & 0xff;

	return 0;
}

/*
	�^�C�}���荞�� ($f04d) (m68subroutine�̉�����)
*/
static int timer(struct M68stat *m68)
{
	if(++m68->m[0x000c] >= 60) {
		m68->m[0x000c] = 0;

		if(++m68->m[0x000b] == 0)
			++m68->m[0x000a];
	}

	return 0;
}

/*
	BREAK���荞�݂��֎~���� ($ffe6) (m68subroutine�̉�����)
*/
static int nmiset(struct M68stat *m68)
{
	m68->a = 0x00;
	m68write8(m68, 0x0013, m68->a);
	m68write8(m68, 0xeec0, m68->a);
	return 20;
}

/*
	BREAK���荞�݂������� ($ffe9) (m68subroutine�̉�����)
*/
static int nmiclr(struct M68stat *m68)
{
	m68->a = 0xf0;
	m68write8(m68, 0x0013, m68->a);
	m68write8(m68, 0xeec0, m68->a);
	return 20;
}

/*
	X���W�X�^�̒l��\������ ($ffec) (m68subroutine�̉�����)
*/
static int outix(struct M68stat *m68)
{
	const char hex[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};

	m68->a = hex[(m68->x >> 12) & 0x0f];
	chrout(m68);
	m68->a = hex[(m68->x >> 8) & 0x0f];
	chrout(m68);
	m68->a = hex[(m68->x >> 4) & 0x0f];
	chrout(m68);
	m68->a = hex[m68->x & 0x0f];
	chrout(m68);
	return 400;
}

/*
	��ʂ��������� ($ffef) (m68subroutine�̉�����)
*/
static int clrtv(struct M68stat *m68)
{
	m68write16(m68, 0x000f, 0);
	m68->a = 0x20;
	for(m68->x = 0x100; m68->x < 0x400; m68->x++)
		m68write8(m68, m68->x, m68->a);

	return 1500;
}

/*
	�������\������ ($fff2) (m68subroutine�̉�����)
*/
static int mesout(struct M68stat *m68)
{
	int s = 0;

	while((m68->a = m68read8(m68, m68->x++)) != 0x04)
		s += chrout(m68);
	m68->x--;
	return s;
}

/*
	�J�[�\���ʒu�̃A�h���X�𓾂� ($fff5) (m68subroutine�̉�����)
*/
static int curpos(struct M68stat *m68)
{
	m68->x = 0x100 + (uint16 )m68read8(m68, 0x000f) + (uint16 )m68read8(m68, 0x0010) * 0x20;
	return 70;
}

/*
	�J�[�\���ʒu��ݒ肷��
*/
void locate(struct Bm2stat *bm2, int x, int y)
{
	m68write8(&bm2->cpu, 0x000f, x);
	m68write8(&bm2->cpu, 0x0010, y);
}

/*
	�������\������
*/
void print(struct Bm2stat *bm2, const char *str, ...)
{
	va_list v;
	uint8 a;
	char buf[256], *p;

	a = bm2->cpu.a;

	va_start(v, str);
	vsprintf(buf, str, v);
	va_end(v);

	for(p = buf; *p != 0; p++) {
		bm2->cpu.a = *p;
		chrout(&bm2->cpu);
	}
	updateScreen(bm2);

	bm2->cpu.a = a;
}

/*
	�����̓��͂𓾂�
*/
char getkey(struct Bm2stat *bm2)
{
	int key;

	while((key = updateKey(bm2)) == 0) {
		print(bm2, "*\x08");
		delay(bm2->fast ? 0: 100);
		print(bm2, " \x08");
		delay(bm2->fast ? 0: 100);
	}
	while(updateKey(bm2) != 0)
		delay(bm2->fast ? 0: 100);
	return key;
}

/*
	������̓��͂𓾂�
*/
static char _getstr(struct Bm2stat *bm2, char *buf, int flag)
{
	char c, *p = buf;

	print(bm2, buf);
	p = buf + strlen(buf);

	for(;;) {
		c = getkey(bm2);

		switch(c) {
		case 0:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			break;
		case '\t':
			return c;
		case '\r':
		case ' ':
			print(bm2, "\r");
			return '\r';
		case 0x7f:
			if(p <= buf)
				break;
			print(bm2, "%c", c);
			*--p = 0;
			if(flag)
				return 0;
			break;
		default:
			if(p >= buf + PATH_MAX)
				break;
			print(bm2, "%c", c);
			*p++ = c;
			*p = 0;
			if(flag)
				return 0;
			break;
		}
	}
}
char getstr(struct Bm2stat *bm2, char *buf)
{
	return _getstr(bm2, buf, FALSE);
}
char getkeystr(struct Bm2stat *bm2, char *buf)
{
	return _getstr(bm2, buf, TRUE);
}

/*
	16�i��4���̓��͂𓾂� (monitor�̉�����)
*/
static int gethex4(struct Bm2stat *bm2)
{
	int address;
	char c, buf[4 + 1] = "", *p = buf;

	for(;;) {
		c = getkey(bm2);

		switch(c) {
		case 0x0d:
			return -1;
		case ' ':
			sscanf(buf, "%x", &address);
			return address;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if(p >= buf + 4)
				break;
			print(bm2, "%c", toupper(c));
			*p++ = c;
			*p = 0;
			break;
		case 0x7f:
			print(bm2, "%c", c);
			if(p > buf)
				p--;
			*p = 0;
			break;
		default:
			break;
		}
	}
}

/*
	���j�^ ($f000) (m68subroutine�̉�����)
*/
static int monitor(struct M68stat *m68)
{
	struct Bm2stat *bm2 = m68->tag;
	off_t address;
	size_t size;
	char c, path[PATH_MAX];

	clrtv(m68);
	print(bm2, "* BASIC MASTER JR. EMULATOR BM2\r\r");

	for(;;) {
		print(bm2, "* INPUT COMMAND   ");
		c = getkey(bm2);

		switch(c) {
		case 0:
			break;
		case 'L':
		case 'l':
			print(bm2, "L\r\r PROGRAM NAME : ");
			inputFileName(bm2, path, FILTER_BINARY);
			print(bm2, "%s\r", path);
			if((size = readSRecord(path, m68->m, &address, 0x10000, FALSE)) > 0)
				print(bm2, "OK %04X-%04X\r", (int )address, (int )address + size - 1);
			else
				print(bm2, "NG\r");
			break;
		case 'G':
		case 'g':
			print(bm2, "G\r\rSTART ADDRESS: ");
			if((address = gethex4(bm2)) < 0)
				break;
			m68write8(m68, m68->sp, address & 0x00ff);
			m68->sp--;
			m68write8(m68, m68->sp, address >> 8);
			m68->sp--;
			goto last;
		default:
			print(bm2, "?\r");
			break;
		}
		print(bm2, "\r");
	}

last:;
	return 0;
}

/*
	�T�u���[�`�����G�~�����[�g����
*/
int m68subroutine(struct M68stat *m68, uint16 address)
{
	if(address < 0xe000)
		return -1;

#if 0
	printf("MONITOR SUBROUTINE %04x\n", address);
	fflush(stdout);
#endif

	switch(address) {
	case 0xe14f:
		return clrtxt(m68);
	case 0xe38d:
		return clrgrp1(m68);
	case 0xe39c:
		return clrgrp2(m68);
	case 0xf000:
		return monitor(m68);
	case 0xf003:
		return addixb(m68);
	case 0xf009:
		return movblk(m68);
	case 0xf00c:
		return music(m68);
	case 0xf00f:
		return kbin(m68);
	case 0xf012:
		return chrget(m68);
	case 0xf015:
		return chrout(m68);
	case 0xf018:
		return load(m68);
	case 0xf01b:
		return save(m68);
	case 0xf04d:
		return timer(m68);
	case 0xffe6:
		return nmiset(m68);
	case 0xffe9:
		return nmiclr(m68);
	case 0xffec:
		return outix(m68);
	case 0xffef:
		return clrtv(m68);
	case 0xfff2:
		return mesout(m68);
	case 0xfff5:
		return curpos(m68);
#if 0
	default:
		printf("$%04X\n", address);
		fflush(stdout);
#endif
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
