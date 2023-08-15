all: client-phase1 client-phase2 client-phase3 client-phase4 client-phase5

client-phase1: client-phase1.cpp
	g++ -g -std=c++17 -pthread -o client-phase1 client-phase1.cpp -lcrypto -lssl

client-phase2: client-phase2.cpp
	g++ -g -std=c++17 -pthread -o client-phase2 client-phase2.cpp -lcrypto -lssl

client-phase3: client-phase3.cpp
	g++ -g -std=c++17 -pthread -o client-phase3 client-phase3.cpp -lcrypto -lssl

client-phase4: client-phase4.cpp
	g++ -g -std=c++17 -pthread -o client-phase4 client-phase4.cpp -lcrypto -lssl

client-phase5: client-phase5.cpp
	g++ -g -std=c++17 -pthread -o client-phase5 client-phase5.cpp -lcrypto -lssl