#include "DirPermit.h"

namespace std {
	/* --------------------------------------------------------------------- *\
	 * --- DIRECTION PERMIT ------------------------------------------------ *
	\* --------------------------------------------------------------------- */
	
	
	
	// --- Constructors --- //
	
	DirPermit::DirPermit()
	:	lp(""), rp(""), dir(UNKNOWN), Parseable(DIRPERMIT, 0, 0) {}
	
	DirPermit::DirPermit(int& l, int& c)
	:	lp(""), rp(""), dir(UNKNOWN), Parseable(DIRPERMIT, l, c) {}
	
	DirPermit::DirPermit(string& l, TKID&& d, string& r)
	:	lp( l), rp( r), dir(   d   ), Parseable(DIRPERMIT, 0, 0) {}
	
	
	
	// --- Private --- //
	
	void DirPermit::subReset() {
		lp.clear();
		dir = UNKNOWN;
		rp.clear();
	}
	
	
	
	/* --------------------------------------------------------------------- *\
	 * --- EQUALITY OPERATOR ----------------------------------------------- *
	\* --------------------------------------------------------------------- */
	
	bool operator==(DirPermit const& l, DirPermit const& r) {
		return
			(l.lp == r.lp && l.rp == r.rp && ( // Same names in same places
				l.dir == r.dir ||              // and either same arrows
				l.lp  == l.rp                  // or same party on both ends
			))
			||
			(l.lp == r.rp && l.rp == r.lp && (          // Mirrored names and
				(l.dir == LARROW && r.dir == RARROW) || // mirrored arrows
				(l.dir == RARROW && r.dir == LARROW) ||
				(l.dir == MARROW && r.dir == MARROW)
			));
	}
	
	
	
	/* --------------------------------------------------------------------- *\
	 * --- HASH FUNCTOR (NEEDED BY UNORDERED_SET) -------------------------- *
	\* --------------------------------------------------------------------- */
	
	// unordered_set needs this to decide an element's bucket and location
	size_t hash<DirPermit>::operator()(DirPermit const& dp) const noexcept {
		// If source and destination are the same, then DirPermits are
		// identical regardless of dir type
		if(dp.lp == dp.rp)
			return hash<string>{}(dp.lp + (char)MARROW + dp.rp);
		
		// Otherwise rotate so party names are sorted left to right
		string
			sortFirst  = min(dp.lp, dp.rp),
			sortSecond = max(dp.lp, dp.rp);
		char
			sortDir =
				sortFirst == dp.lp ? dp.dir :
				dp.dir == LARROW ? RARROW :
				dp.dir == RARROW ? LARROW :
				MARROW;
		
		return hash<string>{}(sortFirst + sortDir + sortSecond);
	}
}