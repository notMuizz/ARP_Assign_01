#include "./../include/command_utilities.h"
#include <fcntl.h> 
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <signal.h>
#include <time.h>

#define ON 1
#define OFF 0

int flag_reset=OFF,cmd;
FILE* logfile;
time_t curtime;

//file descriptors
int fd_mx, fd_mz, fd_cmd_insp,fd_pid_cmd_mx,fd_master_cmd;

void controller(int function, int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"Command ERROR: %s. Line: %d\n",strerror(errno),line);
        fflush(stderr);
        fprintf(logfile,"TIME: %s PID: %d COMMAND PROCESS TERMINATED. ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),line);
        fflush(logfile);
        fclose(logfile);
        kill(getppid(),SIGTERM);
        exit(EXIT_FAILURE);
    }
}

void stop_function (int sig){
    
    time(&curtime);

    if(sig==SIGUSR1){
        flag_reset=OFF;
        fprintf(logfile,"TIME: %s Stop of the system\n",ctime(&curtime));
        fflush(logfile);
    }
    else if(sig==SIGUSR2){
        flag_reset=ON;
        fprintf(logfile,"TIME: %s System Reset. Command disabled\n",ctime(&curtime));
        fflush(logfile);
    }
    
    else if(sig==SIGTERM){  

        close(fd_mx);
        close(fd_mz);
        fclose(logfile);
        
        //kill(getppid(),SIGTERM);

        //kill(getpid(),SIGTERM);  

        exit(EXIT_SUCCESS);
    }
    else if(sig==SIGINT)
        usleep(0.000001);
}


