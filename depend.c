/*
	日立ベーシックマスターJr.エミュレータ
	環境依存
*/

#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include "bm2.h"
#include "sdlxpm.h"
#include "bm2icon.xpm"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include "windows.h"
#endif

#define CHR_WIDTH	8	/* 文字の幅 */
#define CHR_HEIGHT	8	/* 文字の高さ */
#define COLS	32	/* 横の文字数 */
#define ROWS	24	/* 縦の文字数 */
#define BORDER_WIDTH	32	/* 枠の幅 */
#define BORDER_HEIGHT	24	/* 枠の高さ */

#if SDL_MAJOR_VERSION == 2
static SDL_Window *window = NULL;	/* ウィンドウ */
#elif SDL_MAJOR_VERSION == 1
const static SDL_VideoInfo *video = NULL;	/* ビデオ情報 */
#endif
static SDL_Surface *screen;		/* ディスプレイのサーフェイス */
static int zoom;			/* 画面の倍率 */
static int fullLine;			/* 全ライン表示か? */
static Uint8 pixMap[8][16];		/* pixmap */

static uint8 vram[ROWS * COLS * CHR_WIDTH];	/* 仮想VRAM */
static uint8 oldVram[ROWS * COLS * CHR_WIDTH];	/* 前のフレームの仮想VRAM */
static uint8 colorMap[ROWS * COLS];		/* カラーマップ */
static uint8 oldColorMap[ROWS * COLS];		/* 前のフレームのカラーマップ */
static uint8 colorMapMono[ROWS * COLS];		/* カラーアダプタがない場合のカラーマップ */
static uint8 oldBckReg;
static uint8 oldReverse;

static SDL_AudioSpec audio;		/* 音声情報 */
static uint8 *soundBuffer;		/* 音声バッファ */
static int soundBufferSize;		/* 音声バッファのサイズ */
static uint8 **soundReadPointer;	/* 音声読込ポインタへのポインタ */
static uint8 **soundWritePointer;	/* 音声書込ポインタへのポインタ */

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#if SDL_MAJOR_VERSION == 2
/*
	UTF-8をShift-JISに変換する (下請け)
*/
static char *convertUtf8ToSjis(char *outstr, int outsize, const char *instr)
{
	char utf16str[PATH_MAX * 2];

	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR )instr, -1, (LPWSTR )utf16str, sizeof(utf16str));
	WideCharToMultiByte(CP_ACP, 0, (LPCWSTR )utf16str, -1, (LPSTR )outstr, outsize, NULL, NULL);
	return outstr;
}

/*
	Shift-JISをUTF-8に変換する
*/
static char *convertSjisToUtf8(char *outstr, int outsize, char *instr)
{
	char utf16str[PATH_MAX * 2];
	char sjisstr[PATH_MAX];

	MultiByteToWideChar(CP_ACP, 0, (LPCSTR )instr, -1, (LPWSTR )utf16str, sizeof(utf16str));
	if(outstr != NULL) {
		WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR )utf16str, -1, (LPSTR )outstr, outsize, NULL, NULL);
		return outstr;
	} else {
		WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR )utf16str, -1, (LPSTR )sjisstr, sizeof(sjisstr), NULL, NULL);
		strncpy(instr, sjisstr, outsize);
		return instr;
	}
}

#endif
#endif

/*
	水平な線を描く (下請け)
*/
static inline void putline(int y, int x, int width, uint8 color, int full_line)
{
	int zoomed_bpp = screen->format->BytesPerPixel * zoom;
	int zoomed_pitch = screen->pitch * zoom;
	Uint8 *pixmap = pixMap[color & 0x07], *p = (Uint8 *)screen->pixels + y * zoomed_pitch + x * zoomed_bpp, *q;

	for(q = p; q < p + width * zoomed_bpp; q += zoomed_bpp)
		memcpy(q, pixmap, zoomed_bpp);
	if(full_line)
		for(q = p + 1 * screen->pitch; q < p + zoom * screen->pitch; q += screen->pitch)
			memcpy(q, p, width * zoomed_bpp);
}

