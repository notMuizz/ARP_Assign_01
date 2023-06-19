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
#include <signal.h>
#include <time.h>

#define MIN_X 0.00
#define MAX_X 39.00
#define DT 0.02      //0.03
#define MAX_SPEED 3
#define MIN_SPEED -3
#define ON 1
#define OFF 0

int speed=0,command=0,reset=OFF,stop=OFF,count;
float x=MIN_X,z;
FILE* logfile;
time_t curtime;

//file descriptor
int fd_cmd_mx, fd_mx_worldxz, fd_pid_cmd,fd_mx_mz;

void controller(int function, int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"Motor x ERROR: %s. Line: %d\n",strerror(errno),line);
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

        close(fd_cmd_mx);
        close(fd_mx_worldxz);
        close(fd_mx_mz);
        fclose(logfile);
        
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char const *argv[]){
    
    time(&curtime);
    //opening logfile
    logfile=fopen("./logfiles/mx_logfile","w");

    fprintf(logfile,"TIME: %s PID: %d Motor x spawned\n",ctime(&curtime),getpid());
    fflush(logfile);

    //pipes
    char *cmd_mx="/tmp/cmd_mx";
    char *mx_worldxz="/tmp/mx_worldxz";
    char *pid_cmd_mx="/tmp/pid_cmd_mx";
    char *mx_mz="/tmp/mx_mz";

    //pids useful
    pid_t pid_cmd,pid_wd;

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

    //getting watchdog pid
    pid_wd=atoi(argv[1]);
    
    //start opening pipes

    controller((fd_cmd_mx=open(cmd_mx,O_RDONLY)),__LINE__);  //opened the pipe CMD TO MX to get command values
        
    controller((fd_mx_worldxz=open(mx_worldxz,O_WRONLY)),__LINE__);  //opened the pipe MX TO WORLD_XZ to send x-axis position
        
    controller((fd_pid_cmd=open(pid_cmd_mx,O_RDONLY)),__LINE__);  //opened the pipe CMD TO MX to get pid of cmd

    controller((fd_mx_mz=open(mx_mz,O_RDONLY)),__LINE__);    //opened the pipe MZ TO MX

    //end opening pipes

    //start reading

    controller(read(fd_pid_cmd,&pid_cmd,sizeof(pid_cmd)),__LINE__);  //read pid cmd from cmd
    
    //end reading

    //closing file descriptors
    close(fd_pid_cmd);

    while(1){
        //setting select
        FD_ZERO(&readset);
        FD_SET(fd_cmd_mx,&readset);
        timeout.tv_sec=0;
        timeout.tv_usec=0;

        select_val=select(FD_SETSIZE,&readset,NULL,NULL,&timeout);

       if(select_val < 0 && errno!=EINTR){
            fprintf(stderr,"Motor x ERROR: %s. Line: %d\n",strerror(errno),__LINE__);
            fflush(stderr);
            fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),__LINE__);
            fflush(logfile);
            fclose(logfile);
            exit(EXIT_FAILURE);
       }

        if(select_val)
            if(FD_ISSET(fd_cmd_mx,&readset))
                controller(read(fd_cmd_mx,&command,sizeof(int)),__LINE__);  //read from the cmd 

        controller(read(fd_mx_mz,&z,sizeof(float)),__LINE__); //no need to use a select because a value will be always sent by motor z
        
        time(&curtime);
        if(reset==OFF){  // non reset
            switch(command){
                case 1:
                    speed+=1;

                    fprintf(logfile,"TIME: %s Increase velocity command received\n",ctime(&curtime));
                    fflush(logfile);

                    if(speed > MAX_SPEED)
                        speed = MAX_SPEED;
                    break;
                case 2:
                    speed-=1;

                    fprintf(logfile,"TIME: %s Decrease velocity command received\n",ctime(&curtime));
                    fflush(logfile);

                    if(speed < MIN_SPEED)
                        speed = MIN_SPEED;
                    break;
                case 3:
                    speed=0;

                    fprintf(logfile,"TIME: %s Stop velocity command received\n",ctime(&curtime));
                    fflush(logfile);

                    break;
            }
            //command=0; 
        }
        
        else{  // si reset

            if(x==MIN_X && z>0.00){
                   speed=0;
               }
            else{
                if(x>MIN_X && stop==OFF){
                    speed=-3; 
                }
                else{
                    time(&curtime);
                    reset=OFF;
                    command=3;
                    controller(kill(pid_cmd,SIGUSR1),__LINE__);

                    fprintf(logfile,"TIME: %s System Reset terminated\n",ctime(&curtime));
                    fflush(logfile);
                
                    speed=0; 
                }

                stop=OFF;
            }
        }    
        
        x+=speed*DT; //DT in seconds
        
        if(x>MAX_X){
            //command=3;
            speed=0;
            x=MAX_X;
            }
        else if(x<MIN_X){
            //command=3;
            speed=0;
            x=MIN_X;
            }

            
        command=0;

       // printf("x: %.2f and speed: %d\n",x,speed); //TODO: DA ELIMINARE

        //start writing

        time(&curtime);

        controller(write(fd_mx_worldxz,&x,sizeof(x)),__LINE__);

        fprintf(logfile,"TIME: %s Position computed: %f\n",ctime(&curtime),x);
        fflush(logfile);

        //stop writing

        usleep(DT * 1000000); //in microseconds
    }
    
    //closing file descriptors and file pointer
    time(&curtime);
    fprintf(logfile,"TIME: %s Motor x terminated\n",ctime(&curtime));
    fflush(logfile);

    close(fd_cmd_mx);
    close(fd_mx_worldxz);
    close(fd_mx_mz);
    fclose(logfile);

    return 0; 
}