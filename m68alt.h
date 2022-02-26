#include "test.h"

#ifndef CURMPU
struct M68stat {
	uint8 *m;		/* メモリ */
	int emulate_subroutine;	/* サブルーチンをエミュレートするか? */
	void *tag;		/* その他の情報 */
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct M68stat m68init(void *memory, void *tag);
int m68reset(struct M68stat *m68);

#ifdef __cplusplus
}
#endif