/*
	パターンを1行描く (putpatの下請け)
*/
static inline void putpat1(int col, int y, uint8 color, uint8 pat)
{
	int zoomed_bpp, zoomed_pitch, i;
	const Uint8 *fore, *back;
	Uint8 *p;

	zoomed_bpp = screen->format->BytesPerPixel * zoom;
	zoomed_pitch = screen->pitch * zoom;
	p = (Uint8 *)screen->pixels + (y + BORDER_HEIGHT) * zoomed_pitch + (col * CHR_WIDTH + BORDER_WIDTH) * zoomed_bpp;

	fore = pixMap[color & 7];
	back = pixMap[color >> 4];

	memcpy(p + zoomed_bpp * 0, pat & 0x80 ? fore: back, zoomed_bpp);
	memcpy(p + zoomed_bpp * 1, pat & 0x40 ? fore: back, zoomed_bpp);
	memcpy(p + zoomed_bpp * 2, pat & 0x20 ? fore: back, zoomed_bpp);
	memcpy(p + zoomed_bpp * 3, pat & 0x10 ? fore: back, zoomed_bpp);
	memcpy(p + zoomed_bpp * 4, pat & 0x08 ? fore: back, zoomed_bpp);
	memcpy(p + zoomed_bpp * 5, pat & 0x04 ? fore: back, zoomed_bpp);
	memcpy(p + zoomed_bpp * 6, pat & 0x02 ? fore: back, zoomed_bpp);
	memcpy(p + zoomed_bpp * 7, pat & 0x01 ? fore: back, zoomed_bpp);

	if(fullLine)
		for(i = 1; i < zoom; i++)
			memcpy(p + screen->pitch * i, p, zoomed_bpp * CHR_WIDTH);
}

/*
	パターンを描く (下請け)
*/
static inline void putpat(int col, int row, uint8 color, int blink, uint8 pat0, uint8 pat1, uint8 pat2, uint8 pat3, uint8 pat4, uint8 pat5, uint8 pat6, uint8 pat7)
{
	uint8 *v0, *v1, *v2, *v3, *v4, *v5, *v6, *v7, *c = &colorMap[row * COLS + col];

	/* 色を得る */
	if(blink) {
		if(color & 0x08)
			color &= 0x70;
		else
			color &= 0x77;
	} else {
		if(color & 0x80)
			color &= 0x07;
		else
			color &= 0x77;
	}

	/* 仮想VRAMのアドレスを得る */
	v0 = &vram[row * COLS * CHR_WIDTH + col];
	v1 = v0 + COLS;
	v2 = v1 + COLS;
	v3 = v2 + COLS;
	v4 = v3 + COLS;
	v5 = v4 + COLS;
	v6 = v5 + COLS;
	v7 = v6 + COLS;

	/* 仮想VRAMを更新し表示する */
	if(*c != color) {
		*c = color;
		*v0 = pat0;
		*v1 = pat1;
		*v2 = pat2;
		*v3 = pat3;
		*v4 = pat4;
		*v5 = pat5;
		*v6 = pat6;
		*v7 = pat7;
		putpat1(col, row * CHR_HEIGHT + 0, color, pat0);
		putpat1(col, row * CHR_HEIGHT + 1, color, pat1);
		putpat1(col, row * CHR_HEIGHT + 2, color, pat2);
		putpat1(col, row * CHR_HEIGHT + 3, color, pat3);
		putpat1(col, row * CHR_HEIGHT + 4, color, pat4);
		putpat1(col, row * CHR_HEIGHT + 5, color, pat5);
		putpat1(col, row * CHR_HEIGHT + 6, color, pat6);
		putpat1(col, row * CHR_HEIGHT + 7, color, pat7);
	} else {
		if(*v0 != pat0) {
			*v0 = pat0;
			putpat1(col, row * CHR_HEIGHT + 0, color, pat0);
		}
		if(*v1 != pat1) {
			*v1 = pat1;
			putpat1(col, row * CHR_HEIGHT + 1, color, pat1);
		}
		if(*v2 != pat2) {
			*v2 = pat2;
			putpat1(col, row * CHR_HEIGHT + 2, color, pat2);
		}
		if(*v3 != pat3) {
			*v3 = pat3;
			putpat1(col, row * CHR_HEIGHT + 3, color, pat3);
		}
		if(*v4 != pat4) {
			*v4 = pat4;
			putpat1(col, row * CHR_HEIGHT + 4, color, pat4);
		}
		if(*v5 != pat5) {
			*v5 = pat5;
			putpat1(col, row * CHR_HEIGHT + 5, color, pat5);
		}
		if(*v6 != pat6) {
			*v6 = pat6;
			putpat1(col, row * CHR_HEIGHT + 6, color, pat6);
		}
		if(*v7 != pat7) {
			*v7 = pat7;
			putpat1(col, row * CHR_HEIGHT + 7, color, pat7);
		}
	}
}

