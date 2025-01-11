#include <stdio.h>
#include <string.h>
#include <unistd.h> //To remove warnings for fork() and execvp()
#include <sys/wait.h> //To remove warning for wait()

//Macros
#define MAX_LINE 80
#define HISTORY_SIZE 5

//Function Prototypes
void executeCommand(char commandStr[]);
void print_history(char history[][MAX_LINE], int history_count);

//Variables
char *args[MAX_LINE / 2 + 1];
int should_run = 1;
int pid;
int run_concurrent = 0;
char str[MAX_LINE]; //Input string to be used by our execution functions
char input[MAX_LINE]; //Input string to be used by our history functions
char history[HISTORY_SIZE][MAX_LINE] = {0};
int history_count = 0; //variable to store number of commands in history
int command_count = 0; //variable to store total number of commands input by user

int main(int argc, char *argv[]) {

    while (should_run) {
        printf("osh> ");
        fflush(stdout);
        fgets(str, MAX_LINE, stdin); //Gets the entire input as a string
        strcpy(input, str); //Copies the same input string to the input variable
        input[strcspn(input, "\n")] = '\0'; // Remove newline from input variable

        // Check if the command is history
        if (strcmp(input, "history") == 0) {
            print_history(history, history_count);
            continue;
        }

        // If command is not history and is not !!, store the command in the history
        if (strlen(input) > 0 && strcmp(input, "!!") != 0) {
	    command_count += 1;
            if (history_count >= HISTORY_SIZE) {
                // Shift commands in history to make room for the new command
                for (int i = 0; i < HISTORY_SIZE - 1; i++) {
                    strcpy(history[i], history[i + 1]);
                }
                strcpy(history[HISTORY_SIZE - 1], input); // Store new command in last position
            } else {
                strcpy(history[history_count], input); // Store command in the next available spot
                history_count++; // Increment number of commands in history
            }
        }

        // Check if command is !!
        if (strcmp(input, "!!") == 0) {
            if (history_count == 0) {
                printf("No commands in history\n");
            } else {
		command_count += 1;
                char temp1[MAX_LINE]; //Will be used to save in history
                char temp2[MAX_LINE]; //Will be used to pass to executeCommand() because it modifies the string
                strcpy(temp1, history[history_count - 1]); // Get the most recent command
                strcpy(temp2, temp1); // Copy the same command to temp2
                printf("%s\n", temp1);  // Print most recent command
                executeCommand(temp2);  // Run most recent command
                if (history_count >= HISTORY_SIZE) {
                    // Shift commands in history to make room for the new command
                    for (int i = 0; i < HISTORY_SIZE - 1; i++) {
                        strcpy(history[i], history[i + 1]);
                    }
                    strcpy(history[HISTORY_SIZE - 1], temp1); // Store command in last position
                } else {
                    strcpy(history[history_count], temp1); // Store command in the next available spot
                    history_count++; // Increment number of commands in history
                }
            }
            continue;
        }

        executeCommand(str);
    }
    return 0;
}

//Function takes string directly from fgets(), and attempts to execute it
void executeCommand(char commandStr[]) {
    char* token = strtok(commandStr, " "); //Split up the string by delimiter " " (space)
    //Loop through our split string and put each word in global args array.
    int index = 0;
    while (token != NULL) {
        args[index] = token;
        index += 1;
        token = strtok(NULL, " ");
    }
    args[index - 1][strcspn(args[index - 1], "\n")] = '\0'; //Remove newline from last argument
    args[index] = NULL; //Add NULL to the end, becaues execvp takes NULL terminated args array

    //Check if first word was "exit", and if so exit
    if (strcmp(args[0], "exit") == 0) {
        should_run = 0; //Stop while loop execution on next iteration
        return; //Skip the rest of this function
    }

    //Check if last word was & for concurrency
    run_concurrent = 0; //Make sure it is initialized to non-concurrent execution
    if (strcmp(args[index - 1], "&") == 0) {
        run_concurrent = 1;
        args[index - 1] = NULL; //Replace & with NULL so that it isnt passed to execvp()
    }

    //Handle child process
    pid = fork();
    if (pid == 0) {
        should_run = 0; //Makes sure that child does not continue looping with parent
        if (execvp(args[0], args) == -1) {
	        printf("Invalid command\n"); //If command not recognized, print invalid command
        }
        return; //Skips the rest of this function for child
    } 

    //Decide whether or not to wait for child depending on run_concurrent value
    if (!run_concurrent) {
        //Waits until correct child terminates so that parent can go to next loop
        int result = wait(NULL);
        while (result != pid) {
            result = wait(NULL);
        }
    }
}

//function to print the history
void print_history(char history[][MAX_LINE], int history_count) {
    //If history empty, print "No commands in history"
    if (history_count == 0) {
        printf("No commands in history\n");
        return; //Skips rest of function
    }
    int j; //variable to control the index printed
    //If history not empty, print all history from newest to oldest
    for (int i = history_count - 1, j = 0; i >= 0; i--, j++) {
        printf("%d %s\n", command_count - j, history[i]);
    }
}
