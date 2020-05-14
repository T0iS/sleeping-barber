#include "holic.h"


int debug = LOG_INFO;
shared_memory* global_data = NULL;
sem_t* sem_customers = NULL;
sem_t* sem_cutting = NULL;
sem_t* sem_barber = NULL;
int comPipe[2];

fdStruct fd_stdin;
fdStruct fd_pipe;
fdStruct fd_sock_client;




void log_msg( int log_level, const char *form, ... )
{
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( log_level && log_level > debug ) return;

    char buf[ 1024 ];
    va_list arg;
    va_start( arg, form );
    vsprintf( buf, form, arg );
    va_end( arg );

    switch ( log_level )
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf( stdout, out_fmt[ log_level ], buf );
        break;

    case LOG_ERROR:
        fprintf( stderr, out_fmt[ log_level ], errno, strerror( errno ), buf );
        break;
    }
}

void help( int num, char **arg )
{
    if ( num <= 1 ) return;

    if ( !strcmp( arg[ 1 ], "-h" ) )
    {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", arg[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( arg[ 1 ], "-d" ) )
        debug = LOG_DEBUG;
}


int readline_simple(int fd, char* buf){

    int k = 0;
    int ret;
    while(true){
        char txt;
        ret = read(fd,&txt,1);
        if (ret<=0) return ret;

        if(txt == '\n') {
            buf[k++] = '\n';
            buf[k++] = '\0';
            return k;
        }
        else if ( txt == '\0'){
            buf[k++] = '\0';
            return k;
        }
        else{
            buf[k++] = txt;
        }
    }
}



int readline(fdStruct& fd, char* buf){

    int ret, mv;
    char temp[256];
    bzero(temp, sizeof(temp));
    char* t = NULL;

    while(1){
        bzero(temp, sizeof(temp));
        bzero(buf,sizeof(buf));

        ret = read(fd.fd, &temp, 256 - sizeof(strlen(fd.BUF)));
        if (ret > 0 ){
            temp[ret] = '\0';
        }

        strcat(fd.BUF, temp);
        t = strchr(fd.BUF,'\n');

        if (t != NULL){
            mv = t-fd.BUF+1;
            strncpy(buf,fd.BUF, mv);
            strcpy(fd.BUF,fd.BUF+mv);
            return strlen(buf);
        }
        if (ret <= 0) {

            if (strlen(fd.BUF) == 0){
                return ret;
            }
            strcpy(buf, fd.BUF);
            strcpy(fd.BUF,fd.BUF+strlen(buf));
            return strlen(buf);
        }
    }
}  


bool parse(msg* m, char* l){
    bzero(m->text,sizeof(char)*252);
    return (sscanf(l,"%c%2d:%252[^\n]s",&m->typ,&m->NN,(char*)&m->text))==3;
    
            
}

bool cmpvalid(msg* m, char* l, char v_typ, int v_code){

    
    if(m->typ == v_typ && m->NN == v_code){
        return true;
    }
    else return false;
}


void wait_for_message(fdStruct& fd, char* l, msg* m, char v_typ, int v_code){
    
    int ret = -1;
    while(1){

        if ((ret = readline(fd, l)) < 0){
            log_msg(LOG_INFO, "ZADNA PRICHOZI ZPRAVA");
            exit(-1);
        }
        if(parse(m, l)){
            //log_msg(LOG_INFO, "%s\n%d\n%s\n", m->typ, m->NN, m->text);
                if(cmpvalid(m, l, v_typ, v_code)){
                    log_msg(LOG_INFO, "[GOOD] %s", l);
                    return;
                }
                else{
                    log_msg(LOG_INFO, "[BAD or unexpected format] %s\n", l);
                    bzero(l, sizeof(char)*256);  
                }
        }
        else{
          log_msg(LOG_INFO, "[BAD or unexpected format] %s\n", l);
        }
    }
}

void send_response(int fd, char v_typ, int v_code, const char* v_text){

    char buffer[256];
    sprintf(buffer, "%c%d:%s\n", v_typ, v_code, v_text);
    write(fd, buffer, strlen(buffer));
}



void* holic(void* arg){

    if(global_data->child_count>0){
        close(comPipe[1]);
    }
    log_msg(LOG_INFO, "Holic prichazi do prace..");

    msg m;
    char l[256];
    int val_check = -1;
    int cut_time = 0;

    global_data->customer_count = 0;


    while(1){

        sem_getvalue(sem_customers, &val_check);
        if (val_check==0){
            log_msg(LOG_INFO,"Holic usina..");
        }

        sem_wait(sem_customers);

        log_msg(LOG_INFO, "Holic probuzen, jde strihat..");
        global_data->customer_count -= 1;

        //Pojdte se strihat...
        sem_post(sem_barber);

        
        bzero(l,sizeof(l));
        bzero(m.text, sizeof(m.text));
        log_msg(LOG_INFO, "barber before read %d", fd_pipe.fd);
        
        int tmp = read(comPipe[0], l, sizeof(l));
        l[tmp] = '\0';
        //wait_for_message(fd_pipe,l,&m,'A', AI_Zakazka);


        sscanf(l, "Chci strihat po dobu %d", &cut_time);
        global_data->chairs[global_data->last_chair] = 0;
        log_msg(LOG_INFO, "Holic striha zakaznika z zidle %d, potrva to %d vterin.",global_data->last_chair, cut_time);
        

        sleep(cut_time);
        sem_post(sem_cutting);

        bzero(l,sizeof(l));
        bzero(m.text, sizeof(m.text));
        // Nashledanou
        wait_for_message(fd_pipe,l,&m,'A', AI_Nashledanou);
        log_msg(LOG_INFO, "Nashledanou, prijdte zas.");

    }
    if (global_data->child_count>0){
        close(comPipe[0]);
    }
}




void* handleCustomer(void* var){

    if (global_data->child_count>0){
        close(comPipe[0]);
    }

    fdStruct sock_client = *(fdStruct*) var;
    
    msg m;
    char l[256];
    bzero(m.text, sizeof(m.text));
    bzero(l, sizeof(l));


    // Vstup zakaznika
    wait_for_message( sock_client, l, &m, 'C', CI_Vstup);

        
    if (global_data->customer_count < CHAIR_COUNT){

        global_data->customer_count += 1;
        int i = 0;
        for(i; i<CHAIR_COUNT; i++){

            if(global_data->chairs[i] == 0){
                global_data->chairs[i] = 1;
                
                break;
            }
        }

        // Volne misto
        sprintf(l, AS_MistoX, i);
        send_response(sock_client.fd, 'A', AI_MistoX, l);
        log_msg(LOG_INFO, "%s", l );
        sem_post(sem_customers);
        sem_wait(sem_barber);
        global_data->last_chair = i;

        // Pojdte se strihat
        send_response(sock_client.fd, 'C', CI_Strihat, CS_Strihat);
        
        
        // Dekka strihani
        wait_for_message(sock_client, l, &m, 'A', AI_Zakazka);
        parse(&m, l);
        write(comPipe[1], &m.text, strlen(m.text));
      

        bzero(m.text, sizeof(m.text));
        bzero(l, sizeof(l));

        sem_wait(sem_cutting);
        // Dostrihano, muzete odejit.
        send_response(sock_client.fd, 'C', CI_Hotovo, CS_Hotovo);
 
        // Nashledanou
        wait_for_message(sock_client, l, &m, 'A', 72);
        send_response(comPipe[1],'A',AI_Nashledanou,AS_Nashledanou);

        close(sock_client.fd);

    }
    else{
        send_response(sock_client.fd, 'E', EI_Obsazeno, ES_Obsazeno);
        close(sock_client.fd);
    }

    if (global_data->child_count>0){
        close(comPipe[1]);
    }
}