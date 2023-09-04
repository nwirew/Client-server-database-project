#include <iomanip>
#include <iostream>

#include "Sequencer/SqrParser.h"

namespace std {
	void testDirPermit();
}

int main(int argc, char** argv) {
	using namespace std;
	
	// testDirPermit();
	
	SqrParser::debug = true;
	Sequencer sqr = SqrParser::create("sequence/RecordStuff.sequence");
	
	int stuff[4] { 5, 6, 7, 8 };
	
	// Sequencer::debug = true;
	sqr.choose("GetRecord");
	
	sqr.iam("CLIENT");
	// sqr.setPartner("CLIENT", 10);
	// sqr.setPartner("CLIENT2", 11);
	sqr.partner("SERVER").desc = 12;
	// sqr.setPartner("K", 13);
	// sqr.setPartner("J", 14);
	
	string pooData = "poooooooooooooooooooooooooooooooooooooooooo";
	
	// sqr.setMessage("THING1", serialize(pooData));
	// sqr.setMessage("THING2", serialize(stuff[1]));
	// sqr.setMessage("THING3", serialize(stuff[2]));
	// sqr.setMessage("THING4", serialize(stuff[3]));
	// sqr.setMessage("THING5", serialize(stuff[3]));
	sqr.data("THING5") = Sequencer::serialize(stuff[3]);
	
	// cout << "+++ Test driver is starting sequencer...\n";
	// sqr.run();
	// while(sqr.simulate());
	// cout << "+++ Test driver after first yield...\n";
	// while(sqr.simulate() < 3)
		// cout << "+++ Test driver after retry yield...\n";
	// sqr.simulate();
	// cout << "+++ Test driver has finished?\n";
	
	return 0;
}



namespace std {
	void testDirPermit() {
		using namespace std;
		
		cout << "------------------------------------------------\n";
		
		bool match;
		DirPermit ps[6] = {
			DirPermit(), DirPermit(), DirPermit(),
			DirPermit(), DirPermit(), DirPermit()
		};
		
		cout
			<< "A ... B and B ... C:\n"
			<< " - Only equal to self because A != C, even though B == B.\n";
		
		ps[0].lp = "A", ps[0].dir = LARROW, ps[0].rp = "B";
		ps[1].lp = "A", ps[1].dir = MARROW, ps[1].rp = "B";
		ps[2].lp = "A", ps[2].dir = RARROW, ps[2].rp = "B";
		ps[3].lp = "B", ps[3].dir = LARROW, ps[3].rp = "C";
		ps[4].lp = "B", ps[4].dir = MARROW, ps[4].rp = "C";
		ps[5].lp = "B", ps[5].dir = RARROW, ps[5].rp = "C";
		
		cout
			<< "\x1b[38;2;255;106;106m"
			<< left << setw(26)
			<< "operator=="
			<< "hash<DirPermit>{}()"
			<< "\x1b[m\n";
		
		for(int i = 0; i < 6; i++) {
			for(int j = 0; j < 6; j++) {
				match = ps[i] == ps[j];
				cout << "  " << (match ?
					"\x1b[38;2;255;255;106mY" :
					"\x1b[38;2;079;079;079mN"
				) << "\x1b[m";
			}
			cout << setw(8) << "";
			for(int j = 0; j < 6; j++) {
				match = hash<DirPermit>{}(ps[i]) == hash<DirPermit>{}(ps[j]);
				cout << "  " << (match ?
					"\x1b[38;2;255;255;106mY" :
					"\x1b[38;2;079;079;079mN"
				) << "\x1b[m";
			}
			cout << endl;
		}
		
		cout
			<< "A ... B and B ... A:\n"
			<< " - Always equal when direction is \"<->\", else equal when \n"
			<< "   direction is exclusively towards B or towards A for both.\n";
		
		ps[3].rp = ps[4].rp = ps[5].rp = "A";
		
		cout
			<< "\x1b[38;2;255;106;106m"
			<< left << setw(26)
			<< "operator=="
			<< "hash<DirPermit>{}()"
			<< "\x1b[m\n";
		
		for(int i = 0; i < 6; i++) {
			for(int j = 0; j < 6; j++) {
				match = ps[i] == ps[j];
				cout << "  " << (match ?
					"\x1b[38;2;255;255;106mY" :
					"\x1b[38;2;079;079;079mN"
				) << "\x1b[m";
			}
			cout << setw(8) << "";
			for(int j = 0; j < 6; j++) {
				match = hash<DirPermit>{}(ps[i]) == hash<DirPermit>{}(ps[j]);
				cout << "  " << (match ?
					"\x1b[38;2;255;255;106mY" :
					"\x1b[38;2;079;079;079mN"
				) << "\x1b[m";
			}
			cout << endl;
		}
		
		cout
			<< "A ... A:\n"
			<< " - Direction makes no difference because this allows a message\n"
			<< "   from self to self. They must be identical in all cases.\n";
		
		ps[0].rp = ps[1].rp = ps[2].rp = "A";
		
		cout
			<< "\x1b[38;2;255;106;106m"
			<< left << setw(26)
			<< "operator=="
			<< "hash<DirPermit>{}()"
			<< "\x1b[m\n";
		
		for(int i = 0; i < 3; i++) {
			for(int j = 0; j < 3; j++) {
				match = ps[i] == ps[j];
				cout << "  " << (match ?
					"\x1b[38;2;255;255;106mY" :
					"\x1b[38;2;079;079;079mN"
				) << "\x1b[m";
			}
			cout << setw(17) << "";
			for(int j = 0; j < 3; j++) {
				match = hash<DirPermit>{}(ps[i]) == hash<DirPermit>{}(ps[j]);
				cout << "  " << (match ?
					"\x1b[38;2;255;255;106mY" :
					"\x1b[38;2;079;079;079mN"
				) << "\x1b[m";
			}
			cout << endl;
		}
	}
}