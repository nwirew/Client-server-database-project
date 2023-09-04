/*\
 *        Author: Nathan Wilson Rew
 *  Date created: November 9, 2022
 *        Course: CSC 552
 *    Instructor: Dr. Spiegel
 *    Assignment: #3
 *      Due date: December 15, 2022
 *     File name: server.cpp
 *       Purpose: Interact with the client application to demonstrate
 *                semaphores, shared memory, and sockets.
\*/

#include <bitset>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <iomanip>
#include <limits>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "Shr.h"
#include "Sequencer/Sequencer.h"
#include "Sequencer/SqrParser.h"

/*\
 *  Global variables and function prototypes
\*/
namespace std {
	// template<typename ...SIGS>
	// void setupSigHandler(int, __sighandler_t, SIGS...);
	
	template<typename ...SIGS>
	void setupSigHandler(int, __sighandler_t, int, SIGS...);
	
	bool setupSharedMemory();
	int  setupSocket(sockaddr_in&, int);
	void listenForClients();
	void cleanupSharedMemory();
	
	void waitForChild(int);
	void parentShutdown(int);
	
	void doServerChild();
	
	void displayRecord();
	void modifyRecord();
	void createRecord();
	void displaySerLog();
	
	template<typename T>
	T cinget();
	
	template<typename ...Args>
	void serLog(Args...);

	Sequencer sqr;
}



int main(int argc, char** argv) {
	using namespace std;
	
	if(!setupSharedMemory()) return 1;
	serLog("--- BEGIN SERVER LOG ---");
	serLog("Server is starting...");
	
	// No SA_RESTART to avoid getting stuck at accept() - program loop takes
	// care of restart logic.
	setupSigHandler(SIGINT,  parentShutdown, SA_NODEFER, SIGCHLD        );
	setupSigHandler(SIGCHLD, waitForChild,   0,          SIGCHLD, SIGINT);

	// SqrParser::debug = true;
	Sequencer::debug = 0;
	sqr = SqrParser::create("sequence/RecordStuff.sequence");
	
	listenForClients();
	
	serLog("Server is exiting. Goodbye!");
	serLog("--- END SERVER LOG ---");
	cleanupSharedMemory();
	
	return 0;
}



/*\
 *  Function implementations
\*/
namespace std {
	bool setupSharedMemory() {
		cout << "Obtaining shared memory...\n";
		// At a location chosen by the OS, allocate enough memory to store a
		// Shr<Srvr> class instance. Access the memory with read and write
		// permissions, let it be shared across processes via the resulting
		// pointer, and do not use a backing file.
		Shr<Srvr>::Ptr = (Shr<Srvr>*)mmap(nullptr, sizeof(Shr<Srvr>),
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON,
			-1, 0);

		cout << "Successfully obtained shared memory.\n";
		
		// Shared memory has been allocated. Now we need to wipe it to 0 and
		// set up the shared instance of the Shr class.
		memset(Shr<Srvr>::Ptr, 0, sizeof(Shr<Srvr>));
		Shr<Srvr>* shr = new (Shr<Srvr>::Ptr) Shr<Srvr>("ser.log");
		
		return true;
	}
	
	
	
	template<typename ...SIGS>
	void setupSigHandler(
	int sigTarget,
	__sighandler_t handler,
	int flags,
	SIGS... sigs) {
		struct sigaction act;
		act.sa_handler = handler;
		act.sa_flags   = flags;
		sigemptyset(&act.sa_mask);
		(sigaddset(&act.sa_mask, sigs), ...);
		sigprocmask(SIG_BLOCK, &act.sa_mask, nullptr);
		
		// sigset_t intSet;
		// sigemptyset(&intSet);
		// sigaddset(&intSet, SIGINT);
		// sigprocmask(SIG_IGNORE)
		
		sigaction(sigTarget, &act, nullptr);
	}
	
	
	
