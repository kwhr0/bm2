/*
	�ݒ�t�@�C������
*/

#if !defined(CONF_H)
#define CONF_H

#include <stdio.h>

/*
	�\����
*/
/* ������<->���l�ϊ��e�[�u�� */
typedef struct {
	const char *string;
	int value;
} OptTable;

/* config */
typedef struct {
	int line;
	char key[32];
	char value[128];
} Conf;

/*
	�֐��v���g�^�C�v
*/
Conf *getConfig(Conf *, int, const char *, int , char *[]);
const char *getOptText(const Conf *, const char *, const char *);
int getOptInt(const Conf *, const char *, int );
unsigned int getOptHex(const Conf *, const char *, unsigned int );
int getOptTable(const Conf *, const char *, const OptTable *, int );
char *setHomeDir(char *, const char *);

#endif

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
