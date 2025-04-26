/*
	日立ベーシックマスターJr.エミュレータ
	メイン
*/

#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include "bm2.h"
#include "HD6303.h"

struct bin_sem {
	bin_sem() : f(false) {}
	void wait() {
		std::unique_lock<std::mutex> lock(m);
		cv.wait(lock, [=] { return f; });
		f = false;
	}
	void signal() {
		std::lock_guard<std::mutex> lock(m);
		if (!f) {
			f = true;
			cv.notify_one();
		}
	}
	bool f;
	std::mutex m;
	std::condition_variable cv;
};

bin_sem gSem;
HD6303 hd6303;
M68stat *g68;
int gStates, gTotalStates;
bool gNMI;

void signal_snd() {
	gSem.signal();
}

/*
	累積ステート数を得る
*/
int m68states(const struct M68stat *m68)
{
	return gTotalStates - gStates;
}

void reset(struct Bm2stat *bm2) {
	m68reset(&bm2->cpu);
#ifdef NEWMPU
	g68 = &bm2->cpu;
	hd6303.Reset();
#endif
}

int main(int argc, char *argv[])
{
	struct Bm2stat *bm2 = (Bm2stat *)malloc(sizeof(struct Bm2stat));
	int timer = 0;

	/* 初期化する */
	if(!init(bm2, argc, argv))
		return 1;

	reset(bm2);
	bool irq = false, nmi = false;
	for(;;) {
		/* コードを実行する */
		gTotalStates = (gTotalStates & 0x7fffffff) + gStates;
#ifdef CURMPU
		if (bm2->cpu.wai) gStates = 0;
#endif
		while (gStates > 0) {
#ifdef CURMPU
			bool irq_cur = false;
#ifdef NEWMPU
			setCompare(false);
#endif
			if (nmi) m68nmi(&bm2->cpu);
			else if (irq) irq_cur = m68irq(&bm2->cpu);
			m68exec(&bm2->cpu);
#endif
#ifdef NEWMPU
			if (nmi) hd6303.NMI();
			else if (irq) hd6303.IRQ();
#ifdef CURMPU
			setCompare(true);
			hd6303.Execute(0);
#else
			if (bm2->sound_tape) // 6303でテープを読むための調整
				switch (g68->m[hd6303.GetPC()]) {
					case 0x01: case 0x20: case 0x22: case 0x23: case 0x24: case 0x26: case 0x2a:
						gStates--;
						break;
					case 0x7d:
						gStates -= 2;
						break;
					case 0x8d:
						gStates -= 3;
						break;
				}
			gStates -= hd6303.Execute(0);
#endif
#endif
#ifdef CURMPU
			if (irq_cur) irq = false;
#else
			irq = false;
#endif
			nmi = gNMI;
			gNMI = false;
#ifdef NEWMPU
			if (hd6303.isWaiting()) gStates = 0;
#endif
		}
		gTotalStates -= gStates;
		/* 画面を更新する */
		updateScreen(bm2);

		/* キーを更新する */
		updateKey(bm2);

		/* BREAKキーによるNMI
		if(bm2->key_break && (bm2->key_strobe & 0x80)) {
			bm2->key_break = FALSE;
			bm2->fast = FALSE;
			bm2->sound_tape = FALSE;
			m68nmi(&bm2->cpu);
		}*/

		/* IRQ(タイマ) */
		if(timer >= 1000 / 60) {
			if(!(bm2->ram_rom & 0x10)) {
				irq = true;//m68irq(&bm2->cpu);
			}
			timer -= 1000 / 60;
		}

		/* リセット */
		if(bm2->key_break && !(bm2->key_mod & 0x40))
			m68reset(&bm2->cpu);

	//	do {
			timer += 1000 / bm2->io_freq;
			gStates += bm2->cpu_freq / bm2->io_freq;

			/* 待つ */
			if (bm2->use_sound && !bm2->fast) gSem.wait();
			else delay(bm2->fast ? 0 : 1000 / bm2->io_freq);

			/* サウンドバッファを入れ替える */
			if(bm2->use_sound)
				flipSoundBuffer(bm2);
	//	} while(gStates < 0);
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