	int setupSocket(sockaddr_in& sin, int reqPort = -1) {
		struct hostent*  hinf = gethostbyname("acad.kutztown.edu");
		struct protoent* pinf = getprotobyname("tcp");
		int sockFD = socket(AF_INET, SOCK_STREAM, pinf->p_proto);
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(reqPort);
		memcpy(&sin.sin_addr, hinf->h_addr, hinf->h_length);
		bind(sockFD, (sockaddr*)&sin, sizeof(sin));
		listen(sockFD, 64);
		return sockFD;
	}
	
	
	
	void listenForClients() {
		sigset_t INT, CHLD;
		
		sigemptyset(&INT);
		sigaddset(&INT, SIGINT);
		
		sigemptyset(&CHLD);
		sigaddset(&CHLD, SIGCHLD);
		
		sigprocmask(SIG_UNBLOCK, &CHLD, nullptr);
		
		Shr<Srvr>* shr = Shr<Srvr>::Ptr;
		
		// Get the parent server's socket descriptor
		struct sockaddr_in sin;
		Srvr::Sock = setupSocket(sin, Srvr::Port);
		socklen_t sinSize = sizeof(sin);
		
		while(!shr->data.serShutdown) {
			serLog("Parent server is listening for clients...");
			
			// 1: Wait for incoming clients.
			sigprocmask(SIG_UNBLOCK, &INT, nullptr);
			Srvr::Conn = accept(Srvr::Sock, (sockaddr*)&sin, &sinSize);
			sigprocmask(SIG_BLOCK, &INT, nullptr);
			
			// 2: Process interrupt signals, which will have activated the
			//    shutdown trigger and forced accept() to exit prematurely.
			if(shr->data.serShutdownTrigger) {
				if(!shr->data.numServers)
					serLog("All children have exited.");
				
				serLog("--> Received interrupt signal.");
				
				// 1: Get the shutdown response
				char choice;
				do	cout << "Shut down the server? (y/n)\n > ",
					choice = tolower(cinget<char>());
				while (choice != 'y' && choice != 'n');
				
				serLog("Shutdown confirmation: ", choice);
				
				// 2: Close the port if requested
				if(choice == 'y')
					shr->data.serShutdown = true,
					close(Srvr::Sock),
					serLog("<-- Server is beginning shutdown.");
				else
					serLog("<-- Server is resuming...");
				
				// 3: Set at end to (mostly) avoid duplicated prompts
				shr->data.serShutdownTrigger = false;
			}
			else
			// 3: Handle the incoming client. accept() returns a non-negative
			//    value on success; we need to check this here in case an
			//    interrupt did NOT activate the server shutdown trigger.
			if(Srvr::Conn != -1) {
				sem_wait(&shr->data.semNumServers);
				
				if(!fork()) { // Child process
					serLog(
						"Child server ", getpid(), " is closing connection "
						"port ", Srvr::Port, '.'
					);
					close(Srvr::Port);  // Connection port closed
					Srvr::IP = inet_ntoa(sin.sin_addr);
					Srvr::Port = ntohs(sin.sin_port);
					serLog(
						"Child server ", getpid(),
						" is handling a new client from ",
						Srvr::IP, ":", Srvr::Port
					);
					shr->data.numServers++;
					sem_post(&shr->data.semNumServers);
					doServerChild();
					serLog(
						"Child server ", getpid(), " is closing service "
						"port ", Srvr::Port, '.'
					);
					close(Srvr::Port);  // Service port closed
					exit(0);
				}
				
				/* --------------------------------------------------------- *\
				** --- PARENT ONLY ----------------------------------------- **
				\* --------------------------------------------------------- */
				
				// Wait on numservers until child releases it in the if block
				// above
				sem_wait(&shr->data.semNumServers);
				sem_post(&shr->data.semNumServers);
				
				// Parent does not need child socket
				close(Srvr::Conn);
			}
		}
		
		// The port was closed in the SIGINT handler that allowed us to escape
		// the above loop.
		
		// Child servers may still be active after manual SIGINT. Since we unset
		// our signal handlers, there are no data races here.
		signal(SIGCHLD, SIG_DFL);
		signal(SIGINT , SIG_IGN);
		sigprocmask(SIG_UNBLOCK, &INT, nullptr);
		if(shr->data.numServers)
			serLog("Waiting for ", shr->data.numServers, " clients to finish...");
		while(shr->data.numServers)
			serLog(wait(0), " exited, ", --shr->data.numServers, " remain");
		serLog("All clients finished.");
	}
	
	
	