/*
	ディスプレイを更新する(テキストモード) (updateScreenの下請け)
*/
static inline void updateScreenText(const struct Bm2stat *bm2, uint16 p, int blink)
{
	int col, row;
	const uint8 *color = (bm2->mp1710on ? bm2->color_map: colorMapMono), *chr = &bm2->cpu.m[p];

	for(row = 0; row < ROWS; row++)
		for(col = 0; col < COLS; col++) {
			putpat(
			col,
			row,
			bm2->reverse ? *color << 4 | *color >> 4: *color,
			blink,
			bm2->rom_font[*chr][0],
			bm2->rom_font[*chr][1],
			bm2->rom_font[*chr][2],
			bm2->rom_font[*chr][3],
			bm2->rom_font[*chr][4],
			bm2->rom_font[*chr][5],
			bm2->rom_font[*chr][6],
			bm2->rom_font[*chr][7]
			);
			color++;
			chr++;
		}
}

/*
	ディスプレイを更新する(グラフィックモード) (updateScreenの下請け)
*/
static inline void updateScreenGraphic(const struct Bm2stat *bm2, uint16 p, int blink)
{
	int col, row;
	const uint8 *color = (bm2->mp1710on ? bm2->color_map: colorMapMono), *pat = &bm2->cpu.m[p];

	for(row = 0; row < ROWS; row++) {
		for(col = 0; col < COLS; col++) {
			putpat(
			col,
			row,
			bm2->reverse ? *color << 4 | *color >> 4: *color,
			blink,
			pat[COLS * 0],
			pat[COLS * 1],
			pat[COLS * 2],
			pat[COLS * 3],
			pat[COLS * 4],
			pat[COLS * 5],
			pat[COLS * 6],
			pat[COLS * 7]
			);
			color++;
			pat++;
		}
		pat += COLS * (CHR_HEIGHT - 1);
	}
}

/*
	ディスプレイの背景を更新する (updateScreenの下請け)
*/
static inline void updateScreenBack(const struct Bm2stat *bm2)
{
	int y;
	Uint8 color = (bm2->mp1710on ? bm2->bckreg: 0x70);

	if(bm2->reverse)
		color = color << 4 | color >> 4;

	for(y = 0; y < BORDER_HEIGHT; y++)
		putline(y, 0, BORDER_WIDTH + COLS * CHR_WIDTH + BORDER_WIDTH, color, bm2->full_line);
	for(y = 0; y < BORDER_HEIGHT + ROWS * CHR_HEIGHT; y++) {
		putline(y, 0, BORDER_WIDTH, color, bm2->full_line);
		putline(y, BORDER_WIDTH + COLS * CHR_WIDTH, BORDER_WIDTH, color, bm2->full_line);
	}
	for(y = BORDER_HEIGHT + ROWS * CHR_HEIGHT; y < BORDER_HEIGHT + ROWS * CHR_HEIGHT + BORDER_HEIGHT; y++)
		putline(y, 0, BORDER_WIDTH + COLS * CHR_WIDTH + BORDER_WIDTH, color, bm2->full_line);
}

