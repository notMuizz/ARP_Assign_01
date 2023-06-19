#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

pid_t pid_insp;
int count=0;
FILE* logfile;
time_t curtime;

void controller(int function, int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"ERROR: %s. Line: %d\n",strerror(errno),line);
        fflush(stderr);
        fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),line);
        fflush(logfile);
        fclose(logfile);
        exit(EXIT_FAILURE);
    }
}

void check_function(int sig){

    time(&curtime);

    if(sig==SIGALRM){ // any process does anything

        controller(kill(pid_insp,SIGUSR2),__LINE__);  //reset
        
        fprintf(logfile,"TIME: %s System reset because no action has been noted in 60 seconds\n",ctime(&curtime));
        fflush(logfile);

        alarm(60); //restart the timer 

        fprintf(logfile,"TIME: %s Timer restarted after system reset\n",ctime(&curtime));
        fflush(logfile);
    }  

    else if(sig==SIGUSR2){ // some process does something

        alarm(60); // start a timer
        
        if(count==0){
            fprintf(logfile,"TIME: %s Timer started because a command has been noted\n",ctime(&curtime));
            fflush(logfile);
            count++;
        }   
        else{
            fprintf(logfile,"TIME: %s Timer restarted because a command has been noted\n",ctime(&curtime));
            fflush(logfile);
        }
    }

    else if(sig==SIGTERM){

        fclose(logfile);
        
        exit(EXIT_SUCCESS);
    }
}

int main(int argc,char const *argv[]){

    time(&curtime);
    //opening logfile
    logfile=fopen("./logfiles/watchdog_logfile","w");

    fprintf(logfile,"TIME: %s PID: %d Watchdog spawned\n",ctime(&curtime),getpid());
    fflush(logfile);
    
    //pipe
    char *insp_wd="/tmp/insp_wd";

    //file descriptor
    int fd_pid_insp;

    //signal handler to handle SIGUSR2 (TO RESET ALARM), SIGALRM (TO RESET THE SYSTEM) and SIGTERM
    struct sigaction check;

    memset(&check,0,sizeof(check));
    check.sa_handler=&check_function;
    check.sa_flags=SA_RESTART;
    controller(sigaction(SIGUSR2,&check,NULL),__LINE__);
    controller(sigaction(SIGALRM,&check,NULL),__LINE__);
    controller(sigaction(SIGTERM,&check,NULL),__LINE__);

    //start opening pipe

    controller(fd_pid_insp=open(insp_wd,O_RDONLY),__LINE__); //pipe inspection to watchdog

    //end opening pipe

    //start reading

    controller(read(fd_pid_insp,&pid_insp,sizeof(pid_t)),__LINE__);  //read from inspection to get the pid of inspection

    //stop reading   

    //closing file descriptor
    close(fd_pid_insp);

    while(1){  // working

        sleep(1);
    }

    //closing file pointer
    time(&curtime);
    fprintf(logfile,"TIME: %s Watchdog terminated\n",ctime(&curtime));
    fflush(logfile); 

    fclose(logfile);

    return 0;
}