	void cleanupSharedMemory() {
		// Semaphores are released in the Shr destructor.
		Shr<Srvr>*& shr = Shr<Srvr>::Ptr;
		shr->~Shr();
		
		// Release the memory. If error, let the OS deal with it when the
		// parent process ends. If no error, nullify the shared memory pointer.
		cout << "Unmapping shared memory...\n";
		if(munmap(shr, sizeof(Shr<Srvr>)))
			perror("Could not unmap shared memory");
		else
			cout << "Successfully unmapped shared memory.\n",
			shr = nullptr;
	}
	
	
	
	// Child exited
	void waitForChild(int sigNum) {
		Srvr& shmData = Shr<Srvr>::Ptr->data;
		
		// SIGCHLD can arrive in clumps. Need to pick them apart manually.
		pid_t childPID;
		while((childPID = waitpid(-1, nullptr, WNOHANG)) > 0)
			serLog("Child server ", childPID, " has closed."),
			shmData.numServers--;
		
		if(!shmData.numServers)
			kill(getpid(), SIGINT);
	}


	
	// Server exit
	// Allow SIGCHLD to be handled when waiting for user input.
	void parentShutdown(int sigNum) {
		Srvr& shmData = Shr<Srvr>::Ptr->data;
		
		// If control is already nested somewhere inside this handler, skip it.
		if(!sem_trywait(&shmData.semSigintGuard))
			shmData.serShutdownTrigger = true,
			sem_post(&shmData.semSigintGuard);
	}
	
	
	
	void doServerChild() {
		sigset_t blocked;
		sigemptyset(&blocked);
		sigaddset(&blocked, SIGINT);
		sigprocmask(SIG_BLOCK, &blocked, nullptr);
		
		pid_t pid = getpid();
		
		// TODO: "A child server can only catch a signal while waiting
		// for select()"
		
		sqr.choose("TellPort");
		sqr.iam("SERVER");
		sqr.partner("CLIENT").desc = Srvr::Conn;
		sqr.data("PortNum") = Sequencer::serialize(Srvr::Port);
		sqr.run();
		
		serLog("Child server ", pid, " is ready.");
		
		sqr.data("CommandID").resize(sizeof(MsgType));
		sqr.data("Decision").resize(sizeof(MsgType));
		MsgType response = NOTHING, decision;
		char command;
		
		while(command != 'e') {
			sqr.choose("Command");
			
			// Get requested command
			signal(SIGINT, SIG_IGN);
			sigprocmask(SIG_UNBLOCK, &blocked, nullptr);
			sqr.run();
			sigprocmask(SIG_BLOCK, &blocked, nullptr);
			
			// Decode command and send decision
			command = Sequencer::deserialize<char>(sqr.data("CommandID"));
			decision = Shr<Srvr>::Ptr->data.serShutdown ? STOP : DEFAULT;
			sqr.data("Decision") = Sequencer::serialize<MsgType&>(decision);
			sqr.run();
			
			switch(command) {
				case 'd': displayRecord(); break;
				case 'm': modifyRecord();  break;
				case 'n': createRecord();  break;
				case 's': displaySerLog(); break;
			}
		}
		
		serLog("Child server ", pid, ": Client ", Srvr::Port, " has disconnected. Exiting...");
	}
	
	
	
