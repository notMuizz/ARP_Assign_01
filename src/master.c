#include <stdio.h>
#include <string.h> 
#include <errno.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

pid_t konsole_cmd,konsole_inspection,mx,mz,worldxz,watchdog,pid_cmd,pid_insp;
int status, other_states;
char *cmd_mx="/tmp/cmd_mx";  // CMD TO MOTOR X
char *cmd_mz="/tmp/cmd_mz";  // CMD TO MOTOR Z
char *mx_worldxz="/tmp/mx_worldxz";  // MOTOR X TO WORLDXZ
char *mz_worldxz="/tmp/mz_worldxz";     // MOTOR Z TO WORLDXZ
char *worldxz_insp="/tmp/worldxz_insp";  // WORLDXZ TO INSP
char *mx_insp="/tmp/mx_insp";  // MOTOR X TO INSP
char *mz_insp="/tmp/mz_insp";   // MOTOR Z TO INSP
char *cmd_insp="/tmp/cmd_insp"; // CMD TO INSP
char *pid_cmd_mx="/tmp/pid_cmd_mx"; // CMD TO MOTOR FOR PID CMD
char *insp_wd="/tmp/insp_wd";       // INSPECTION TO WATCHDOG
char *mx_mz="/tmp/mx_mz";           // MOTOR Z TO MOTOR X
char *master_insp="/tmp/master_insp";  //INSPECTION TO MASTER
char *master_cmd="/tmp/master_cmd";    //CMD TO MASTER
char pid_mx[16],pid_mz[16],pid_wd[16];
FILE* logfile;
time_t curtime;

void controller(int function, int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"Master ERROR: %s. Line: %d\n",strerror(errno),line);
        fflush(stderr);
        fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),line);
        fflush(logfile);
        fclose(logfile);
        exit(EXIT_FAILURE);
    }
}

int fifo_maker(char *file_name){

    return mkfifo(file_name,0666);
}

int spawn(const char *program, char **arg_list){

    pid_t child = fork();

    //controller(child);
    if(child==-1)
        exit(EXIT_FAILURE);
        
    if(child!=0)
        return child;
    
    else{
        execvp(program, arg_list);
        fprintf(stderr,"Master ERROR: %s. Line: %d\n",strerror(errno),__LINE__);
        fflush(stderr);
        fprintf(logfile,"TIME: %s PID: %d ERROR: %s. Line: %d\n",ctime(&curtime),getpid(),strerror(errno),__LINE__);
        fflush(logfile);
        fclose(logfile);
        exit(EXIT_FAILURE);
        }
}

