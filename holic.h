#ifndef holic

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>

// cmd messages
#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
extern int debug;

// Other used constants
#define CHAIR_COUNT 2


//SHM folder
#define SHM_NAME "/shm_holicstvi"
#define MQ_NAME "/mq_holic"
#define SEM_BARBER "/sem_barber"
#define SEM_CUSTOMERS "/sem_customers"
#define SEM_CUTTING "/sem_cutting"


// Prikazy protokolu
#define CI_Vstup        80
#define CS_Vstup        "Prichazi zakaznik"
#define CI_Strihat      81
#define CS_Strihat      "Pojdte se strihat"
#define CI_Hotovo       82
#define CS_Hotovo       "Dostrihano, muzete odejit"

// Odpovedi protokolu
#define AI_MistoX       70
#define AS_MistoX       "Misto %d je v cekarne volne"
#define AI_Zakazka      71
#define AS_Zakazka      "Chci strihat po dobu %d"
#define AI_Nashledanou  72
#define AS_Nashledanou  "Nashledanou"

// Chyby
#define EI_Klient       90
#define ES_Klient       "Obecna chyba klienta"
#define EI_Server       91
#define ES_Server       "Obecna chyba serveru"
#define EI_Obsazeno     92
#define ES_Obsazeno     "Holicstvi je obsazene"


// Struct for readline
struct fdStruct{
    
    int fd;
    char BUF[BUFSIZ];
    
};

// Message struct
struct msg {

    char typ;
    int NN;
    char text[252];

};

// Shared memory struct
struct shared_memory{

    int chairs[CHAIR_COUNT];
    int last_chair;
    int customer_count;
    int child_count;
};


// EXTERNS
extern int debug;
extern shared_memory* global_data;
extern sem_t* sem_customers;
extern sem_t* sem_cutting;
extern sem_t* sem_barber;

extern int comPipe[2];
extern fdStruct fd_stdin;
extern fdStruct fd_pipe;
extern fdStruct fd_sock_client;
extern char BBUF[BUFSIZ];


// FUNCTIONS

// Print a message
void log_msg(int log_level, const char *form, ... );

// help
void help(int num, char **arg);

// Reads one line from a FD
int readline_simple(int fd, char* buf);

// Reads one line from a FD, stores the rest in a buffer.
int readline(fdStruct& fd, char* buf);

// Parse the message according to the protocol
bool parse(msg* m, char* l);

// Compare and validate the protocol message
bool cmpvalid(msg* m, char* l, char v_typ, int v_code);

// Wait for the right message to be received
void wait_for_message(fdStruct& fd, char* l, msg* m, char v_typ, int v_code);

// Send parsed response
void send_response(int fd, char type, int code, char const* text);

// Barber function
void* holic(void*arg = NULL);

// Function to serve customers
void* handleCustomer(void*);

#endif