/*
	ディスプレイを更新する
*/
void updateScreen(const struct Bm2stat *bm2)
{
	static int count = 0, blink = 0;
	int col, row;
	SDL_Rect rect[ROWS * COLS], *r;

	if(count++ > bm2->io_freq / 2) {
		count = 0;
		blink ^= 1;
	}

	/* 背景をサーフェイスに書き込む */
	if(bm2->bckreg != oldBckReg || bm2->reverse != oldReverse)
		updateScreenBack(bm2);

	/* サーフェイスに書き込む */
	switch(bm2->screen_mode) {
	case 0x00:	/* テキスト */
		updateScreenText(bm2, 0x100, blink);
		break;
	case 0x40:	/* テキスト・グラフィックページ1混合 */
		if(count & 1)
			updateScreenText(bm2, 0x100, blink);
		else
			updateScreenGraphic(bm2, 0x900, blink);
		break;
	case 0x4c:	/* テキスト・グラフィックページ2混合 */
		if(count & 1)
			updateScreenText(bm2, 0x100, blink);
		else
			updateScreenGraphic(bm2, 0x2100, blink);
		break;
	case 0xc0:	/* グラフィックページ1 */
		updateScreenGraphic(bm2, 0x900, blink);
		break;
	case 0xcc:	/* グラフィックページ2 */
		updateScreenGraphic(bm2, 0x2100, blink);
		break;
	}

	/* 前のフレームと変わった部分を更新する */
	r = rect;
	if(bm2->bckreg != oldBckReg || bm2->reverse != oldReverse) {
		oldBckReg = bm2->bckreg;
		oldReverse = bm2->reverse;
		r->x = 0;
		r->y = 0;
		r->w = 0;
		r->h = 0;
	} else {
		for(row = 0; row < ROWS; row++)
			for(col = 0; col < COLS; col++) {
				if(
				colorMap[row * COLS + col] == oldColorMap[row * COLS + col] &&
				vram[(row * CHR_HEIGHT + 0) * COLS + col] == oldVram[(row * CHR_HEIGHT + 0) * COLS + col] &&
				vram[(row * CHR_HEIGHT + 1) * COLS + col] == oldVram[(row * CHR_HEIGHT + 1) * COLS + col] &&
				vram[(row * CHR_HEIGHT + 2) * COLS + col] == oldVram[(row * CHR_HEIGHT + 2) * COLS + col] &&
				vram[(row * CHR_HEIGHT + 3) * COLS + col] == oldVram[(row * CHR_HEIGHT + 3) * COLS + col] &&
				vram[(row * CHR_HEIGHT + 4) * COLS + col] == oldVram[(row * CHR_HEIGHT + 4) * COLS + col] &&
				vram[(row * CHR_HEIGHT + 5) * COLS + col] == oldVram[(row * CHR_HEIGHT + 5) * COLS + col] &&
				vram[(row * CHR_HEIGHT + 6) * COLS + col] == oldVram[(row * CHR_HEIGHT + 6) * COLS + col] &&
				vram[(row * CHR_HEIGHT + 7) * COLS + col] == oldVram[(row * CHR_HEIGHT + 7) * COLS + col]
				)
					continue;
				r->x = (col * CHR_WIDTH + BORDER_WIDTH) * zoom;
				r->y = (row * CHR_HEIGHT + BORDER_HEIGHT) * zoom;
				r->w = CHR_WIDTH * zoom;
				r->h = CHR_HEIGHT * zoom;
				r++;
			}
	}
	if((int )(r - rect) == 0)
		return;
	memcpy(oldVram, vram, sizeof(oldVram));
	memcpy(oldColorMap, colorMap, sizeof(oldColorMap));
#if SDL_MAJOR_VERSION == 2
	SDL_UpdateWindowSurfaceRects(window, rect, (int )(r - rect));
#elif SDL_MAJOR_VERSION == 1
	SDL_UpdateRects(screen, (int )(r - rect), rect);
#endif
}

/*
	MB-6885仮想キーコードからSDLキーを得る (getAutoKeyEventの下請け)
*/
static int getSdlkey(const struct Bm2stat *bm2, int bmkey)
{
	int key;

	for(key = 0; key < sizeof(bm2->keyconv) / sizeof(bm2->keyconv[0]); key++)
		if(bm2->keyconv[key] == bmkey)
			return key;
	return 0;
}

/*
	自動入力キーを得る (updateKeyの下請け)
*/
static int getAutoKeyEvent(struct Bm2stat *bm2, SDL_Event *e)
{
	int press, bmkey;
	char ch;

	/* キーを得る */
	if(!getAutoKey(bm2, &press, &ch, &bmkey))
		return FALSE;

	/* SDLのイベントにする */
	memset(e, 0, sizeof(*e));
	e->type = (press ? SDL_KEYDOWN: SDL_KEYUP);
#if SDL_MAJOR_VERSION == 2
	e->key.keysym.scancode = getSdlkey(bm2, bmkey);
	e->key.keysym.sym = ch;
#elif SDL_MAJOR_VERSION == 1
	e->key.keysym.sym = getSdlkey(bm2, bmkey);
	e->key.keysym.unicode = ch;
#endif
	return TRUE;
}

