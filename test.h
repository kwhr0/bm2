#ifndef _TEST_H_
#define _TEST_H_

//#define CURMPU
#define NEWMPU

typedef unsigned char	uint8;
typedef char	int8;
typedef unsigned short	uint16;
typedef short	int16;
typedef unsigned int	uint32;
typedef int	int32;

struct M68stat;
extern struct M68stat *g68; // for m[] and tag

#ifdef __cplusplus
extern "C" {
#endif

void setCompare(int f);
void signal_snd(void);
uint8 m68read8(const struct M68stat *, uint16);
uint16 m68read16(const struct M68stat *, uint16);
void m68write8(struct M68stat *, uint16, uint8);
void m68write16(struct M68stat *, uint16, uint16);

#ifdef __cplusplus
}
#endif

#define read8(a)	m68read8(g68, a)
#define write8(a, d)	m68write8(g68, a, d)

#endif
