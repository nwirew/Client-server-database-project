# Client/server database project
Final project for Advanced Unix Programming, Kutztown University, Fall 2022

This is a (technically) functional client/server database demo created from scratch, intended to demonstrate understanding of concepts including semaphores, shared memory, network sockets, concurrent processing, and UNIX signal handling. It is written in C++17, intended to be compiled with g++, and targets UNIX operating systems. The included makefile contains commands for building and testing.

The demo is currently hardcoded to connect to the university's CS department server (which will work only if you connect to their vpn), but changing this should be as simple as modifying the hostent structs in client.cpp and server.cpp; the port number, too, is hardcoded to 2008, which can be changed in Shr.cpp (see Clnt::Port and Srvr::Port).

The client and server also make use of a custom-made sequencing script to help abstract and coordinate send and recv function calls. Message sequences are described in a file accessible by both programs, and a program only needs to specify a sequence name, its own name, the name/s of its partner/s, and a serialized form of the data it will send - the sequencer class then manages sending the serialized data to the appropriate recipients in the appropriate order.

Please note that this project is not entirely bug-free - in fact, if you modify and compile it to run on your system you are almost guaranteed to make your terminal hang if you run the "hundred.sh" test script. The project includes a program named "Annihilate" which attempts to release all locked resources in such a scenario, but there is no guarantee that it handles every possible scenario. Try it out only at your own risk!
