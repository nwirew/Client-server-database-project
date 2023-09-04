#include <cstring>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Sequencer.h"

namespace std {
	
	/* --------------------------------------------------------------------- *\
	 * --- SEQUENCER ------------------------------------------------------- *
	\* --------------------------------------------------------------------- */
	
	// --- Statics --- //
	
	int Sequencer::debug = 0;
	
	
	
	// --- Constructors --- //
	
	Sequencer::Sequencer()
	  : sub{0}, msg{0}, tries{0}, allowYield{true},
	    choice{}, seqs{}, byteData{}, partners{},
	    Parseable(SEQUENCELIST, 0, 0) {}
	
	
	
	// --- Private --- //
	
	void Sequencer::subReset() {
		me.clear();
		choice.clear();
		seqs.clear();
	}
	
	void Sequencer::establish() {
		unordered_map<string,Message> mostRecent;
		
		using kvss = unordered_map<string,Sequence>::value_type;
		for(const kvss        & entry : seqs             )
		for(const Subsequence & sub   : entry.second.subs)
		for(const Message     & msg   : sub.messages     ) {
			partners[msg.src],  partners[msg.dst],  byteData[msg.dat];
			
			if(!mostRecent.count(msg.src))
				const_cast<Message&>(msg).srcFirst = true;
			if(!mostRecent.count(msg.dst))
				const_cast<Message&>(msg).dstFirst = true;
			
			mostRecent[msg.src] = mostRecent[msg.dst] = msg;
		}
		
		using kvsm = unordered_map<string,Message>::value_type;
		for(const kvsm & mr : mostRecent)
			const_cast<Message&>(mr.second).srcLast =
			const_cast<Message&>(mr.second).dstLast =
				true;
	}
	
	bool Sequencer::checkData() {
		bool okay = true;

		if(choice == "") {
			cout << "Please choose a sequence\n";
			okay = false;
		}
		
		if(me.empty()) {
			cout << "Please choose a role for this sequencer before running "
			<< choice << endl;
			okay = false;
		}

		if(okay) {
			bool
				myDataOkay = checkMyData(),
				myPartnersOkay = checkPartners(),
				partnersDataOkay;
			
			if(!myDataOkay || !myPartnersOkay)
				cout << "Please supply the requested data before running "
				<< choice << '"' << endl;
			else
				partnersDataOkay = checkPartnerData();

			okay = myDataOkay && myPartnersOkay && partnersDataOkay;
		}

		if(debug > 0)
			cout << "- Data check complete.\n";
		
		return okay;
	}
	
	bool Sequencer::checkMyData() {
		if(debug > 0)
			cout << "- Verifying data handles for sequence \""
			<< choice << "\"\n";
		
		Sequence& seq = seqs[choice];
		
		bool allDataOkay = true;
		unordered_set<string> checked;
		
		for(Subsequence & ss : seq.subs   )
		for(Message     &  m : ss.messages)
			if(!checked.count(m.dat)) {
				checked.emplace(m.dat);
				if(allDataOkay)
					allDataOkay = handleOkay(m.dat);
				bool yieldNow = m.perYield || m.finYield &&
					(m.finYieldTarget == me || m.finYieldTarget == "");
				if(allDataOkay && yieldNow)
					goto checkMyDataYield;
			}
		
		checkMyDataYield:
		return allDataOkay;
	}
	
	bool Sequencer::handleOkay(string& hName) {
		bool
			found = byteData.count(hName),
			holds = byteData.at(hName).size(),
			dataOkay = found && holds;
		
		if(!found) cout << "Data \"" << hName << "\" not defined\n";
		else
		if(!holds) cout << "Data \"" << hName << "\" not attached\n";
		
		return dataOkay;
	}
	
	bool Sequencer::checkPartners() {
		if(debug > 0)
			cout << "- Verifying participants for sequence \""
			<< choice << "\"\n";
		
		Sequence& seq = seqs[choice];
		
		bool allPartnersOkay = true;
		unordered_set<string> checked { me };
		
		for(Subsequence & ss : seq.subs        )
		for(Message     &  m : ss.messages     )
		for(const string&  p : { m.src, m.dst })
			if(!checked.count(p)) {
				checked.emplace(p);
				if(allPartnersOkay)
					allPartnersOkay = partnerOkay(p);
				bool yieldNow = m.perYield || m.finYield &&
					(m.finYieldTarget == me || m.finYieldTarget == "");
				if(allPartnersOkay && yieldNow)
					goto checkPartnersYield;
			}
		
		checkPartnersYield:
		return allPartnersOkay;
	}
	