/*
	キーのビットを得る (updateKeyの下請け)
*/
static int getKeyF(int bmkey)
{
	if(bmkey & 0xf00)
		return 0;
	return bmkey >> 4;
}

/*
	キーストローブを得る (updateKeyの下請け)
*/
static int getKeyE(int bmkey)
{
	if(bmkey & 0xf00)
		return -1;
	return bmkey & 0x00f;
}

/*
	キーを更新する
*/
int updateKey(struct Bm2stat *bm2)
{
	SDL_Event e;
	int bmkey;

	while(SDL_PollEvent(&e) || getAutoKeyEvent(bm2, &e)) {
		switch(e.type) {
		case SDL_KEYDOWN:
			if(e.key.keysym.mod & (KMOD_RALT | KMOD_LALT)) {
				if(e.key.keysym.sym == SDLK_f) {
					if(!bm2->menu)
						menu(bm2);
				} else if(e.key.keysym.sym == SDLK_p) {
#if SDL_MAJOR_VERSION == 2
					if(SDL_HasClipboardText())
						startAutoKey(bm2, SDL_GetClipboardText(), TRUE);
#endif
				} else if(e.key.keysym.sym == SDLK_RETURN) {
				}
				continue;
			}

#if 0
#if SDL_MAJOR_VERSION == 2
			if(e.key.keysym.scancode == SCANCODE_F12)
#elif SDL_MAJOR_VERSION == 1
			if(e.key.keysym.sym == SDLK_F12)
				bm2->cpu.trace = TRUE;
#endif
#endif

#if SDL_MAJOR_VERSION == 2
			bmkey = bm2->keyconv[e.key.keysym.scancode];
#elif SDL_MAJOR_VERSION == 1
			bmkey = bm2->keyconv[e.key.keysym.sym];
#endif
			if(bmkey == BMKEY_ALPHA)
				bm2->key_mod &= ~0x10;
			else if(bmkey == BMKEY_ASHIFT)
				bm2->key_mod &= ~0x20;
			else if(bmkey == BMKEY_KSHIFT)
				bm2->key_mod &= ~0x40;
			else if(bmkey == BMKEY_KANA)
				bm2->key_mod &= ~0x80;
			else if(bmkey == BMKEY_BREAK)
				bm2->key_break = TRUE;
			else if(bmkey == BMKEY_OVERWRITE)
				rewindTape(bm2);
			else if(bmkey == BMKEY_APPEND)
				foreTape(bm2);
			else if(getKeyE(bmkey) >= 0)
				bm2->key_mat[getKeyE(bmkey)] &= ~getKeyF(bmkey);

#if SDL_MAJOR_VERSION == 2
			if(e.key.keysym.sym == 0x08)
				return 0x7f;
			else if(e.key.keysym.sym == '\n')
				return '\n';
			else if(e.key.keysym.sym > 0xff)
				return 0;
			return e.key.keysym.sym;
#elif SDL_MAJOR_VERSION == 1
			if(e.key.keysym.unicode == 0x08)
				return 0x7f;
			else if(e.key.keysym.unicode == '\n')
				return '\r';
			return e.key.keysym.unicode;
#endif
		case SDL_KEYUP:
			if(e.key.keysym.mod & (KMOD_RALT | KMOD_LALT))
				continue;

#if SDL_MAJOR_VERSION == 2
			bmkey = bm2->keyconv[e.key.keysym.scancode];
#elif SDL_MAJOR_VERSION == 1
			bmkey = bm2->keyconv[e.key.keysym.sym];
#endif
			if(bmkey == BMKEY_ALPHA)
				bm2->key_mod |= 0x10;
			else if(bmkey == BMKEY_ASHIFT)
				bm2->key_mod |= 0x20;
			else if(bmkey == BMKEY_KSHIFT)
				bm2->key_mod |= 0x40;
			else if(bmkey == BMKEY_KANA)
				bm2->key_mod |= 0x80;
			else if(bmkey == BMKEY_BREAK)
				bm2->key_break = FALSE;
			else if(getKeyE(bmkey) >= 0)
				bm2->key_mat[getKeyE(bmkey)] |= getKeyF(bmkey);
			return 0;
#if SDL_MAJOR_VERSION == 2
		case SDL_MOUSEBUTTONUP:
			if(e.button.button == SDL_BUTTON_MIDDLE)
				if(SDL_HasClipboardText())
					startAutoKey(bm2, SDL_GetClipboardText(), TRUE);
			break;
		case SDL_DROPFILE:
			{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
				char file[PATH_MAX];
				convertUtf8ToSjis(file, sizeof(file), e.drop.file);
#else
				const char *file = e.drop.file;
#endif
				if(!readSRecord(file, bm2->memory, NULL, bm2->ram_end, FALSE))
					setTape(bm2, file);
				SDL_free(e.drop.file);
				SDL_RaiseWindow(window);
			}
			break;
		case SDL_WINDOWEVENT:
			if(e.window.event == SDL_WINDOWEVENT_EXPOSED)
				SDL_UpdateWindowSurface(window);
			break;
#endif
		case SDL_QUIT:
			exit(0);
		}
	}

	return 0;
}

