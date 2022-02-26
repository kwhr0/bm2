#include "m68alt.h"
#include "test.h"

#ifndef CURMPU

/*
	M6800エミュレータを初期化する
*/
struct M68stat m68init(void *memory, void *tag)
{
	struct M68stat m68;

	m68.m = memory;
	m68.tag = tag;
	return m68;
}

/*
	リセット信号を送る
*/
int m68reset(struct M68stat *m68)
{
	return 1;
}

#endif // CURMPU
