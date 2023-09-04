#include "Message.h"

namespace std {
	/* --------------------------------------------------------------------- *\
	 * --- MESSAGE --------------------------------------------------------- *
	\* --------------------------------------------------------------------- */
	
	
	
	// --- Constructors --- ///
	
	Message::Message()
	  : attempts{1}, tryRate{0}, perYield{false}, finYield{false},
	    srcFirst{false}, dstFirst{false}, srcLast{false}, dstLast{false},
	    finYieldTarget{""}, src{""}, dst{""}, dat{""},
		Parseable(MESSAGE, 0, 0) {}
	
	Message::Message(Message& m)
	  : attempts{m.attempts}, tryRate{m.tryRate},
	    perYield{m.perYield}, finYield{m.finYield},
	    srcFirst{m.srcFirst}, dstFirst{m.dstFirst},
		srcLast{m.srcLast}, dstLast{m.dstLast},
		finYieldTarget{m.finYieldTarget}, src{m.src}, dst{m.dst}, dat{m.dat},
		Parseable(m.ID, m.lineIdx, m.charIdx) {}
	
	
	
	// --- Private --- //
	
	void Message::subReset() {
		src.clear();
		dst.clear();
		dat.clear();
	}
	
	
	
	// --- Public --- //
	
	void Message::set(string& l, TKID& d, string& r, string& c) {
		if(d == MARROW) printError(*this, MESSAGEBADDIR);
		else
		if(d == RARROW) src = l, dst = r;
		else            src = r, dst = l;
		dat = c;
	}
	
	bool Message::isParticipant(string& name) {
		return name == src || name == dst;
	}
	
	// Yielding on the current try, aka for a given nonnegative yieldVal:
	// - yieldVal must be nonzero AND
	//   - name is a perYield target AND this is not the final attempt OR
	//   - name is a finYield target AND this is the final attempt
	// Yielding on the current message, aka for a negative yieldVal:
	// - Same as above, but without the attempt checks and yieldVal is
	//   guaranteed to be nonzero
	bool Message::willYield(string& name, int yieldVal) {
		bool yielding;
		
		if(yieldVal < 0)
			yielding = perYield && name == dst || finYield && (
				finYieldTarget.empty() || name == finYieldTarget
			);
		else
			yielding = yieldVal && (
				yieldVal <  attempts && perYield && name == dst ||
				yieldVal == attempts && finYield && (
					finYieldTarget.empty() || name == finYieldTarget
				)
			);

		return yielding;
	}
}