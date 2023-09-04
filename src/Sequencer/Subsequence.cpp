#include "Subsequence.h"

namespace std {
	/* ---------------------------------------------------------------------- *\
	 * --- SUBSEQUENCE ------------------------------------------------------ *
	\* ---------------------------------------------------------------------- */
	
	
	
	// --- Constructors --- //
	
	Subsequence::Subsequence(/*----------*/)
	  : perms{}, messages{}, parties{}, Parseable(SUBSEQUENCE, 0, 0) {}
	
	
	
	// --- Private --- //
	
	void Subsequence::subReset() {
		perms.clear();
		messages.clear();
		parties.clear();
	}
	
	
	
	// --- Public --- //
	
	bool Subsequence::addPermit(DirPermit& perm) {
		if(perm.dir == MARROW) {
			DirPermit
				dpl(perm),        dpr(perm);
				dpl.dir = LARROW; dpr.dir = RARROW;
			
			bool addL = addPermit(dpl), addR = addPermit(dpr);
			return addL || addR;
		}
		else {
			bool success = get<bool>(perms.insert(perm));
			if(success) parties.insert(perm.lp), parties.insert(perm.rp);
			return success;
		}
	}
	
	void Subsequence::addMessage(Message& msg) {
		if(!parties.count(msg.src)) printError(msg, SUBSEQBADSRC, msg.src);
		else
		if(!parties.count(msg.dst)) printError(msg, SUBSEQBADDST, msg.dst);
		else
		if(!perms.count(DirPermit(msg.src, RARROW, msg.dst)))
			printError(msg, SUBSEQBADMSG);
		else
			messages.push_back(msg);
	}
}