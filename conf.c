/*
	�ݒ�t�@�C������(conf.c)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include "conf.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#	include "windows.h"
#endif

#define COMMENT	'#'

/*
	���s�t�@�C���̃f�B���N�g���𓾂� (win32��p)
*/
static char *getexedir(void)
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	static char buf[PATH_MAX];
	char *p;

	GetModuleFileName(NULL, buf, sizeof(buf));
	for(p = buf + strlen(buf); p > buf && *p != '\\'; p--)
		;
	*p = 0;

	return buf;
#else
	return "";
#endif
}

/*
	���s�t�@�C���̈�������I�v�V�����𓾂�
*/
static int readArg(Conf *conf, char *argv)
{
	int size;
	char *p, *q;

	if(*argv != '-')
		return 0;

	for(p = argv; *p != '=' && *p != '\0'; p++)
		;
	if(*p == '\0')
		return 0;

	for(q = p; *q != '\0'; q++)
		;
	
	size = (int )(p - argv - 1);
	memcpy(conf->key, argv + 1, size);
	*(conf->key + size) = '\0';

	size = (int )(q - p + 1);
	memcpy(conf->value, p + 1, size);
	*(conf->value + size) = '\0';

	*(conf + 1)->key = '\0';
	return 1;
}

/*
	Config�t�@�C�����I�[�v������
*/
static FILE *openConfig(const char *file)
{
	const static char *hide[] = { ".", "" };
	FILE *fp;
	char path[FILENAME_MAX];
	const char *dir[5], **p, **q;

	dir[0] = ".";
	dir[1] = getenv("HOME");
	dir[2] = getenv("USERPROFILE");
	dir[3] = getenv("ALLUSERSPROFILE");
	dir[4] = getexedir();

	for(p = hide; p != hide + sizeof(hide) / sizeof(hide[0]); p++)
		for(q = dir; q != dir + sizeof(dir) / sizeof(dir[0]); q++) {
			if(*q == NULL)
				continue;
			sprintf(path, "%s/%s%s", *q, *p, file);
			if((fp = fopen(path, "r")) != NULL)
				return fp;
		}
	return NULL;
}

/*
	�t�@�C������I�v�V�����𓾂�
*/
static int readConfig(FILE *fp, Conf *conf)
{
	char buf[160], *p, *q;

	/* ���ӂ𓾂� */
	fgets(buf, sizeof(buf), fp);
	for(p = buf; *p == ' ' || *p == '\t'; p++)
		;
	if(*p == COMMENT || *p == '\r' || *p == '\n' || *p == 0)
		return 0;
	for(q = p; *q != ' ' && *q != '\t' && *q != '\r' && *q != '\n' && *q != 0; q++)
		;
	memcpy(conf->key, p, (int )(q - p));
	*(conf->key + (int )(q - p)) = '\0';
	
	/* �E�ӂ𓾂� */
	for(p = q; *p == ' ' || *p == '\t'; p++)
		;
	for(q = p; *q != '\r' && *q != '\n' && *q != 0 && *q != COMMENT; q++)
		;
	if(p < q)
		for(; *(q - 1) == ' ' || *(q - 1) == '\t'; q--)
			;
	memcpy(conf->value, p, (int )(q - p));
	*(conf->value + (int )(q - p)) = '\0';

	*(conf + 1)->key = '\0';
	return 1;
}

/*
	Config�t�@�C����ǂݍ��� 
*/
Conf *getConfig(Conf *conf, int length, const char *file, int argc, char *argv[])
{
	Conf *p = conf, *last = conf + length - 1;
	FILE *fp;
	int i, line;

	strcpy(p->key, "");
	
	/* ���s�t�@�C���̈�������I�v�V�����𓾂� */
	line = INT_MIN;
	for(i = 1; i < argc && p < last; i++) {
		if(readArg(p, argv[i]))
			(p++)->line = line;
		line++;
	}

	/* �t�@�C������I�v�V�����𓾂� */
	if((fp = openConfig(file)) != NULL) {
		line = 1;
		while(!feof(fp) && p < last) {
			if(readConfig(fp, p))
				(p++)->line = line;
			line++;
		}
		fclose(fp);
	}

	return (p != conf ? conf: NULL);
}

/*
	Config�t�@�C�����當��������o��
*/
const char *getOptText(const Conf *conf, const char *key, const char *default_value)
{
	const Conf *p;

	for(p = conf; p->key[0] != '\0'; p++)
		if(strcasecmp(p->key, key) == 0)
			return p->value;
	return default_value;
}

/*
	Config�t�@�C�����琔�l�����o��(10�i��)
*/
int getOptInt(const Conf *conf, const char *key, int default_value)
{
	const char *p;

	p = getOptText(conf, key, "");
	if(strcmp(p, "") != 0)
		return atoi(p);
	else
		return default_value;
}

/*
	Config�t�@�C�����琔�l�����o��(16�i��)
*/
unsigned int getOptHex(const Conf *conf, const char *key, unsigned int default_value)
{
	int x;
	const char *p;

	p = getOptText(conf, key, "");
	if(strcmp(p, "") != 0) {
		sscanf(p, "%x", &x);
		return x;
	} else
		return default_value;
}

/*
	Config�t�@�C�����當��������o��, �e�[�u���𗘗p���Đ��l�ɕϊ�����
*/
static int textToInt(const OptTable *table, const char *str, int default_value)
{
	const OptTable *p;

	for(p = table; p->string != NULL; p++)
		if(strcasecmp(str, p->string) == 0)
			return p->value;
	return default_value;
}
int getOptTable(const Conf *conf, const char *key, const OptTable *table, int default_value)
{
	return textToInt(table, getOptText(conf, key, ""), default_value);
}

/*
	~���z�[���f�B���N�g���ɒu��������
*/
char *setHomeDir(char *buf, const char *path)
{
	if(*path != '~') {
		strcpy(buf, path);
		return buf;
	}

	if(getenv("HOME") != NULL)
		sprintf(buf, "%s%s", getenv("HOME"), path + 1);
	else if(getenv("USERPROFILE") != NULL)
		sprintf(buf, "%s%s", getenv("USERPROFILE"), path + 1);
	else if(getexedir() != NULL)
		sprintf(buf, "%s%s", getexedir(), path + 1);
	else
		sprintf(buf, "%s%s", ".", path + 1);
	return buf;
}

/*
	Copyright 2005 ~ 2008 maruhiro
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