	bool Sequencer::partnerOkay(const string& pName) {
		bool
			found = partners.count(pName),
			opened = partners[pName].desc != -1 || pName == me,
			dataOkay = found && opened;
		
		if(!found)  cout << "Partner \"" << pName << "\" not defined\n";
		else
		if(!opened) cout << "Partner \"" << pName << "\" descriptor not "
		"specified\n";
		
		return dataOkay;
	}
	
	bool Sequencer::checkPartnerData(Message* ref) {
		if(debug > 0)
			cout << "- Verifying partner data for sequence \"" << choice
			<< "\"\n";
		
		Sequence& seq = seqs[choice];
		unordered_set<string> checkedData;
		unordered_set<string> checkedPartners;
		
		bool allSizesOkay = true;
		
		for(int i = sub; i < seq.subs.size(); i++) {
		Subsequence& ss = seq.subs[i];
		for(int j = msg; j < ss.messages.size(); j++) {
		Message& m = ss.messages[j];
		if(m.isParticipant(me)) {
			string&
				self = m.src == me ? m.src : m.dst,
				them = m.src == me ? m.dst : m.src;
			
			// For all messages:
			// - Skip if we already checked partner+message combination
			// When a reference message is given:
			// - Skip if them is not a participant
			// When a reference message is not given:
			// - Check all messages
			if(!( checkedPartners.count(them) && checkedData.count(m.dat) )
				&&
				( !ref || ref->isParticipant(them) )
			) {
				if(debug > 0)
					cout << "- Rendezvous: " << them << '+' << m.dat << endl;
				// Both partners send a ping...
				cvec rendezvous;
				sendMsg(partners[them].desc, rendezvous, RENDEZVOUS);
				MsgType response = recvMsg(partners[them].desc, rendezvous);
				
				if(response != RENDEZVOUS)
					throw runtime_error(self + " could not rendezvous "
					"with " + them + " (do both of them run the \"" +
					choice + "\" sequence?)");
				else if(!rendezvous.empty())
					throw runtime_error(self + " received non-empty "
					"message from " + them + " during rendezvous");
				// ...then check data sizes
				else {
					checkedData.insert(m.dat);
					checkedPartners.insert(them);
					if(allSizesOkay) allSizesOkay = sizeOkay(m);
				}
			}
			// else we don't care about messages we don't participate in
		}}}
		
		if(!allSizesOkay)
			cout << "Please ensure corresponding source and destination "
			"containers are the same size before running \""
			<< choice << "\"\n";
		
		for(const string& name : checkedPartners) {
			partners[name].checkAgain = false;
		}
		
		return allSizesOkay;
	}
	
	bool Sequencer::sizeOkay(Message& m) {
		string&
			self = me == m.src ? m.src : m.dst,
			them = me == m.src ? m.dst : m.src;
		
		int
			intSize = sizeof(int)/sizeof(char),
			mine = byteData[m.dat].size(),
			theirs;

		cvec serialSize(intSize);
		
		if(debug > 0)
			cout << "- Data size check: " << them << '+' << m.dat << endl;
		
		memcpy(serialSize.data(), &mine, intSize);
		sendMsg(partners[them].desc, serialSize);
		recvMsg(partners[them].desc, serialSize);
		memcpy(&theirs, serialSize.data(), intSize);

		if(mine != theirs)
			cout << "Size mismatch for message handle \"" << m.dat << "\" "
			<< "(" << self << "'s is " << mine << ", " << them << "'s is "
			<< theirs << ")\n";
		
		return mine == theirs;
	}
	
	void Sequencer::sendMsg(int sd, cvec& bkt, MsgType&& t, int&& fgs) {
		// 1: Prepare the message by appending its type
		bkt.push_back(t);
		
		if(debug > 2) {
			cout << "----- Send bucket (with message type):\n----- {";
			for(const char& c : bkt)
				cout << ' ' << (int)c;
			cout << " }\n";
		}
		// 2: Share the message pieces
		int totBytes = bkt.size();
		unsigned char* const vecData = const_cast<unsigned char*>(bkt.data());
		int remBytes, sendAmt, sent = 64, totSent = 0;
		
		while(sent >= 0 && totSent < totBytes) {
			remBytes = totBytes - totSent;
			sendAmt  = remBytes < 64 ? remBytes : 64;
			sent     = ::send(sd, vecData+totSent, sendAmt, fgs);
			totSent += sent;
		}
		
		// 3: Return the local bucket to its original size
		const_cast<cvec&>(bkt).pop_back();
	}
	