void closing_function(int sig){

    //start unlinking pipes

    time(&curtime);
    controller(unlink(cmd_mx),__LINE__); 
    fprintf(logfile,"TIME: %s Pipe cmd_mx unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(unlink(cmd_mz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe cmd_mz unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(unlink(mx_worldxz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe mx_worldxz unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(unlink(mz_worldxz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe mz_worldxz unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(unlink(worldxz_insp),__LINE__);
    fprintf(logfile,"TIME: %s Pipe worldxz_insp unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(unlink(cmd_insp),__LINE__);
    fprintf(logfile,"TIME: %s Pipe cmd_insp unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(unlink(pid_cmd_mx),__LINE__);
    fprintf(logfile,"TIME: %s Pipe pid_cmd_mx unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(unlink(insp_wd),__LINE__);
    fprintf(logfile,"TIME: %s Pipe ins_wd unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(unlink(mx_mz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe mx_mz unlinked\n",ctime(&curtime));
    fflush(logfile);

    //end unlinking pipes

    //start killing processes

    time(&curtime);
    controller(other_states=kill(mx,SIGTERM),__LINE__); 
    fprintf(logfile,"TIME: %s Motor x terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    time(&curtime); 
    controller(other_states=kill(mz,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Motor z terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    time(&curtime);
    controller(other_states=kill(worldxz,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Worldxz terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    time(&curtime);
    controller(other_states=kill(watchdog,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Watchdog terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    time(&curtime);
    controller(other_states=kill(pid_cmd,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Konsole Command terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    time(&curtime);
    controller(other_states=kill(pid_insp,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Konsole Inspection terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    //end killing processes

    fprintf(logfile,"TIME: %s Master terminated\n",ctime(&curtime));
    fflush(logfile);
    
    printf("\nMaster terminated\n");
    fflush(stdout);

    fclose(logfile);

    exit(EXIT_SUCCESS);

}

int main(int argc, char const *argv[]){

    int fd_master_insp,fd_master_cmd;

    time(&curtime);
    pid_t process;
    logfile=fopen("./logfiles/master_logfile","w"); 
    struct sigaction closing;

    fprintf(logfile,"TIME: %s Master spawned with PID: %d\n",ctime(&curtime),getpid());

    memset(&closing,0,sizeof(closing));
    sigemptyset(&closing.sa_mask);
    closing.sa_flags=SA_RESTART;
    closing.sa_handler=&closing_function;
    controller(sigaction(SIGINT,&closing,NULL),__LINE__);

    //starting making pipes

    time(&curtime);
    controller(fifo_maker(cmd_mx),__LINE__);
    fprintf(logfile,"TIME: %s Pipe cmd_mx created\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(fifo_maker(cmd_mz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe cmd_mz created\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(fifo_maker(mx_worldxz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe mx_worldxz created\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(fifo_maker(mz_worldxz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe mz_worldxz created\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(fifo_maker(worldxz_insp),__LINE__);
    fprintf(logfile,"TIME: %s Pipe worldxz_insp created\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(fifo_maker(cmd_insp),__LINE__);
    fprintf(logfile,"TIME: %s Pipe cmd_insp created\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(fifo_maker(pid_cmd_mx),__LINE__);
    fprintf(logfile,"TIME: %s Pipe pid_cmd_mx created\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(fifo_maker(insp_wd),__LINE__);
    fprintf(logfile,"TIME: %s Pipe insp_wd created\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(fifo_maker(mx_mz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe mx_mz created\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(fifo_maker(master_cmd),__LINE__);
    fprintf(logfile,"TIME: %s Pipe master_cmd created\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(fifo_maker(master_insp),__LINE__);
    fprintf(logfile,"TIME: %s Pipe master_insp created\n",ctime(&curtime));
    fflush(logfile);

    //end making pipes

    usleep(200);           //in order to avoid problems about pipes

    //starting spawning process

    time(&curtime);
    char *arg_list_6[] = {"./bin/watchdog",NULL};
    watchdog=spawn("./bin/watchdog",arg_list_6);
    fprintf(logfile,"TIME: %s Watchdog spawned with PID: %d\n",ctime(&curtime),watchdog);
    fflush(logfile);

    sprintf(pid_wd,"%d",watchdog);

    time(&curtime);
    char *arg_list_3[] = {"./bin/mx",pid_wd,NULL};
    mx=spawn("./bin/mx",arg_list_3);
    fprintf(logfile,"TIME: %s Motor x spawned with PID: %d\n",ctime(&curtime),mx);
    fflush(logfile);

    sprintf(pid_mx,"%d",mx);

    time(&curtime);
    char *arg_list_4[] = {"./bin/mz", pid_wd,pid_mx, NULL};
    mz=spawn("./bin/mz",arg_list_4);
    fprintf(logfile,"TIME: %s Motor z spawned with PID: %d\n",ctime(&curtime),mz);
    fflush(logfile);

    sprintf(pid_mz,"%d",mz);

    time(&curtime);
    char *arg_list_5[] = {"./bin/worldxz",pid_wd ,NULL};
    worldxz=spawn("./bin/worldxz",arg_list_5);
    fprintf(logfile,"TIME: %s Worldxz spawned with PID: %d\n",ctime(&curtime),worldxz);
    fflush(logfile);

    time(&curtime);
    char *arg_list_1[] = {"/usr/bin/konsole","--hold" ,"-e", "./bin/cmd",pid_wd, NULL};
    konsole_cmd=spawn("/usr/bin/konsole",arg_list_1);
    fprintf(logfile,"TIME: %s Konsole Command spawned with PID: %d\n",ctime(&curtime),konsole_cmd);
    fflush(logfile);
    
    time(&curtime);
    char *arg_list_2[] = {"/usr/bin/konsole","--hold", "-e", "./bin/inspection", pid_mx, pid_mz, pid_wd,NULL};
    konsole_inspection=spawn("/usr/bin/konsole",arg_list_2);
    fprintf(logfile,"TIME: %s Konsole Inspection spawned with PID: %d\n",ctime(&curtime),konsole_inspection);
    fflush(logfile);

    //end spawning process

    //start opening pipes

    controller(fd_master_cmd=open(master_cmd,O_RDONLY),__LINE__); // open pipe MASTER_CMD to get cmd pid

    controller(fd_master_insp=open(master_insp,O_RDONLY),__LINE__); // open pipe MASTER_INSP to get inspection pid

    //end opening pipes

    //start reading 

    controller(read(fd_master_cmd,&pid_cmd,sizeof(pid_t)),__LINE__); //receive cmd pid

    controller(read(fd_master_insp,&pid_insp,sizeof(pid_t)),__LINE__); //receive inspection pid

    //end reading

    //close file descriptors
    close(fd_master_cmd);
    close(fd_master_insp);

    printf("insp: %d cmd: %d\n",pid_insp,pid_cmd);

    //start unlinking master_cmd and master_insp

    time(&curtime);
    unlink(master_cmd);
    fprintf(logfile,"TIME: %s Pipe master_cmd unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    unlink(master_insp);
    fprintf(logfile,"TIME: %s Pipe master_insp unlinked\n",ctime(&curtime));
    fflush(logfile);

    //start wait

    controller(process=wait(&status),__LINE__);
    
    time(&curtime);
    if(WIFEXITED(status)){
        printf("First process has terminated with PID: %d and status: %d\n",process,WEXITSTATUS(status));
        fflush(stdout);
        fprintf(logfile,"TIME: %s First process has terminated with PID: %d and status: %d\n",ctime(&curtime),process,WEXITSTATUS(status));
        fflush(logfile);   
    }
    else{
        fprintf(stderr,"First process has terminated with PID: %d and status: %d\n",process,status);
        fflush(stderr);
        fprintf(logfile,"TIME: %s First process has terminated with PID: %d and status: %d\n",ctime(&curtime),process,status);
        fflush(logfile);
    }

    //end wait

    // start unlinking pipes

    time(&curtime);
    controller(unlink(cmd_mx),__LINE__);
    fprintf(logfile,"TIME: %s Pipe cmd_mx unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    controller(unlink(cmd_mz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe cmd_mz unlinked\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(unlink(mx_worldxz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe mx_worldxz unlinked\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(unlink(mz_worldxz),__LINE__);
    fprintf(logfile,"TIME: %s Pipe mz_worldxz unlinked\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(unlink(worldxz_insp),__LINE__);
    fprintf(logfile,"TIME: %s Pipe worldxz_insp unlinked\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(unlink(cmd_insp),__LINE__);
    fprintf(logfile,"TIME: %s Pipe cmd_insp unlinked\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(unlink(pid_cmd_mx),__LINE__);
    fprintf(logfile,"TIME: %s Pipe pid_cmd_mx unlinked\n",ctime(&curtime));
    fflush(logfile);
    
    time(&curtime);
    controller(unlink(insp_wd),__LINE__);
    fprintf(logfile,"TIME: %s Pipe insp_wd unlinked\n",ctime(&curtime));
    fflush(logfile);

    time(&curtime);
    unlink(mx_mz);
    fprintf(logfile,"TIME: %s Pipe mx_mz unlinked\n",ctime(&curtime));
    fflush(logfile);

    //end unlinking pipes

    //start killing process

    time(&curtime);
    if(process==konsole_cmd){    //only the konsole cmd and cmd or the konsole inspection and inspection can be terminated
        controller(other_states=kill(pid_insp,SIGTERM),__LINE__);   // inspection
        controller(other_states=kill(konsole_inspection,SIGTERM),__LINE__);  // konsole inspection
        fprintf(logfile,"TIME: %s Konsole insp and insp killed. Konsole inspection status: %d\n",ctime(&curtime),other_states);
        fflush(logfile);
    }
    else{  //TODO: da sistemare
        controller(other_states=kill(pid_cmd,SIGTERM),__LINE__);
        controller(other_states=kill(konsole_cmd,SIGTERM),__LINE__);
        fprintf(logfile,"TIME: %s Konsole command and command killed. Konsole command status: %d\n",ctime(&curtime),other_states);
        fflush(logfile);
    }

    time(&curtime);
    controller(other_states=kill(mx,SIGTERM),__LINE__); 
    fprintf(logfile,"TIME: %s Motor x terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    time(&curtime); 
    controller(other_states=kill(mz,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Motor z terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    time(&curtime);
    controller(other_states=kill(watchdog,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Watchdog terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);

    if(process!=worldxz){
    time(&curtime);
    controller(other_states=kill(worldxz,SIGTERM),__LINE__);
    fprintf(logfile,"TIME: %s Worldxz terminated with status: %d\n",ctime(&curtime),other_states);
    fflush(logfile);
    }
    else{
        time(&curtime);
        fprintf(logfile,"TIME: %s konsole inspection and inspection terminated with status: %d\n",ctime(&curtime),status);
        fflush(logfile);
    }
    //end killing process

    //ending master process and closing file pointer
    time(&curtime);
    fprintf(logfile,"TIME: %s Master terminated\n",ctime(&curtime));
    fflush(logfile);
    
    printf("Master terminated\n");
    fflush(stdout);

    fclose(logfile);

    return 0;
}
