
all:
	g++ -g -o holic_proc	server_fork.cpp    holic.cpp -pthread -lrt
	g++ -g -o holic_thread	server_thread.cpp    holic.cpp -pthread -lrt
