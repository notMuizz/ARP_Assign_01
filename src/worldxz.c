#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <errno.h>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#define MIN_X 0.00
#define MAX_X 39.00
#define MIN_Z 0.00
#define MAX_Z 9.00

FILE* logfile;
time_t curtime;

//file descriptors
int fd_mx_worldxz, fd_mz_worldxz, fd_worldxz_insp;

void controller(int function, int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"Worldxz ERROR: %s. Line: %d\n",strerror(errno),line);
        fflush(stderr);
        fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),line);
        fflush(logfile);
        fclose(logfile);

        sleep(5);
        exit(EXIT_FAILURE);
    }
}

void sa_function(int sig){
    
    close(fd_mx_worldxz);
    close(fd_mz_worldxz);
    close(fd_worldxz_insp);
    fclose(logfile);
    
    exit(EXIT_SUCCESS);
    
}

int main(int argc, char const *argv[]){

    time(&curtime);
    //opening file
    logfile=fopen("./logfiles/worldxz_logfile\n","w");

    fprintf(logfile,"TIME: %s PID: %d Worldxz spawned\n",ctime(&curtime),getpid());
    fflush(logfile);

    //pipes
    char *mx_worldxz="/tmp/mx_worldxz";
    char *mz_worldxz="/tmp/mz_worldxz";
    char *worldxz_insp="/tmp/worldxz_insp";

    //signal handler to handle SIGTERM (TERMINATE)
    struct sigaction sa;

    memset(&sa,0,sizeof(sa));
    sa.sa_handler=&sa_function;
    sa.sa_flags=SA_RESTART;
    controller(sigaction(SIGTERM,&sa,NULL),__LINE__);

    //error to get a real value
    int err;

    float x,z, real_value[2];

    //parameters of select
    int select_val;
    fd_set readfds;
    struct timeval timeout;

    //to get a different order of numbers when the error is computed
    srand(time(NULL));
    
    //start opening pipes

    controller((fd_mx_worldxz=open(mx_worldxz,O_RDONLY)),__LINE__);      // open pipe MX TO WORLDXZ  to get x-axis position
        
    controller((fd_mz_worldxz=open(mz_worldxz,O_RDONLY)),__LINE__);      // open pipe MZ TO WORLDXZ to get z-axis position
        
    controller((fd_worldxz_insp=open(worldxz_insp,O_WRONLY)),__LINE__);  // open pipe WORLDXZ TO INSPECTION to send it the position values
    
    //end opening pipes

    while(1){
            
        //setting select
        FD_ZERO(&readfds);
        FD_SET(fd_mx_worldxz,&readfds);
        FD_SET(fd_mz_worldxz,&readfds);
        timeout.tv_sec=3;
        timeout.tv_usec=0;

        select_val=select(FD_SETSIZE,&readfds,NULL,NULL,&timeout);

        if(select_val < 0 && errno!=EINTR){
            fprintf(stderr,"Worldxz ERROR: %s. Line: %d\n",strerror(errno),__LINE__);
            fflush(stderr);
            fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),__LINE__);
            fflush(logfile);
            fclose(logfile);
            exit(EXIT_FAILURE);
       }

        time(&curtime);
        if(select_val>0){
            if(FD_ISSET(fd_mx_worldxz,&readfds)){
                controller(read(fd_mx_worldxz,&x,sizeof(float)),__LINE__);   //getting x position

                fprintf(logfile,"TIME: %s Position x: %f received\n",ctime(&curtime),x);
                fflush(logfile);
            }
            if(FD_ISSET(fd_mz_worldxz,&readfds)){
                controller(read(fd_mz_worldxz,&z,sizeof(float)),__LINE__);   //getting z position

                fprintf(logfile,"TIME: %s Position z: %f received\n",ctime(&curtime),z);
                fflush(logfile);
            }   //TODO: RANDOM
        }
        
        err=rand()%1; //error

        x+=x*err;   //computing the real value adding estimated position to an error
        
        if(x<=MIN_X)
            x=MIN_X;
        else if (x>=MAX_X)
            x=MAX_X;

        z+=z*err;   //computing the real value adding estimated position to an error

        if(z<=MIN_Z)
            z=MIN_Z;
        else if (z>=MAX_Z)
            z=MAX_Z;
        
        real_value[0]=x;
        real_value[1]=z;

        time(&curtime);
        controller(write(fd_worldxz_insp,&real_value,sizeof(real_value)),__LINE__);   //sending real position to inspection
        
        fprintf(logfile,"TIME: %s Real x and z value computed. Position x: %f and  position z: %f\n",ctime(&curtime),real_value[0],real_value[1]);
        fflush(logfile);

        usleep(10000);
        
    }

    //closing file descriptors and file pointer
    time(&curtime);
    fprintf(logfile,"TIME: %s Process terminated\n",ctime(&curtime));
    fflush(logfile);

    close(fd_mx_worldxz);
    close(fd_mz_worldxz);
    close(fd_worldxz_insp);
    fclose(logfile);

    sleep(2);

    return 0;
}