	MsgType Sequencer::recvMsg(int sd, cvec& bkt, int&& fgs) {
		// Prior error checks ensure network source and destination are the
		// same size. We just need to push/pop one extra byte for message type.

		// 1: Prep the bucket by reserving space for the message type byte
		bkt.resize(bkt.size()+1);
		int totBytes = bkt.size();
		unsigned char* const vecData = bkt.data();
		memset(vecData, 0, totBytes);
		
		// 2: Collect the message pieces
		static int counter = 0;
		int remBytes, recvAmt, recd = 64, totRecd = 0;
		while(recd >= 0 && totRecd < totBytes) {
			remBytes = totBytes - totRecd;
			recvAmt  = remBytes < 64 ? remBytes : 64;
			recd     = ::recv(sd, vecData+totRecd, recvAmt, fgs);
			errno = 0;
			if(recd > 0) totRecd += recd;
		}
		
		if(debug > 2) {
			cout << "----- Recv bucket (with message type):\n----- {";
			for(const unsigned char& c : bkt)
				cout << ' ' << (int)c;
			cout << " }\n";
		}
		
		// 3: Retrieve the message type for the caller
		MsgType t;
		if(totRecd > 0) {
			t = (MsgType&)bkt.back();
			bkt.pop_back();
		}
		else
			t = NOTHING;
		
		return t;
	}
	
	void Sequencer::checkYield() {
		if(sub || msg)
			throw runtime_error("Sequencer cannot be modified while a yielded "
				"sequence is in progress. To quit a yielded sequence, call "
				"sequencer::stop(bool)");
	}
	
	
	
	// --- Public --- //
	
	void Sequencer::choose(string&& seqName) {
		checkYield();
		if(seqs.count(seqName)) {
			choice = seqName;
			restart();
		}
		else
			throw runtime_error("Sequence \"" + seqName + "\" is not defined");
	}
	
	void Sequencer::iam(string&& pName) {
		checkYield();
		if(partners.count(pName))
			me = pName;
		else
			throw runtime_error("\"" + pName + "\" is not established as a "
			"participant in this sequencer");
	}
	
	Partner& Sequencer::partner(string&& pName) {
		// checkYield();
		bool rHst = false, rFut = false, allFound = false;
		
		Sequence& s = seqs[choice];
		for(int i = 0; i < s.subs.size() && !allFound; i++) {
		Subsequence& ss = s.subs[i];
		for(int j = 0; j < ss.messages.size() && !allFound; j++) {
			Message& m = ss.messages[j];
			
			if(m.isParticipant(me) && m.isParticipant(pName))
				i < sub || i == sub && j < msg ? rHst : rFut = true;
			
			allFound = rHst && rFut;
		}}
		
		if(checkHistory(pName, true) && checkHistory(pName, false))
			throw runtime_error("\"" + pName + "\" may only be modified "
			"before or after all messages involving it and \"" + me + "\""
			"have been exchanged");
		else
		if(!partners.count(pName))
			throw runtime_error("\"" + pName + "\" is not established as a "
			"participant in this sequencer");
		else
			return partners[pName];
	}

	bool Sequencer::checkHistory(string& pName, bool past) {
		bool found = false;
		Sequence& s = seqs[choice];
		
		int subStart = past ? sub        : 0,
			subEnd   = past ? s.subs.size() : sub;
		for(int i = subStart; i <= subEnd && !found; i++) {
			Subsequence& ss = s.subs[i];
			
			int rMsgStart = i == sub ? msg : 0,
				rMsgEnd   = i == sub ? msg : ss.messages.size();
			for(int j = rMsgStart; j < rMsgEnd && !found; j++) {
				Message& m = ss.messages[j];
				found = m.isParticipant(me) && m.isParticipant(pName);
			}
		}
		
		return found;
	}
	
	cvec& Sequencer::data(string&& name) {
		if(byteData.count(name))
			return byteData[name];
		else
			throw runtime_error("\"" + name + "\" is not established as a "
			"data handle in this sequencer");
	}
	
