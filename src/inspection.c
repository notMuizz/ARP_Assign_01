#include "./../include/inspection_utilities.h"
#include <fcntl.h> 
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <signal.h>
#include <time.h>

pid_t pid_wd,pid_mx, pid_mz,pid_cmd;
FILE* logfile;
time_t curtime;

//file descriptor
int fd_worldxz_insp, fd_cmd_insp,fd_insp_wd,fd_master_insp;

void controller(int function, int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"Inspection ERROR: %s. Line: %d\n",strerror(errno),line);
        fflush(stderr);
        fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),line);
        fflush(logfile);
        fclose(logfile);
        kill(getppid(),SIGTERM);
        exit(EXIT_FAILURE);
    }
}

void sa_function(int sig){

    time(&curtime);

    if(sig==SIGUSR2){

        controller(kill(pid_mx,SIGUSR2),__LINE__);

        controller(kill(pid_mz,SIGUSR2),__LINE__);

        controller(kill(pid_cmd,SIGUSR2),__LINE__);

        fprintf(logfile,"TIME: %s System reset requested by the watchdog\n",ctime(&curtime));
        fflush(logfile);
    }

    else if(sig==SIGTERM){  //TODO: CONTROLLI

        close(fd_worldxz_insp);
        fclose(logfile);

        //kill(getppid(),SIGTERM);

        exit(EXIT_SUCCESS);
    }
    else if(sig==SIGINT)
        usleep(0.000001);
}

int main(int argc, char const *argv[]){

    time(&curtime);

    //opening logfile
    logfile=fopen("./logfiles/inspection_logfile","w"); 

    fprintf(logfile,"TIME: %s PID: %d Command spawned\n",ctime(&curtime),getpid());

    //pipes
    char* worldxz_insp="/tmp/worldxz_insp";
    char *cmd_insp="/tmp/cmd_insp";
    char *insp_wd="/tmp/insp_wd";
    char *master_insp="/tmp/master_insp";

    //useful pid
    pid_t pid_insp;

    //where putting position values
    float real_value[2];

    //element for select
    fd_set readset;
    struct timeval timeout;
    int select_val;

    //signal handler to handle SIGUSR2 (RESET), SIGINT (to handle Ctrl+C) and SIGTERM
    struct sigaction sa;

    memset(&sa,0,sizeof(sa));
    sa.sa_handler=&sa_function;
    sa.sa_flags=SA_RESTART;
    controller(sigaction(SIGUSR2,&sa,NULL),__LINE__);
    controller(sigaction(SIGINT,&sa,NULL),__LINE__);
    controller(sigaction(SIGTERM,&sa,NULL),__LINE__);
    
    //pids useful
    pid_insp=getpid();
    pid_mx=atoi(argv[1]);
    pid_mz=atoi(argv[2]);
    pid_wd=atoi(argv[3]);
    
    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // End-effector coordinates
    float ee_x, ee_z;
    
    //start opening pipes

    controller(fd_worldxz_insp=open(worldxz_insp,O_RDONLY),__LINE__); // open pipe WORLDXZ TO INSP to get position values
        
    controller(fd_cmd_insp=open(cmd_insp,O_RDONLY),__LINE__); // open pipe CMD TO INSP in order to get the pid of cmd

    controller(fd_insp_wd=open(insp_wd,O_WRONLY),__LINE__); // open pipe INSPECTION TO WATCHDOG in order to send the pid of inspection

    controller(fd_master_insp=open(master_insp,O_WRONLY),__LINE__); // open pipe MASTER_INSPECTION to send the pid of inspection
    
    //ending opening file descriptors

    //start reading

    controller(read(fd_cmd_insp,&pid_cmd,sizeof(pid_t)),__LINE__);  //read from cmd to get pid of cmd
    
    //end reading

    //start writing
    
    controller(write(fd_insp_wd,&pid_insp,sizeof(pid_insp)),__LINE__); //write to watchdog to send the pid of inspection

    controller(write(fd_master_insp,&pid_insp,sizeof(pid_insp)),__LINE__); //write to master to send the pid of inspection
    
    //end writing    

    //closing file descriptors
    close(fd_cmd_insp);
    close(fd_insp_wd);
    close(fd_master_insp);

    // Initialize User Interface 
    init_console_ui();

    while(1){

        // Get mouse/resize commands in non-blocking mode...
        int cmd = getch();

        // If user resizes screen, re-draw UI
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }
        // Else if mouse has been pressed
        else if(cmd == KEY_MOUSE) {

            // Check which button has been pressed...
            if(getmouse(&event) == OK) {

                // STOP button pressed
                if(check_button_pressed(stp_button, &event)) {
                    
                    time(&curtime);

                    controller(kill(pid_mx,SIGUSR1),__LINE__);  //sending stop to motor x
                    
                    controller(kill(pid_mz,SIGUSR1),__LINE__);  //sending stop to motor z

                    controller(kill(pid_cmd,SIGUSR1),__LINE__); //sending stop to cmd

                    controller(kill(pid_wd,SIGUSR2),__LINE__); //reset the timer of the watchdog

                    fprintf(logfile,"TIME: %s Stop button pressed\n",ctime(&curtime));
                    fflush(logfile);
                }
                // RESET button pressed
                else if(check_button_pressed(rst_button, &event)) {
                    
                    time(&curtime);

                    controller(kill(pid_mx,SIGUSR2),__LINE__);  //sending reset to motor x

                    controller(kill(pid_mz,SIGUSR2),__LINE__);  //sending reset to motor z

                    controller(kill(pid_cmd,SIGUSR2),__LINE__); //sending reset to cmd

                    fprintf(logfile,"TIME: %s Reset button pressed\n",ctime(&curtime));
                    fflush(logfile);
                }
            }
        }
        
        //setting parameters for select
        FD_ZERO(&readset);
        FD_SET(fd_worldxz_insp,&readset);
        timeout.tv_sec=0;
        timeout.tv_usec=10000;
        
        select_val=select(FD_SETSIZE,&readset,NULL,NULL,&timeout);

        if(select_val < 0 && errno!=EINTR){
            fprintf(stderr,"Inspection ERROR: %s. Line: %d\n",strerror(errno),__LINE__);
            fflush(stderr);
            fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),__LINE__);
            fflush(logfile);
            fclose(logfile);
            exit(EXIT_FAILURE);
       }
        
        if(select_val)
            if(FD_ISSET(fd_worldxz_insp,&readset)){
                time(&curtime);
                controller(read(fd_worldxz_insp,&real_value,2*sizeof(float)),__LINE__);  //read from the worldxz to get position values
                
                fprintf(logfile,"TIME: %s Position values received. X: %f Z: %f\n",ctime(&curtime),real_value[0],real_value[1]);
                fflush(logfile);
            }
        //position values
        ee_x=real_value[0];
        ee_z=real_value[1];

        update_console_ui(&ee_x, &ee_z);

        //usleep(10000);
    }

    //closing file descriptor and file pointer
    endwin();

    time(&curtime);
    fprintf(logfile,"TIME: %s Inspection terminated\n",ctime(&curtime));
    fflush(logfile);

    close(fd_worldxz_insp);
    fclose(logfile);
    
    return 0;
}