int main(int argc, char const *argv[]){
    
    time(&curtime);

    //opening logfile
    logfile=fopen("./logfiles/cmd_logfile","w"); 

    fprintf(logfile,"TIME: %s PID: %d Command spawned\n",ctime(&curtime),getpid());
    fflush(logfile);

    //pipes
    char *cmd_mx="/tmp/cmd_mx";
    char *cmd_mz="/tmp/cmd_mz";
    char *cmd_insp="/tmp/cmd_insp";
    char *pid_cmd_mx="/tmp/pid_cmd_mx";
    char *master_cmd="/tmp/master_cmd";

    //used to command motors
    int up_x=1, down_x=2, stop_x=3, up_z=4, down_z=5, stop_z=6;

    //pids useful
    pid_t pid_cmd,pid_wd;

    //using sigaction to handle SIGUSR1 (STOP), SIGUSR2 (RESET) and SIGINT (to handle Crtl+C)
    struct sigaction stop;

    memset(&stop,0,sizeof(stop));
    //sigemptyset(&stop_reset.sa_mask);
    stop.sa_flags=SA_RESTART;
    stop.sa_handler=&stop_function;
    controller(sigaction(SIGUSR1,&stop,NULL),__LINE__);
    controller(sigaction(SIGUSR2,&stop,NULL),__LINE__);
    controller(sigaction(SIGINT,&stop,NULL),__LINE__);
    controller(sigaction(SIGTERM,&stop,NULL),__LINE__);

    pid_cmd=getpid();  //command pid

    pid_wd=atoi(argv[1]); //watchdog pid

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    //starting opening file descriptors

    controller(fd_mx=open(cmd_mx,O_WRONLY),__LINE__); // opened pipe CMD TO MX to send command to motor x
        
    controller(fd_mz=open(cmd_mz,O_WRONLY),__LINE__); // opened pipe CMD TO MZ to send command to motor z
        
    controller(fd_cmd_insp=open(cmd_insp,O_WRONLY),__LINE__); // opened pipe CMD TO INSPECTION to send the pid of cmd to inspection

    controller(fd_pid_cmd_mx=open(pid_cmd_mx,O_WRONLY),__LINE__); // opened pipe CMD TO MX to send the pid of cmd to motor x

    controller(fd_master_cmd=open(master_cmd,O_WRONLY),__LINE__); // opened pipe MASTER_CMD to send the pid of cmd to master

    //ending opening file descriptors

    //starting sending pid of cmd to inspection and to motor x
        
    controller(write(fd_cmd_insp,&pid_cmd, sizeof(pid_cmd)),__LINE__); // send pid cmd to inspection
      
    controller(write(fd_pid_cmd_mx,&pid_cmd, sizeof(pid_cmd)),__LINE__); // send pid cmd to mx

    controller(write(fd_master_cmd,&pid_cmd,sizeof(pid_cmd)),__LINE__);  // send pid of cmd to master
    
    //ending sending pid of cmd to inspection and to motor x

    //close file descriptors
    close(fd_cmd_insp);
    close(fd_pid_cmd_mx);
    close(fd_master_cmd);
    // Initialize User Interface 
    init_console_ui();


    while(1){

        // Get mouse/resize commands in non-blocking mode...
        
        cmd = getch();
        // If user resizes screen, re-draw UI
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }
        if(flag_reset==OFF){
                if(cmd == KEY_MOUSE) {//else

                // Check which button has been pressed...
                if(getmouse(&event) == OK) {

                    // Vx-- button pressed
                    if(check_button_pressed(vx_decr_btn, &event)) {
                        
                        controller(write(fd_mx,&down_x, sizeof(int)),__LINE__); //write Vx-- to motor x
                        
                        controller(kill(pid_wd,SIGUSR2),__LINE__);  //in order to start and reset watchdog

                        time(&curtime);
                        fprintf(logfile,"TIME: %s Decreasing velocity x button pressed\n",ctime(&curtime));
                        fflush(logfile); 
                    }

                    // Vx++ button pressed
                    else if(check_button_pressed(vx_incr_btn, &event)) {
                        
                        controller(write(fd_mx,&up_x, sizeof(int)),__LINE__); // write Vx++ to motor x
                        
                        controller(kill(pid_wd,SIGUSR2),__LINE__);  //in order to start and reset watchdog 

                        time(&curtime);
                        fprintf(logfile,"TIME: %s Increasing velocity x button pressed\n",ctime(&curtime));
                        fflush(logfile); 
                    }

                    // Vx stop button pressed
                    else if(check_button_pressed(vx_stp_button, &event)) {
                        
                        controller(write(fd_mx,&stop_x, sizeof(int)),__LINE__); //write Vx stop to motor x

                        controller(kill(pid_wd,SIGUSR2),__LINE__);  //in order to start and reset watchdog

                        time(&curtime);
                        fprintf(logfile,"TIME: %s Stop velocity x button pressed\n",ctime(&curtime));
                        fflush(logfile); 
                    }

                    // Vz-- button pressed
                    else if(check_button_pressed(vz_decr_btn, &event)) {

                        controller(write(fd_mz,&down_z, sizeof(int)),__LINE__); //write Vz-- to motor z

                        controller(kill(pid_wd,SIGUSR2),__LINE__);  //in order to start and reset watchdog

                        time(&curtime);
                        fprintf(logfile,"TIME: %s Decreasing velocity z button pressed\n",ctime(&curtime));
                        fflush(logfile); 
                    }

                    // Vz++ button pressed
                    else if(check_button_pressed(vz_incr_btn, &event)) {

                        controller(write(fd_mz,&up_z, sizeof(int)),__LINE__); //write Vz++ to motor z

                        controller(kill(pid_wd,SIGUSR2),__LINE__);  //in order to start and reset watchdog

                        time(&curtime);
                        fprintf(logfile,"TIME: %s Increasing velocity z button pressed\n",ctime(&curtime));
                        fflush(logfile); 
                    }

                    // Vz stop button pressed
                    else if(check_button_pressed(vz_stp_button, &event)) {

                        controller(write(fd_mz,&stop_z, sizeof(int)),__LINE__); //write Vz stop to motor z

                        controller(kill(pid_wd,SIGUSR2),__LINE__);  //in order to start and reset watchdog

                        time(&curtime);
                        fprintf(logfile,"TIME: %s Stop velocity z button pressed\n",ctime(&curtime));
                        fflush(logfile);
                        
                    }
                }
            } 
        }  
        refresh();
        
        usleep(10000);  // to reduce busy waiting
	}

    // closing file descriptors and file pointer
    endwin();

    time(&curtime);
    fprintf(logfile,"TIME: %s Command terminated\n",ctime(&curtime));
    fflush(logfile);

    close(fd_mx);
    close(fd_mz);
    fclose(logfile);

    return 0;
}
