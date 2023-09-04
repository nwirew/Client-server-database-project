#pragma once

#include "Parseable.h"

namespace std {
	class Message : public Parseable {
	private:
		void subReset();
	public:
		int attempts;
		int tryRate;
		bool perYield;
		bool finYield;
		bool srcFirst;
		bool dstFirst;
		bool srcLast;
		bool dstLast;
		
		string finYieldTarget;
		string src;
		string dst;
		string dat;
		
		Message();
		Message(Message&);
		Message(const Message&) = default;
		
		void set(string&, TKID&, string&, string&);
		bool isParticipant(string&);
		bool willYield(string&, int = -1);
	};
}