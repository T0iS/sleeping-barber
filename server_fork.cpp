#include "holic.h"



int main( int argn, char **arg )
{
    if ( argn <= 1 ) help( argn, arg );

    int port = 0;

    // parsing arguments
    for ( int i = 1; i < argn; i++ )
    {
        if ( !strcmp( arg[ i ], "-d" ) )
            debug = LOG_DEBUG;

        if ( !strcmp( arg[ i ], "-h" ) )
            help( argn, arg );

        if ( *arg[ i ] != '-' && !port )
        {
            port = atoi( arg[ i ] );
            break;
        }
    }
    if ( port <= 0 )
    {
        log_msg( LOG_INFO, "Bad or missing port number %d!", port );
        help( argn, arg );
    }

    log_msg( LOG_INFO, "Server will listen on port: %d.", port );

    // socket creation
    int sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sock_listen == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    in_addr addr_any = { INADDR_ANY };
    sockaddr_in srv_addr;
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons( port );
    srv_addr.sin_addr = addr_any;

    // Enable the port number reusing
    int opt = 1;
    if ( setsockopt( sock_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) ) < 0 )
      log_msg( LOG_ERROR, "Unable to set socket option!" );

    // assign port number to socket
    if ( bind( sock_listen, (const sockaddr * ) &srv_addr, sizeof( srv_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Bind failed!" );
        close( sock_listen );
        exit( 1 );
    }

    // listenig on set port
    if ( listen( sock_listen, 1 ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to listen on given port!" );
        close( sock_listen );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );



    
    pipe(comPipe);

    fd_stdin.fd = STDIN_FILENO;
    bzero(fd_stdin.BUF, sizeof(fd_stdin.BUF));

    fd_pipe.fd = comPipe[0];
    bzero(fd_pipe.BUF, sizeof(fd_pipe.BUF));



    sem_unlink(SEM_CUTTING);
    sem_unlink(SEM_BARBER);
    sem_unlink(SEM_CUSTOMERS);

    sem_barber = sem_open(SEM_BARBER, O_RDWR | O_CREAT, 0660, 0);
    sem_customers = sem_open(SEM_CUSTOMERS, O_RDWR | O_CREAT, 0660, 0);
    sem_cutting = sem_open(SEM_CUTTING, O_RDWR | O_CREAT, 0660, 0);

    if(!sem_barber || !sem_customers || !sem_cutting){
        log_msg(LOG_INFO, "Unable to create semaphores.\n");
        exit(-1);
    }


    sem_init (sem_barber, 1, 0);
    sem_init (sem_customers, 1, 0);
    sem_init (sem_cutting, 1, 0);



    int shm_check = shm_open( SHM_NAME, O_RDWR, 0660);
    if (shm_check<0){
        log_msg( LOG_ERROR, "Unable to open shared memory, creating new one.");
        shm_check = shm_open( SHM_NAME, O_RDWR | O_CREAT, 0660);
        if(shm_check < 0)
        {
            log_msg(LOG_ERROR, "Unable to create shared memory");
            exit(-1);
        }
        ftruncate( shm_check, sizeof( shared_memory ) );
    }
    global_data = (shared_memory*) mmap (NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_check, 0);
    
    if(!global_data){
        log_msg(LOG_ERROR, "Unable to allocate shared memory");
        exit(-1);
    }

    for( int i = 0; i<CHAIR_COUNT; i++){
        global_data->chairs[i] = 0;
    }
    global_data->last_chair = -1;
    global_data->child_count = 0;
    //global_data->customer_count = -1;


    if(fork() == 0){
        global_data->child_count++;
        log_msg(LOG_INFO, "Holic v provozu s PID: %d", getpid());
        holic();
    }


     // go!
    while ( 1 )
    {
        int sock_client = -1;

        while ( 1 ) // wait for new client
        {
            // set for handles
            fd_set read_wait_set;
            // empty set
            FD_ZERO( &read_wait_set );
            // add stdin
            FD_SET( STDIN_FILENO, &read_wait_set );
            // add listen socket
            FD_SET( sock_listen, &read_wait_set );

            int sel = select( sock_listen + 1, &read_wait_set, NULL, NULL, NULL );

            if ( sel < 0 )
            {
                log_msg( LOG_ERROR, "Select failed!" );
                exit( 1 );
            }

            if ( FD_ISSET( sock_listen, &read_wait_set ) )
            { // new client?
                sockaddr_in rsa;
                int rsa_size = sizeof( rsa );
                // new connection
                sock_client = accept( sock_listen, ( sockaddr * ) &rsa, ( socklen_t * ) &rsa_size );
                if ( sock_client == -1 )
                {
                        log_msg( LOG_ERROR, "Unable to accept new client." );
                        close( sock_listen );
                        exit( 1 );
                }

                uint lsa = sizeof( srv_addr );
               // client IP
                getpeername( sock_client, ( sockaddr * ) &srv_addr, &lsa );
                log_msg( LOG_INFO, "Pripojil sa zakaznik z ip: '%s'  port: %d",
                                 inet_ntoa( srv_addr.sin_addr ), ntohs( srv_addr.sin_port ) );

                //Oblsuha klienta

                fdStruct fd_sock_client;
                fd_sock_client.fd = sock_client;
                bzero(fd_sock_client.BUF,sizeof(fd_sock_client.BUF));

                if(fork() == 0) 
                {
                    close(sock_listen);
                    handleCustomer(&fd_sock_client);
		        } else {
                    //close(sock_client);
                }
                break;
            }
        } // while wait for client

        wait3((int *)-1,WNOHANG,NULL);
    } // while ( 1 )
	
    return 0;
}