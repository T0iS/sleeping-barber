
all:
	g++ -g -o holic_proc	server_fork.c    holic.c -pthread -lrt
	g++ -g -o holic_thread	server_thread.c    holic.c -pthread -lrt
