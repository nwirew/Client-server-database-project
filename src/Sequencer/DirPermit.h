#pragma once

#include "Parseable.h"

namespace std {
	class DirPermit : public Parseable {
	private:
		void subReset();
	public:
		string lp, rp;
		TKID dir;

		DirPermit();
		DirPermit(int&, int&);
		DirPermit(string&, TKID&&, string&);
	};
	
	bool operator==(DirPermit const&, DirPermit const&);
	
	template<>
	struct hash<DirPermit> {
		size_t operator()(DirPermit const&) const noexcept;
	};
}