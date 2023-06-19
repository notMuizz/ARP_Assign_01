# ARP_Assign_01

[Hafiz Muizz Ahmed Sethi](https://github.com/notMuizz)<br>
[M.Sc Robotics Engineering](https://corsi.unige.it/corsi/10635)<br>

## Content of the Repository
Apart from the *install.src* and *run.src* files, the repository is organized as follows:
- The `src` folder contains the source code for the **Master**, **cmd**, **Inspection Console**, **Motor1 (mx)**, **Motor2 (mz)**, **Worldxz** & **Watchdog** processes.
- The `bin` folder is where the executable files corresponding to the previous processes are generated after compilation.
- The `include` folder contains all the data structures and methods used within the *ncurses* framework to build the two GUIs.
- The `log` folder is where the log files corresponding to the above processes will be genearated after running the program.


## Project Description
The project entails the design and development of an interactive hoist simulator with 2 degrees of freedom (d.o.f.). The simulator allows users to control the hoist using two different consoles.

The hoist is equipped with two motors that enable movement along two axes: X (horizontal) and Z (vertical). The range of motion along these axes is limited:  for the X axis and  Z axis.

On the user side, there are two consoles, namely the **command console(cmd)** and the **inspection console**. These consoles, implemented using the **ncurses** library, provide a simulated interface for the actual system. They offer a simple graphical user interface (GUI).

The program consists of following processes, each serving a specific role:

1. **Master** process: This process spawns all the other processes and establishes the necessary unnamed pipes for inter-process communication (IPC). When any of the child processes terminate, the master process unlinks all the pipes, sends a SIGTERM signal to the remaining processes, and finally terminates. To prevent the master process from being terminated before all its children, a handler for the SIGINT signal has been implemented.

2. **Command Console** process: Responsible for reading velocity commands entered by the user via buttons such as **Vx-**,  **STP**, **Vx+**, **Vz-**, and **Vz+**. It relays these commands to the appropriate motor for execution. Additionally, it handles the **Stop** and **Reset** signals, disregarding user commands when these signals are active.

3. **Motor1 (mx)** process: Simulates hoist movement along the X axis. It receives velocity commands from the **Command Console** process, calculates the estimated X position of the hoist, and transmits this information to the **World** process. It also handles the *Stop* signal (immediately setting the velocity to zero) and the *RESET* signal (returning the hoist to its initial position).

4. **Motor2 (mz)** process: Simulates hoist movement along the Z axis. Similar to **Motor1**, it receives velocity commands from the **Command Console** process and performs corresponding operations. It also handles the *STOP* and *RESET* signals in the same manner as **Motor1**.

5. **World** process: Receives the estimated X and Z positions from **Motor1** and **Motor2**, respectively. It introduces a certain level of error to these positions and transmits the actual position values, including the error, to the **Inspection Console** process.

6. **Inspection Console** process: Receives the actual X and Z positions from **Motor1** and **Motor2**. It displays the current position of the hoist in its associated window. Additionally, it manages the STOP **S** and RESET **R** buttons, relaying the appropriate signals to the **Command Console**, **Motor1**, and **Motor2** processes when these buttons are pressed.

7. **Watchdog** process: The watchdog process ensures a 60-second period of inactivity in the CMD process, preventing command transmissions. It also enforces a 60-second restriction on user interaction with the system. Initially inactive, it becomes active when the CMD process triggers movement with a SIGUSR2 signal, starting a 60-second alarm. If no additional SIGUSR2 signals are received within this interval, the watchdog triggers a self-generated SIGALRM signal. In response, the watchdog sends a SIGUSR2 signal to the inspection process, initiating a system reset. Process termination is controlled by the SIGTERM signal.


## Required libraries/packages
- *ncurses* library. You can install it with the following command:
```console
sudo apt-get install libncurses-dev
```

- *konsole* application. You can install it with the following command:
```console
sudo apt-get install konsole
```

## Compiling and running the code
In order to compile all the processes that make up the program, you just need to execute the following command:
```console
chmod +x install.src
./install.src
```

Then, you can run the program by executing:
```console

chmod +x run.src
./run.src
```
After successful execution two windows will appear:
![Screenshot from 2023-06-19 13-30-46](https://github.com/notMuizz/ARP_Assign_01/assets/123844091/cfb60994-aa72-4dbc-99cc-f5283268499d)

### KONSOLE COMMAND

**Vx+**: it increases horizontal velocity by 1 <br>
**Vx-**: it decreases horizontal velocity by 1 <br>
**STP**: it sets the horizontal velocity to zero <br>

**Vz+**: it increases vertical velocity by 1 <br>
**Vz-**: it decreases vertical velocity by 1 <br>
**STP**: it sets the vertical velocity to zero <br>

The upper and lower upper limit of velocity is +3/-3

By Pressing the Command Buttons and Reaching the target the final result displayed as under :
![Screenshot from 2023-06-19 13-32-15](https://github.com/notMuizz/ARP_Assign_01/assets/123844091/466f5eee-ca5d-4b67-97ae-de4c7a8a6d18)
   


### KONSOLE INSPECTION

**S**: it stops both engines

**R**: it resets the hoist to the initial position (0.00,0.00)

A video representation of this assignment is also present :


https://github.com/notMuizz/ARP_Assign_01/assets/123844091/21c4c358-b86b-4681-af0a-a0a42d3d21e6



## Troubleshooting
Should you experience some weird behavior after launching the application (buttons not spawning inside the GUI or graphical assets misaligned) simply try to resize the terminal window, it should solve the bug.




