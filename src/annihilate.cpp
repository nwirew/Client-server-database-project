#include <fstream>
#include <iostream>
#include <regex>
#include <unordered_set>

#include "Shr.h"

using namespace std;

unordered_set<string> getSessions(string&);
unordered_set<string> getPids(string&, unordered_set<string>&);
void killProcs(unordered_set<string>&);

int main() {
	cout << "Closing named semaphores.\n";
	Clnt cli;
	cli.~Clnt();
	
	string fname = "nihil.txt";
	
	unordered_set<string> sessions = getSessions(fname);
	unordered_set<string> pids = getPids(fname, sessions);
	killProcs(pids);
	
	string rmcommand =
		"rm " + fname;
	system(rmcommand.c_str());
	
	return 0;
}

unordered_set<string> getSessions(string& fname) {
	unordered_set<string> sessionSet;
	
	string filtercommand =
		"ps xao user,command,sid | "
		"grep \"^\\(USER\\|nrew088\\)\" | "
		"grep \"\\(\\[\\|\\./\\)\\(server\\|client\\|scripts\\)\\|COMMAND\" | "
		"grep -v grep "
		"1> " + fname + ' ' +
		"2> /dev/null";
	system(filtercommand.c_str());
	
	regex rex(R"((\d+)\s*$)");
	smatch sm;
	
	string line;
	ifstream nihil(fname);
	while(getline(nihil, line))
		if(regex_search(line, sm, rex))
			sessionSet.insert(sm.str(1));
	nihil.close();
	
	return sessionSet;
}

unordered_set<string> getPids(string& fname, unordered_set<string>& sessionSet) {
	unordered_set<string> pidSet;
	
	if(!sessionSet.empty()) {
		string pgrepargs = "";
		
		for(const string& s : sessionSet)
			pgrepargs += s + ',';
		pgrepargs.erase(pgrepargs.length()-1);
		
		string pgrepcommand =
			"pgrep -s " +
			pgrepargs +
			' ' +
			"-x \"scripts|server|client\" "
			"1> nihil.txt "
			"2> /dev/null";
		system(pgrepcommand.c_str());
		
		string line;
		ifstream nihil(fname);
		while(getline(nihil, line))
			pidSet.insert(line);
		nihil.close();
	}
	
	return pidSet;
}

void killProcs(unordered_set<string>& pidSet) {
	if(!pidSet.empty()) {
		string killargs = "";
		
		for(const string& p : pidSet)
			killargs += p + ' ';
		killargs.erase(killargs.length()-1);
		
		string killcommand =
			"kill -9 " + killargs;
		
		system(killcommand.c_str());
	}
	
	cout << "Killed " << pidSet.size() << " processes.\n";
}