	void displayRecord() {
		Srvr& data = Shr<Srvr>::Ptr->data;
		int numRecs = 0;
		Record rec;
		
		// 1: Count the records in the file.
		data.beginReadBinFile();  // CS begin
		data.binFile.clear();
		data.binFile.seekg(0, ios::beg);
		while(data.binFile.read((char*)&rec, sizeof(Record))) numRecs++;
		data.endReadBinFile();    // CS end
		
		// 2: Send the number of records to the client and get client's choice
		sqr.choose("ChooseIndex");
		sqr.data("RecordCount") = Sequencer::serialize(numRecs);
		sqr.data("RecordChoice").resize(sizeof(int));
		sqr.run();
		
		int choice = Sequencer::deserialize<int>(sqr.data("RecordChoice"));
		
		sqr.choose("SendRecord");
		sqr.iam("SENDER");
		sqr.partner("RECEIVER") = sqr.partner("CLIENT");
		sqr.data("Record").resize(sizeof(Record));
		
		/* ----------------------------------------------------------------- *\
		 * --- Begin critical section -------------------------------------- *
		\* ----------------------------------------------------------------- */
		data.beginReadBinFile();
		data.binFile.clear();
		if(choice == -999) { // Print all records
			serLog("All records: Sending to client ", Srvr::IP, ':',
				Srvr::Port, "...");
			
			data.binFile.seekg(0, ios::beg);
			while(data.binFile >> rec) {
				sqr.data("Record") = rec.serialize();
				sqr.run();
			}
			
			serLog("All records: Finished sending to client ", Srvr::IP, ':',
				Srvr::Port, ".");
		}
		else { // Print the chosen record
			serLog("Record ", choice, ": Sending to client ", Srvr::IP, ':',
				Srvr::Port, "...");
			
			// Find and send the chosen record
			data.binFile.seekg((choice-1)*sizeof(Record), ios::beg);
			data.binFile >> rec;
			sqr.data("Record") = rec.serialize();
			sqr.run();
			
			serLog("Record ", choice, ": Finished sending to client ",
				Srvr::IP, ':', Srvr::Port, '.');
		}
		data.endReadBinFile();
		/* ----------------------------------------------------------------- *\
		 * --- End critical section ---------------------------------------- *
		\* ----------------------------------------------------------------- */
		
		// Set my sequencer role back to my chosen default
		sqr.iam("SERVER");
	}
	
	
	
	void modifyRecord() {
		Srvr& data = Shr<Srvr>::Ptr->data;
		int numRecs = 0;
		Record rec;
		const Record blank;
		
		// 1: Count the records in the file.
		data.beginReadBinFile();  // CS begin
		data.binFile.clear();
		data.binFile.seekg(0, ios::beg);
		while(data.binFile.read((char*)&rec, sizeof(Record))) numRecs++;
		data.endReadBinFile();    // End CS
		
		// 2: Send the number of records to the client and get client's choice
		sqr.choose("ModifyRecord");
		sqr.data("RecordCount") = Sequencer::serialize(numRecs);
		sqr.data("RecordChoice").resize(sizeof(int));
		sqr.data("Record").resize(sizeof(Record));
		sqr.run();
		int choice = Sequencer::deserialize<int>(sqr.data("RecordChoice"));
		
		// 3: Get the requested record from the data file
		data.beginReadBinFile();  // CS begin
		data.binFile.clear();
		data.binFile.seekg((choice-1)*sizeof(Record), ios::beg);
		data.binFile >> rec;
		data.endReadBinFile();    // CS end
		
		// 4: Send the requested record and receive the modified record
		sqr.data("Record") = rec.serialize();
		sqr.run();
		rec.deserialize(sqr.data("Record"));
		
		// Record is a POD type. We want to skip writing the received record if
		// it's blank, as this would probably mean the client terminated before
		// sending an actual record.
		if(!memcmp(&rec, &blank, sizeof(Record))) {
			data.beginWriteBinFile();  // CS begin
			data.binFile.clear();
			data.binFile.seekp((choice-1)*sizeof(Record), ios::beg);
			data.binFile << rec;
			data.endWriteBinFile();    // CS end
		}
		
		serLog("Record ", choice, ": Modified by client ", Srvr::IP, ':',
			Srvr::Port, '.');
	}
	
	
	
