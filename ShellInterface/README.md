#Shell Interface Program

This program is a simple shell interface that allows users to execute shell commands, view command history, and run commands concurrently. The shell is implemented in C and provides basic functionalities similar to a Unix/Linux terminal.

#Features:

Execute shell commands: Users can type in any valid shell command, and the program will execute it.
Command history: The program keeps a history of the last five commands entered by the user.
Repeat last command: Users can type !! to execute the most recent command from the history.
Concurrent execution: Users can run commands concurrently by appending & to the command.
Exit command: The shell can be exited by typing exit.


Example Usage:
osh> ls
osh> pwd
osh> history
3 pwd
2 ls
1 history
osh> !!
pwd
osh> ls &


Compile the program using:
gcc -o shell shell.c

Run the shell program:
./shell