	// Returns:
	// - == 0 when the run finishes
	// - >= 1 at yield (# of current message tries)
	// Preconditions:
	// - For each message with this sequencer, a partner, and a data handle, up
	//   to either the next yield point or the end of the sequence, the size of
	//   the vector<char> referenced in this sequencer's byteData by the handle
	//   name is the same as that in the partner's byteData.
	// - For each message that this sequencer sends, with this sequencer and a
	//   data handle, up to either the next yield point or the end of the
	//   sequence, the vector<char> referenced in this sequencer's byteData has
	//   previously been filled with the data that should be sent during this
	//   specific function call instance.
	// Postconditions:
	// - Every byteData vector<char> referenced by a message in the latest run
	//   of the sequence will contain data that can be deserialized into a
	//   sensible instance of some expected data type.
	int Sequencer::run() {
		if(debug > 0) {
			cout << "------------------------------------------------\n"
			<< "- " << (sub || msg || tries ? "RESUMING " : "START OF ")
			<< "RUN FOR \"" << choice << "\"\n";
		}

		if(checkData()) {
			Sequence& s = seqs[choice];
			bool tryAgain = false;

			// Send or receive messages
			do {
				bool selfYield = false;
				int  yieldVal  = 0;
				Message m = prepareMessage(s.subs[sub].messages[msg]);
				MsgType r = NOTHING;

				string&
					self = me == m.src ? m.src : m.dst,
					them = me == m.src ? m.dst : m.src;
				
				// Try the one message as many times as specified
				do {
					if(partners[them].checkAgain) {
						if(debug > 0)
							cout << "- PARTNER RESUMED, DATA CHECK FOR \""
							<< choice << "\"\n";
						checkPartnerData(&m);
						partners[them].checkAgain = false;
					}

					if(debug > 1)
						cout << "--- Attempt " << tries+1 << " for " << m.dat
						<< endl;
					
					r = tryMessage(m);
					
					// TODO: isn't there a function call for this?
					if( m.perYield || m.finYield && (
						m.finYieldTarget == me || m.finYieldTarget.empty()
					))		yieldVal = tries;

					tryAgain = nextTry(r);
					
					// Who is yielding?
					partners[m.src].checkAgain = m.willYield(m.src);
					partners[m.dst].checkAgain = m.willYield(m.dst);
					selfYield = m.willYield(me, yieldVal);
					
					// I never need to check myself
					partners[me].checkAgain = false;
					
				} while(tries && r == NOTHING && !selfYield);
				// (tries && !tryAgain) ≡> false
				
				if(allowYield && selfYield) {
					if(debug > 0)
						cout << "- YIELDING IN RUN FOR \"" << choice << "\"\n"
						<< "------------------------------------------------\n";
					for(pair<const string,Partner>& p : partners)
						if(p.first != me)
							p.second.checkAgain = true;
					return yieldVal;
				}
			} while(tryAgain);
		}
		// do-while: try at least once, exit when counters reset (aka at end)

		if(debug > 0)
			cout << "- END OF RUN FOR \"" << choice << "\"\n"
			<< "------------------------------------------------\n";
		restart();
		return 0;
	}
	
	Message Sequencer::prepareMessage(Message& m1) {
		Message m2(const_cast<Message&>(m1));
		
		// Message source partner and non-participants should be single-try
		if(m2.dst != me) {
			m2.tryRate = 0;
			m2.perYield = false;
			m2.attempts = 1;
			
			if(debug > 0)
				cout << "- Sending " << m2.dat << " to " << m2.dst << endl;
		}
		else
		if(debug > 0)
			cout << "- Receiving " << m2.dat << " from " << m2.src << endl;

		return m2;
	}
	
	MsgType Sequencer::tryMessage(Message& m) {
		MsgType r = NOTHING;

		// TODO: poll()?
		if(m.tryRate) usleep(1000*m.tryRate);

		if(m.src == me) {
			sendMsg(partners[m.dst].desc, byteData[m.dat]);
			r = SEND;
		}
		else
		if(m.dst == me) {
			r = recvMsg(
				partners[m.src].desc,
				byteData[m.dat],
				m.attempts > 1 ? MSG_DONTWAIT : 0
			);
		}
		else {
			if(debug > 0)
				cout << "- I am not involved\n";
			r = SKIP;
		}
		
		if(debug > 0)
			if(r == NOTHING)
				cout << "- No response\n";
			else
			if(r == SKIP)
				cout << "- Skipping this message...\n";
		
		tries++;

		return r;
	}
	
	bool Sequencer::nextTry(MsgType lastResponse) {
		// 1: Get current sequence state...
		Sequence& s = seqs[choice];

		// 2: Increment like addition; carry the ones!
		Subsequence & ss =  s.subs    [sub];
		Message     &  m = ss.messages[msg];

		// Next try if bad response
		if(lastResponse == NOTHING) {
			if(!(tries = tries %  m.attempts       ))
			if(!(msg   = ++msg % ss.messages.size()))
				 sub   = ++sub %  s.subs    .size();
		}
		// Next message if okay response
		else {
			tries = 0;
			if(!(msg   = ++msg % ss.messages.size()))
				 sub   = ++sub %  s.subs.size()    ;
		}

		return sub || msg || tries;
	}
	
	void Sequencer::stop() {
		restart();
		// TODO: Send stop message to all partners?
		// {
		// 	// Gather a set of parties in all subsequences
		// 	// Send a stop message exactly once to each of them
		// 	// Except some of them might have already finished...
		// }
	}
	
	void Sequencer::restart() {
		sub = msg = tries = 0;
	}
}