	void createRecord() {
		Srvr& data = Shr<Srvr>::Ptr->data;
		Record rec;
		
		// 1: Get the new record
		sqr.choose("SendRecord");
		sqr.iam("RECEIVER");
		sqr.partner("SENDER") = sqr.partner("CLIENT");
		sqr.data("Record").resize(sizeof(Record));
		sqr.run();
		rec.deserialize(sqr.data("Record"));
		
		// 2: Put it in the data file
		data.beginWriteBinFile();  // CS begin
		data.binFile.clear();
		data.binFile.seekp(0, ios::end);
		data.binFile << rec;
		data.endWriteBinFile();    // CS end
		
		serLog("New record created by client ", Srvr::IP, ':', Srvr::Port, '.');
		sqr.iam("SERVER");
	}
	
	
	
	void displaySerLog() {
		// 1: Pause signal handling
		sigset_t blocked;
		sigemptyset(&blocked);
		sigaddset(&blocked, SIGCHLD);
		sigaddset(&blocked, SIGINT);
		sigprocmask(SIG_BLOCK, &blocked, nullptr);
		
		serLog("Client ", Srvr::IP, ':', Srvr::Port, " has requested the log "
			"file.");
		
		Shr<Srvr>* shr = Shr<Srvr>::Ptr;
		Srvr& data = shr->data;
		int chunkSize = 128;
		
		sqr.choose("SendLogFile");
		sqr.iam("SENDER");
		sqr.partner("RECEIVER") = sqr.partner("CLIENT");
		sqr.data("LogFileChunk").resize(chunkSize+1);
		
		vector<unsigned char>& chunk = sqr.data("LogFileChunk");
		
		// 2: Acquire the log file semaphore as a reader
		sem_wait(&data.semOrderLogFile);
		sem_wait(&data.semNumReadersLogFile);
		if(++data.numReadersLogFile == 1)
			sem_wait(&data.semWriteLogFile);
		sem_post(&data.semOrderLogFile);
		sem_post(&data.semNumReadersLogFile);
		
		// 3: Send the log file, in chunks, to the client
		// CS begin
		shr->logFile.seekg(0, ios::beg);
		do {
			shr->logFile.read((char*)chunk.data(), chunkSize);
			sqr.data("LogFileChunk").back() = shr->logFile.gcount();
			sqr.run();
		} while(sqr.data("LogFileChunk").back() == chunkSize);
		shr->logFile.clear();
		// CS end
		
		// 4: Release the log file semaphore as a reader
		sem_wait(&data.semNumReadersLogFile);
		if(--data.numReadersLogFile == 0)
			sem_post(&data.semWriteLogFile);
		sem_post(&data.semNumReadersLogFile);
		
		sqr.iam("SERVER");
		
		// 5: Resume signal handling
		sigprocmask(SIG_UNBLOCK, &blocked, nullptr);
	}
	
	
	
	template<typename T>
	T cinget() {
		T dest;
		cin >> dest;
		if(cin.fail()) cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		return dest;
	}
	
	
	
	template<typename ...Args>
	void serLog(Args... args) {
		using namespace std;
		
		sigset_t blocked;
		sigemptyset(&blocked);
		sigaddset(&blocked, SIGCHLD);
		sigaddset(&blocked, SIGINT);
		sigprocmask(SIG_BLOCK, &blocked, nullptr);
		
		Shr<Srvr>* shr = Shr<Srvr>::Ptr;
		Srvr& data = shr->data;
		
		// Fair readers-writer's algorithm
		sem_wait(&data.semOrderLogFile);
		sem_wait(&data.semWriteLogFile);
		sem_post(&data.semOrderLogFile);
		(shr->logFile << ... << args) << endl;
		(cout         << ... << args) << endl;
		sem_post(&data.semWriteLogFile);
		
		sigprocmask(SIG_UNBLOCK, &blocked, nullptr);
	}
}