/*
	待つ
*/
void delay(int interval)
{
	static Uint32 last = 0, left;
	Uint32 now;

	now = SDL_GetTicks();
	if (last + interval > now) {
		left = last + interval - now;
		SDL_Delay(left);
		last = now + left;
	} else
		last = now;
}

/*
	見出しを更新する
*/
void updateCaption(const struct Bm2stat *bm2)
{
	const char *file;
	char cap[256] = "BASIC MASTER Jr. ";

	if(bm2 != NULL) {
		for(file = bm2->tape_path + strlen(bm2->tape_path) - 1; file >= bm2->tape_path && *file != '/' && *file != '\\' && *file != ':'; file--)
			;
		file++;

		strcat(cap, "- ");
		strcat(cap, file);

		switch(bm2->tape_mode) {
		case TAPE_MODE_APPEND:
			break;
		case TAPE_MODE_OVERWRITE:
			strcat(cap, " [OW]");
			break;
		}
	}

#if SDL_MAJOR_VERSION == 2
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
	SDL_SetWindowTitle(window, convertSjisToUtf8(NULL, sizeof(cap), cap));
#else
	SDL_SetWindowTitle(window, cap);
#endif
#elif SDL_MAJOR_VERSION == 1
	SDL_WM_SetCaption(cap, NULL);
#endif
}

/*
	音を出力する (下請け)
*/
static void SDLCALL playSound(void *unused, Uint8 *stream, int len)
{
	uint8 *next_read_pointer;

#if SDL_MAJOR_VERSION == 2
	memset(stream, 0, len);
#endif
	SDL_MixAudio(stream, *soundReadPointer, len, SDL_MIX_MAXVOLUME);

	next_read_pointer = *soundReadPointer + len;
	if(next_read_pointer >= soundBuffer + soundBufferSize)
		next_read_pointer = soundBuffer;
	if(*soundWritePointer <= next_read_pointer && next_read_pointer < *soundWritePointer + len)
		return;

	*soundReadPointer = next_read_pointer;
}

/*
	エラーを表示する
*/
void popup(const char *str, ...)
{
	va_list v;
	char buf[256];

	va_start(v, str);
	vsprintf(buf, str, v);
	va_end(v);
#if SDL_MAJOR_VERSION == 2
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "bm2", buf, window);
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32)
	MessageBox(NULL, buf, "bm2", MB_OK | MB_ICONERROR);
#else
	fprintf(stderr, "%s", buf);
#endif
}

/*
	終了する (下請け)
*/
static SDLCALL void quitDepend(void)
{
	SDL_Quit();
}

/*
	画像の点の明るさを得る (下請け)
*/
static int point(SDL_Surface *surface, int x, int y)
{
	Uint32 pix;
	Uint8 r, g, b, *p = surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;

	SDL_LockSurface(surface);

	switch(surface->format->BytesPerPixel) {
	case 1:
		r = surface->format->palette->colors[*p].r;
		g = surface->format->palette->colors[*p].g;
		b = surface->format->palette->colors[*p].b;
		break;
	case 2:
		pix = *(Uint16 *)p;
		SDL_GetRGB(pix, surface->format, &r, &g, &b);
		break;
	case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		pix = ((Uint32 )p[0] << 16) | ((Uint32 )p[1] << 8) | (Uint32 )p[2];
#else
		pix = ((Uint32 )p[2] << 16) | ((Uint32 )p[1] << 8) | (Uint32 )p[0];
#endif
		SDL_GetRGB(pix, surface->format, &r, &g, &b);
		break;
	case 4:
		pix = *(Uint32 *)p;
		SDL_GetRGB(pix, surface->format, &r, &g, &b);
		break;
	default:
		r = g = b = 0;
		break;
	}

	SDL_UnlockSurface(surface);

	return (299 * (int )r + 587 * (int )g + 114 * (int )b) / 100;
}

