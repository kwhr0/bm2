/*
	XPMからSDL_Surfaceを生成する (sdlxpm.c)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"
#include "sdlxpm.h"

#define FALSE	0
#define TRUE	1

/*
	16進数の文字列を数値にする(下請け)
*/
static unsigned int atoix(const char *txt)
{
	unsigned int x;

	sscanf(txt, "%x", &x);
	return x;
}

/*
	Surfaceに点を描く(SDL_CreateRGBSurfaceAndMaskFromXpmの下請け)
*/
static void pset(Uint8 *dst, Uint32 pix, int bpp)
{
	switch(bpp) {
	case 1:
		*dst = (Uint8 )pix;
		break;
	case 2:
		*(Uint16 *)dst = (Uint16 )pix;
		break;
	case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		*(dst + 0) = pix >> 16 & 0xff;
		*(dst + 1) = pix >>  8 & 0xff;
		*(dst + 2) = pix       & 0xff;
#else
		*(dst + 0) = pix       & 0xff;
		*(dst + 1) = pix >>  8 & 0xff;
		*(dst + 2) = pix >> 16 & 0xff;
#endif
		break;
	case 4:
		*(Uint32 *)dst = pix;
		break;
	}
}

/*
	ある色がテーブルに存在するか調べる(get_keycolorの下請け)
*/
static int exist_color(const SDL_Color color[], int colors, SDL_Color key_color)
{
	const SDL_Color *p;

	for(p = color; p != color + colors; p++)
		if(p->r == key_color.r && p->g == key_color.g && p->b == key_color.b)
			return TRUE;
	return FALSE;
}

/*
	透明色を求める(SDL_CreateRGBSurfaceAndMaskFromXpmの下請け)
*/
static SDL_Color get_keycolor(SDL_Surface *s, const SDL_Color color[], int colors)
{
	SDL_Color key_color = { 0, 0xff, 0 };
	int i;

	if(!exist_color(color, colors, key_color))
		return key_color;

	key_color.b = key_color.r = 0xff;
	for(i = 0; i != 0x100; i++) {
		key_color.g = i;
		if(!exist_color(color, colors, key_color))
			return key_color;
	}
	return key_color;
}

/*
	XPMからSDL_Surfaceとマスクを生成する
*/
SDL_Surface *SDL_CreateRGBSurfaceFromXpm(char *xpm[], Uint8 *mask)
{
#if SDL_MAJOR_VERSION == 1
	const SDL_VideoInfo *v = SDL_GetVideoInfo();
#endif
	SDL_Color color[0x100];
	SDL_Surface *s;
	Uint32 pix;
	Uint8 *p_pix, *q_pix, *p_mask;
	int i, width, height, colors, bytes, key = 0;
	char symbol[4], c[4], rgb[8], *p_xpm, **pp_xpm = xpm;

	/* 幅, 高さを設定する */
	if(sscanf(*pp_xpm++, "%d %d %d %d", &width, &height, &colors, &bytes) != 4)
		return NULL;
	if(bytes != 1)
		return NULL;
#if SDL_MAJOR_VERSION == 2
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	if((s = SDL_CreateRGBSurface(0, width, height, 24, 0xff0000, 0x00ff00, 0x0000ff, 0)) == NULL)
		return NULL;
#else
	if((s = SDL_CreateRGBSurface(0, width, height, 24, 0x0000ff, 0x00ff00, 0xff0000, 0)) == NULL)
		return NULL;
#endif
#elif SDL_MAJOR_VERSION == 1
	if((s = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, v->vfmt->BitsPerPixel, v->vfmt->Rmask, v->vfmt->Gmask, v->vfmt->Bmask, 0)) == NULL)
		return NULL;
#endif
	memset(color, 0, sizeof(color));
	if(mask != NULL)
		memset(mask, 0, (width + 7) / 8 * height);

	/* 色を設定する */
	for(i = 0; i != colors; i++) {
		if(**pp_xpm != ' ')
			sscanf(*pp_xpm++, "%3s %3s %7s", symbol, c, rgb);
		else {
			strcpy(symbol, " ");
			sscanf(*pp_xpm++, "%3s %7s", c, rgb);
		}
		if(memcmp(rgb, "None", 4) != 0) {
			pix = atoix(&rgb[1]);
			color[(int )*symbol].r = pix >> 16 & 0xff;
			color[(int )*symbol].g = pix >>  8 & 0xff;
			color[(int )*symbol].b = pix       & 0xff;
		} else
			key = (int )*symbol;
	}
	color[key] = get_keycolor(s, color, colors);
	if(s->format->BytesPerPixel == 1) {
#if SDL_MAJOR_VERSION == 2
		SDL_SetPaletteColors(s->format->palette, color, 0, colors);
#elif SDL_MAJOR_VERSION == 1
		SDL_SetPalette(s, SDL_LOGPAL, color, 0, colors);
#endif
	}
#if SDL_MAJOR_VERSION == 2
	SDL_SetColorKey(s, SDL_TRUE, SDL_MapRGB(s->format, color[key].r, color[key].g, color[key].b));
#elif SDL_MAJOR_VERSION == 1
	SDL_SetColorKey(s, SDL_SRCCOLORKEY, SDL_MapRGB(s->format, color[key].r, color[key].g, color[key].b));
#endif

	/* Pixelを描き込む */
	if(SDL_MUSTLOCK(s))
		SDL_LockSurface(s);
	for(i = 0, q_pix = s->pixels, p_mask = mask; i != height; i++, q_pix += s->pitch, p_mask += (width + 7) / 8, pp_xpm++) {
		for(p_pix = q_pix, p_xpm = *pp_xpm; p_pix != q_pix + s->pitch; p_pix += s->format->BytesPerPixel, p_xpm++) {
			pix = SDL_MapRGB(s->format, color[(int )*p_xpm].r, color[(int )*p_xpm].g, color[(int )*p_xpm].b);
			pset(p_pix, pix, s->format->BytesPerPixel);
			if(mask != NULL && (int )*p_xpm != key)
				*(p_mask + (int )(p_xpm - *pp_xpm) / 8) |= 0x80 >> ((int )(p_xpm - *pp_xpm) % 8);
		}
	}
	if(SDL_MUSTLOCK(s))
		SDL_UnlockSurface(s);

	return s;
}

/*
	Copyright 2006 ~ 2013 maruhiro
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
