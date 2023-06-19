#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <errno.h>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>

#define MIN_Z 0.00
#define MAX_Z 9.00
#define DT 0.02
#define MAX_SPEED 3
#define MIN_SPEED -3
#define ON 1
#define OFF 0

int speed=0,command,reset=OFF,stop=OFF;
float z=MIN_Z;
FILE* logfile;
time_t curtime;

// file descriptors
int fd_cmd_mz, fd_mz_worldxz,fd_mx_mz;

void controller(int function,int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"Motor z ERROR: %s. Line: %d\n",strerror(errno),line);
        fflush(stderr);
        fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),line);
        fflush(logfile);
        fclose(logfile);
        exit(EXIT_FAILURE);
    }
}


void stop_reset_function(int sig){

    time(&curtime);
    
    if(sig==SIGUSR1){
        speed=0;
        stop=ON;

        fprintf(logfile,"TIME: %s Stop of the system\n",ctime(&curtime));
        fflush(logfile);

    }
    else if(sig==SIGUSR2){
        stop=OFF;
        reset=ON;

        fprintf(logfile,"TIME: %s System Reset\n",ctime(&curtime));
        fflush(logfile);
        
    }
    else if(sig==SIGTERM){

        close(fd_cmd_mz);
        close(fd_mz_worldxz);
        close(fd_mx_mz);
        fclose(logfile);
        
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char const *argv[]){
    
    time(&curtime);
    //opening logfile
    logfile=fopen("./logfiles/mz_logfile","w");

    fprintf(logfile,"TIME: %s PID: %d Motor z spawned\n",ctime(&curtime),getpid());
    fflush(logfile);

    //pipes
    char *cmd_mz="/tmp/cmd_mz";
    char *mz_worldxz="/tmp/mz_worldxz";
    char*pid_cmd_mz="/tmp/pid_cmd_mz";
    char *mx_mz="/tmp/mx_mz";

    //pid useful
    pid_t pid_wd,pid_mx;

    //parameters for select
    int select_val;
    fd_set readset;
    struct timeval timeout;

    //signal handler to handle SIGUSR1 (STOP), SIGUSR2 (RESET) and SIGTERM (TERMINATE)
    struct sigaction stop_reset;

    memset(&stop_reset,0,sizeof(stop_reset));
    //sigemptyset(&stop_reset.sa_mask);
    stop_reset.sa_flags=SA_RESTART;
    stop_reset.sa_handler=&stop_reset_function;
    controller(sigaction(SIGUSR1,&stop_reset,NULL),__LINE__);
    controller(sigaction(SIGUSR2,&stop_reset,NULL),__LINE__);
    controller(sigaction(SIGTERM,&stop_reset,NULL),__LINE__);

    // getting watchdog pid and motor x pid
    pid_wd=atoi(argv[1]);
    pid_mx=atoi(argv[2]);

    //start opening pipes

    controller((fd_cmd_mz=open(cmd_mz,O_RDONLY)),__LINE__); //opened the pipe CMD TO MZ
        
    controller((fd_mz_worldxz=open(mz_worldxz,O_WRONLY)),__LINE__);  //opened the pipe MZ TO WORLD_XZ

    controller((fd_mx_mz=open(mx_mz,O_WRONLY)),__LINE__);  //opened the pipe MZ TO MX

    //end opening pipes   

    while(1){

        //setting parameters
        FD_ZERO(&readset);
        FD_SET(fd_cmd_mz,&readset);
        timeout.tv_sec=0;
        timeout.tv_usec=0;

        select_val=select(FD_SETSIZE, &readset, NULL, NULL, &timeout);

        if(select_val < 0 && errno!=EINTR){
            fprintf(stderr,"Motor z ERROR: %s. Line: %d\n",strerror(errno),__LINE__);
            fflush(stderr);
            fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),__LINE__);
            fflush(logfile);
            fclose(logfile);
            exit(EXIT_FAILURE);
       }

        if(select_val)
            if(FD_ISSET(fd_cmd_mz,&readset))
                controller(read(fd_cmd_mz,&command,sizeof(int)),__LINE__); //read from the cmd

        time(&curtime);
        if(reset==OFF){
            switch(command){
                case 4:
                    speed+=1;

                    fprintf(logfile,"TIME: %s Increase velocity command received\n",ctime(&curtime));
                    fflush(logfile);

                    if(speed > MAX_SPEED)
                        speed = MAX_SPEED;
                    break;
                case 5:
                    speed-=1;

                    fprintf(logfile,"TIME: %s Decrease velocity command received\n",ctime(&curtime));
                    fflush(logfile);

                    if(speed < MIN_SPEED)
                        speed = MIN_SPEED;
                    break;
                case 6:
                    speed=0;

                    fprintf(logfile,"TIME: %s Stop velocity command received\n",ctime(&curtime));
                    fflush(logfile);

                    break;
            }
            
        }
        else{
            if(z>MIN_Z && stop==OFF)
                speed=-3;
            else{
                time(&curtime);
                reset=OFF;

                fprintf(logfile,"TIME: %s System Reset terminated\n",ctime(&curtime));
                fflush(logfile);

                speed=0;
            }
            stop=OFF;
        }

        z+=speed*DT; //DT in seconds

        if(z<MIN_Z){
            speed=0;
            z=MIN_Z;
        }
        else if(z>MAX_Z){
            speed=0;
            z=MAX_Z;
        }

        command=0;
        
        //printf("z: %.2f and speed: %d\n",z,speed); //TODO: DA ELIMINARE

        time(&curtime);

        controller(write(fd_mz_worldxz,&z,sizeof(z)),__LINE__);  //write to worldxz z-axis position
        
        fprintf(logfile,"TIME: %s Position computed: %f\n",ctime(&curtime),z);
        fflush(logfile);

        controller(write(fd_mx_mz,&z,sizeof(z)),__LINE__);

        usleep(DT*1000000); //in microseconds
    }

    //closing file descriptor and file pointer
    time(&curtime);
    fprintf(logfile,"TIME: %s Motor z terminated\n",ctime(&curtime));
    fflush(logfile);

    close(fd_cmd_mz);
    close(fd_mz_worldxz);
    close(fd_mx_mz);
    fclose(logfile);

    return 0;  
}