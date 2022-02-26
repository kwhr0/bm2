#include <deque>

struct CompareProcess {
	CompareProcess() : compareMode(false) {}
	virtual void Stop() {}
	void setCompare(bool f) {
		compareMode = f;
		if (!compareMode) { rlog.clear(); wlog.clear(); }
	}
	bool WriteStart(uint32_t adr, uint32_t data) {
		if (!compareMode)
			wlog.emplace_back(adr, data);
		else {
			if (wlog.empty()) {
				fprintf(stderr, "wlog is empty.\n");
				Stop();
			}
			Acs acs = wlog.front();
			wlog.pop_front();
			if (adr != acs.adr) {
				fprintf(stderr, "write address error: cur=%x new=%x\n", acs.adr, adr);
				Stop();
			}
			if ((data & 0xff) != acs.data) {
				fprintf(stderr, "write data error: cur=%x new=%x\n", acs.data, data & 0xff);
				Stop();
			}
			return true;
		}
		return false;
	}
	bool ReadStart(uint32_t adr, uint32_t &data) {
		if (compareMode) {
			if (rlog.empty()) {
				fprintf(stderr, "rlog is empty.\n");
				Stop();
			}
			Acs acs = rlog.front();
			rlog.pop_front();
			if (adr != acs.adr) {
				fprintf(stderr, "read address error: cur=%x new=%x\n", acs.adr, adr);
				Stop();
			}
			data = acs.data;
			return true;
		}
		orgadr = adr;
		return false;
	}
	void ReadEnd(uint32_t data) {
		if (!compareMode)
			rlog.emplace_back(orgadr, data);
	}
	struct Acs {
		Acs(uint16_t a, uint8_t d) : adr(a), data(d) {}
		uint16_t adr;
		uint8_t data;
	};
	std::deque<Acs> rlog, wlog;
	uint32_t orgadr;
	bool compareMode;
};