/*
	フォントを読み込む
*/
int loadFontBmp(struct Bm2stat *bm2, const char *font_path)
{
	SDL_Surface *chr;
	int i, x, y, code = 0;

	if((chr = SDL_LoadBMP(font_path)) == NULL)
		return FALSE;

	for(x = 0; x < chr->w; x += 8)
		for(y = 0; y < chr->h; y += 8) {
			for(i = 0; i < 8; i++)
				bm2->rom_font[code][i] =
				(point(chr, x + 0, y + i) > 0x80 ? 0x80: 0) |
				(point(chr, x + 1, y + i) > 0x80 ? 0x40: 0) |
				(point(chr, x + 2, y + i) > 0x80 ? 0x20: 0) |
				(point(chr, x + 3, y + i) > 0x80 ? 0x10: 0) |
				(point(chr, x + 4, y + i) > 0x80 ? 0x08: 0) |
				(point(chr, x + 5, y + i) > 0x80 ? 0x04: 0) |
				(point(chr, x + 6, y + i) > 0x80 ? 0x02: 0) |
				(point(chr, x + 7, y + i) > 0x80 ? 0x01: 0);

			code++;
		}

	SDL_FreeSurface(chr);
	return TRUE;
}

/*
	環境依存部分を初期化する
*/
int initDepend(const struct Bm2stat *bm2, int argc, char *argv[])
{
	SDL_Surface *icon;
	Conf conf[256];
	int i, r, g, b;
	Uint32 _pix[8];
	Uint8 pix[4], *p, mask[32 * 32 / 8];

	/* 初期化済みか? */
#if SDL_MAJOR_VERSION == 2
	if(window != NULL)
		return TRUE;
#elif SDL_MAJOR_VERSION == 1
	if(video != NULL)
		return TRUE;
#endif
	/* 設定ファイルを読み込む */
	getConfig(conf, sizeof(conf) / sizeof(conf[0]), "bm2config", argc, argv);

	/* SDLを初期化する */
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | (bm2->use_sound ? SDL_INIT_AUDIO: 0))) {
		fprintf(stderr, "SDL_Init fail. %s\n", SDL_GetError());
		return FALSE;
	}
	atexit(quitDepend);

	/* ウィンドウを生成する */
#if SDL_MAJOR_VERSION == 2
	if((window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_HIDDEN)) == NULL) {
		popup("SDL_CreateWindow fail. %s\n", SDL_GetError());
		return FALSE;
	}
#elif SDL_MAJOR_VERSION == 1
	if((video = SDL_GetVideoInfo()) == NULL) {
		popup("SDL_GetVideoInfo fail. %s\n", SDL_GetError());
		return FALSE;
	}
#endif
	if((icon = SDL_CreateRGBSurfaceFromXpm(bm2icon_xpm, mask)) != NULL) {
#if SDL_MAJOR_VERSION == 2
		SDL_SetWindowIcon(window, icon);
#elif SDL_MAJOR_VERSION == 1
		SDL_WM_SetIcon(icon, mask);
#endif
	}

	zoom = bm2->zoom;
	fullLine = bm2->full_line;
#if SDL_MAJOR_VERSION == 2
	SDL_SetWindowSize(window, (BORDER_WIDTH + COLS * CHR_WIDTH + BORDER_WIDTH) * zoom, (BORDER_HEIGHT + ROWS * CHR_HEIGHT + BORDER_HEIGHT) * zoom);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(window);
	if((screen = SDL_GetWindowSurface(window)) == NULL) {
		popup("SDL_GetWindowSurface fail. %s", SDL_GetError());
		return FALSE;
	}
#elif SDL_MAJOR_VERSION == 1
	if((screen = SDL_SetVideoMode((BORDER_WIDTH + COLS * CHR_WIDTH + BORDER_WIDTH) * zoom, (BORDER_HEIGHT + ROWS * CHR_HEIGHT + BORDER_HEIGHT) * zoom, video->vfmt->BitsPerPixel, SDL_HWSURFACE)) == NULL) {
		popup("SDL_SetVideoMode fail. %s", SDL_GetError());
		return FALSE;
	}
