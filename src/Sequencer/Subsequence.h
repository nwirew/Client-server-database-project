#pragma once

#include <unordered_set>
#include <vector>

#include "Parseable.h"
#include "Message.h"
#include "DirPermit.h"

namespace std {
	class Subsequence : public Parseable {
	private: 
		void subReset();
	public:
		unordered_set<DirPermit> perms;
		unordered_set<string> parties;
		vector<Message> messages;
		
		Subsequence();
		Subsequence(int&, int&);
		
		bool addPermit(DirPermit&);
		void addMessage(Message&);
	};
}