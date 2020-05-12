
all:
	g++ -g -o holic_proc	server_fork.cpp    holic.cpp -pthread -lrt

