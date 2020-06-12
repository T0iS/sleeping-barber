

Project focused on implementing one of the standard IPC problems - the sleeping barber problem. 

It shows the main purpose of semaphores and data-exchange between processes mainly using pipes and shared memory.

The implementation is created for both processes and threads.
I used mainly posix library tools since I figured out to like these more than the System V ones. Also, I just started out with posix semaphores so I might as well stick with the rest as well right. 


HOW TO USE:

The project is composed of two main parts, the client and the server. 
First you gotta start the server binary after compiling (either process or threads one). 
The barber then waits for the client to arrive. 

Barber and clients exchange messages according to a predefined message format, defined in the barber header file.