#endif
	updateCaption(bm2);

	/* pixmapを得る */
	if(bm2->display == 0x000000) {
		_pix[0] = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
		_pix[1] = SDL_MapRGB(screen->format, 0x00, 0x00, 0xff);
		_pix[2] = SDL_MapRGB(screen->format, 0xff, 0x00, 0x00);
		_pix[3] = SDL_MapRGB(screen->format, 0xff, 0x00, 0xff);
		_pix[4] = SDL_MapRGB(screen->format, 0x00, 0xff, 0x00);
		_pix[5] = SDL_MapRGB(screen->format, 0x00, 0xff, 0xff);
		_pix[6] = SDL_MapRGB(screen->format, 0xff, 0xff, 0x00);
		_pix[7] = SDL_MapRGB(screen->format, 0xff, 0xff, 0xff);
	} else {
		r = (bm2->display >> 16) & 0xff;
		g = (bm2->display >> 8) & 0xff;
		b = bm2->display & 0xff;
		_pix[0] = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
		_pix[1] = SDL_MapRGB(screen->format, r * 0x8e / 0xff, g * 0x8e / 0xff, b * 0x8e / 0xff);
		_pix[2] = SDL_MapRGB(screen->format, r * 0xa7 / 0xff, g * 0xa7 / 0xff, b * 0xa7 / 0xff);
		_pix[3] = SDL_MapRGB(screen->format, r * 0xb4 / 0xff, g * 0xb4 / 0xff, b * 0xb4 / 0xff);
		_pix[4] = SDL_MapRGB(screen->format, r * 0xcb / 0xff, g * 0xcb / 0xff, b * 0xcb / 0xff);
		_pix[5] = SDL_MapRGB(screen->format, r * 0xd9 / 0xff, g * 0xd9 / 0xff, b * 0xd9 / 0xff);
		_pix[6] = SDL_MapRGB(screen->format, r * 0xf2 / 0xff, g * 0xf2 / 0xff, b * 0xf2 / 0xff);
		_pix[7] = SDL_MapRGB(screen->format, r, g, b);
	}
	for(i = 0; i < 8; i++) {
		switch(screen->format->BytesPerPixel) {
		case 1:
			*pix = (Uint8 )_pix[i];
			break;
		case 2:
			*(Uint16 *)pix = (Uint16 )_pix[i];
			break;
		case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			*(pix + 0) = _pix[i] >> 16 & 0xff;
			*(pix + 1) = _pix[i] >> 8 & 0xff;
			*(pix + 2) = _pix[i] & 0xff;
#else
			*(pix + 0) = _pix[i] & 0xff;
			*(pix + 1) = _pix[i] >> 8 & 0xff;
			*(pix + 2) = _pix[i] >> 16 & 0xff;
#endif
			break;
		case 4:
			*(Uint32 *)pix = _pix[i];
			break;
		}
		for(
		p = pixMap[i];
		p < pixMap[i] + zoom * screen->format->BytesPerPixel;
		p += screen->format->BytesPerPixel
		)
			memcpy(p, pix, screen->format->BytesPerPixel);
	}

	/* モノクロ時のカラーマップを初期化する */
	memset(colorMapMono, 0x07, sizeof(colorMapMono));

	/* 音声を初期化する */
	if(bm2->use_sound) {
		audio.freq = 44100;
		audio.format = AUDIO_S8;
		audio.channels = 1;
		audio.samples = bm2->sound_sample_size;
		audio.callback = playSound;
		audio.userdata = (void *)bm2;
		soundBuffer = bm2->sound_buffer;
		soundBufferSize = bm2->sound_buffer_size;
		soundReadPointer = (uint8 **)&bm2->sound_read_pointer;
		soundWritePointer = (uint8 **)&bm2->sound_write_pointer;
		if(SDL_OpenAudio(&audio, NULL) < 0)
			popup("SDL_OpenAudio fail.\n");
		SDL_PauseAudio(0);
	}

#if SDL_MAJOR_VERSION == 1
	SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_EnableUNICODE(1);
#endif

#if SDL_MAJOR_VERSION == 2
	SDL_UpdateWindowSurface(window);
#endif

	return TRUE;
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
