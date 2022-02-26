/*
	日立ベーシックマスターJr.エミュレータ
	メニュー
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include "bm2.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

#include <windows.h>

/*
	ファイル名の入力を得る(Windows)
*/
int inputFileName(struct Bm2stat *bm2, char *path, unsigned int filter)
{
	const char *text_table[][3] = {
		{ "Tape file (*.bin)", "Binary file (*.B)", "All files (*.*)" },
		{ "テープイメージ(*.bin)", "バイナリファイル(*.B)", "全てのファイル(*.*)" }
	};
	const char *ext[] = { "*.bin", "*.B", "*.*" };
	const char **text;
	OPENFILENAME o;
	LCID lcid;
	char filter_text[256] = "", *p = filter_text;

	lcid = GetUserDefaultLCID();
	if(lcid == 1041) /* 日本語 */
		text = text_table[1];
	else /* その他(英語) */
		text = text_table[0];

	if(filter & FILTER_TAPE) {
		strcpy(p, text[0]); p += strlen(text[0]) + 1;
		strcpy(p, ext[0]); p += strlen(ext[0]) + 1;
	}
	if(filter & FILTER_BINARY) {
		strcpy(p, text[1]); p += strlen(text[1]) + 1;
		strcpy(p, ext[1]); p += strlen(ext[1]) + 1;
	}
	strcpy(p, text[2]); p += strlen(text[2]) + 1;
	strcpy(p, ext[2]); p += strlen(ext[2]) + 1;
	*p = 0;

	path[0] = 0;

	memset(&o, 0, sizeof(o));
	o.lStructSize = sizeof(o);
	o.lpstrFilter = filter_text;
	o.lpstrFile   = path;
	o.nMaxFile    = PATH_MAX;
	o.Flags       = OFN_HIDEREADONLY;
	return GetOpenFileName(&o);
}

#else

#include <dirent.h>
#define MAX(a, b)	((a) > (b) ? (a): (b))

/*
	ディレクトリを得る(下請け)
*/
static char *getDir(char *dir, const char *path)
{
	char *p;

	strcpy(dir, path);
	for(p = dir + strlen(dir) - 1; p >= dir && *p != '/'; p--)
		*p = 0;

	return dir;
}

/*
	ファイル名を得る(下請け)
*/
static char *getFile(char *file, const char *path)
{
	const char *p;

	for(p = path + strlen(path) - 1; p >= path && *p != '/'; p--)
		;
	if(p < path)
		strcpy(file, path);
	else
		strcpy(file, p + 1);
	return file;
}

/*
	ファイルの種類を得る(下請け)
*/
static int getFileType(const char *path)
{
	int fd;
	char buf[16];

	if(strlen(path) >= 4 && memcmp(path + strlen(path) - 4, ".bin", 4) == 0)
		return FILTER_TAPE;

	if((fd = open(path, O_RDONLY)) < 0)
		return 0;
	read(fd, buf, 12);
	close(fd);

	if(memcmp(buf, "RIFF", 4) == 0 && memcmp(buf + 8, "WAVE", 4) == 0)
		return FILTER_SOUND;
	if(buf[0] == 'S')
		return FILTER_BINARY;
	return FILTER_TEXT;
}

/*
	ファイル名を比較する(下請け)
*/
static int cmpfile(const void *a, const void *b)
{
	const char *s1 = (const char *)a;
	const char *s2 = (const char *)b;

	return strcasecmp(s1, s2);
}

/*
	ファイル名の一覧を表示する (下請け)
*/
static int printFileList(struct Bm2stat *bm2, const char *base_name, int filter)
{
	static char file[256][17 + 1];
	DIR *dir;
	struct dirent *p;
	int n = 0, i;
	char dir_name[PATH_MAX], base_file_name[PATH_MAX], path[PATH_MAX];

	memset(file, 0, sizeof(file));

	print(bm2, "\x0c");

	getDir(dir_name, base_name);
	if(strcmp(dir_name, "") == 0)
		strcpy(dir_name, "./");
	getFile(base_file_name, base_name);

	if((dir = opendir(dir_name)) == NULL)
		return -1;
	for(p = readdir(dir); p != NULL && n < 256; p = readdir(dir)) {
		sprintf(path, "%s%s", dir_name, p->d_name);
		if((p->d_name[0] == '.' && strcmp(p->d_name, "..") != 0))
			continue;
		if(strlen(base_file_name) > 0 && memcmp(p->d_name, base_file_name, strlen(base_file_name)) != 0)
			continue;
		if(p->d_type == DT_DIR)
			sprintf(file[n++], "%-.16s/", p->d_name);
		else if(filter & getFileType(path))
			sprintf(file[n++], "%-.16s", p->d_name);
	}
	closedir(dir);

	qsort(file, n, sizeof(file[0]), cmpfile);
	for(i = 0; i <22 * 2; i++) {
		locate(bm2, (i / 22) * 16, 3 + (i % 22));
		print(bm2, "%-16s", file[i]);
	}
	return n;
}

/*
	ファイル名の入力を得る(UNIX)
*/
int inputFileName(struct Bm2stat *bm2orig, char *path, unsigned int filter)
{
	struct Bm2stat _bm2 = *bm2orig, *bm2 = &_bm2;
	char c, dir[PATH_MAX], file[PATH_MAX];

	bm2->cpu = m68init(bm2->memory, bm2);
	bm2->screen_mode = 0;
	bm2->mp1710 = FALSE;

	getDir(dir, path);
	strcpy(path, dir);

	for(;;) {
		printFileList(bm2, path, filter);

		locate(bm2, 0, 0);
		print(bm2, "FILE NAME : ");
		switch((c = getkeystr(bm2, path))) {
		case 0:
			break;
		case '\r':
			getFile(file, path);
			return strcmp(file, "") != 0;
		case '\t':
			break;
		}
	}
}

#endif

/*
	テープファイルを設定する
*/
void menu(struct Bm2stat *bm2)
{
	char path[PATH_MAX];

	if(bm2->cpu.emulate_subroutine)
		return;

	bm2->menu = TRUE;
	
	strcpy(path, bm2->tape_path);
	if(inputFileName(bm2, path,  FILTER_TAPE | FILTER_BINARY)) {
		if(!readSRecord(path, bm2->memory, NULL, bm2->ram_end, FALSE))
			setTape(bm2, path);
	}
	
	bm2->menu = FALSE;
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
