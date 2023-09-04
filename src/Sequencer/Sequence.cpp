#include <iostream>
#include "Sequence.h"

namespace std {
	/* --------------------------------------------------------------------- *\
	 * --- SEQUENCE -------------------------------------------------------- *
	\* --------------------------------------------------------------------- */
	
	
	
	// --- Constructors --- //
	
	Sequence::Sequence() : subs{}, Parseable(SEQUENCE, 0, 0) {}
	
	
	
	// --- Private --- //
	
	void Sequence::subReset() {
		subs.clear();
	}
}