#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
//Total number of child processes
#define NUM_CHILDREN 15

//Defines what each child should do
typedef enum {
    //Execute a command
    ACTION_EXEC = 0,      
    //Terminate using abort
    ACTION_ABORT = 1     
} ChildAction;

typedef struct {
    ChildAction action;
    char *argv[8];        
} ChildTask;
//Helper function to print command
static void print_command(char *const argv[]) {
    for (int i = 0; argv[i] != NULL; i++) {
        if (i > 0) printf(" ");
        printf("%s", argv[i]);
    }
}

int main(void) {
    ChildTask tasks[NUM_CHILDREN] = {
        //List of child tasks
        { ACTION_EXEC,  { "ls", "-l", NULL } },
        { ACTION_EXEC,  { "date", NULL } },
        { ACTION_EXEC,  { "pwd", NULL } },
        { ACTION_EXEC,  { "whoami", NULL } },
        { ACTION_EXEC,  { "uname", "-a", NULL } },
        { ACTION_EXEC,  { "id", NULL } },
        { ACTION_EXEC,  { "uptime", NULL } },
        { ACTION_EXEC,  { "echo", "Hello Kobe", NULL } },    
	{ ACTION_EXEC,  { "echo", "Child says hi", NULL } },
        { ACTION_EXEC,  { "env", NULL } },
        //execvp failure
        { ACTION_EXEC,  { "fakecommand1", NULL } },   
        { ACTION_EXEC,  { "fakecommand2", NULL } },  
        //signal-based terminations
        { ACTION_ABORT, { "Abort Child 12", NULL } },    
        { ACTION_ABORT, { "Abort Child 13", NULL } },   
        { ACTION_EXEC,  { "echo", "Last child ran", NULL } }
    };
    //Store child PIDs
    pid_t childPids[NUM_CHILDREN];

    printf("Parent process PID: %d\n", (int)getpid());
    fflush(stdout);
    //Create children and record PID
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            //Child process
            printf("Child #%d | PID=%d | About to run: ", i, (int)getpid());

            if (tasks[i].action == ACTION_ABORT) {
                printf("abort() (signal termination demo)\n");
                fflush(stdout);
                abort(); 
            }
            //Execute command
            print_command(tasks[i].argv);
            printf("\n");
            fflush(stdout);

            execvp(tasks[i].argv[0], tasks[i].argv);

            //Runs if execvp fails
            perror("execvp failed");
	    _exit(127); 
        }

        childPids[i] = pid;
    }

    //Counters
    int countExit0 = 0;
    int countExitNonZero = 0;
    int countSignaled = 0;

    printf("\nParent:\n");
    fflush(stdout);
    //Wait for children in creation order
    for (int i = 0; i < NUM_CHILDREN; i++) {
        int status = 0;
        pid_t w = waitpid(childPids[i], &status, 0);

        if (w < 0) {
            perror("waitpid failed");
            exit(EXIT_FAILURE);
        }

        printf("Parent: Child #%d (PID=%d) finished -> ", i, (int)childPids[i]);
        //Normal termination
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            printf("exited normally, exit code=%d\n", code);
            if (code == 0) countExit0++;
            else countExitNonZero++;
        //Signal termination
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            printf("terminated by signal %d (%s)\n", sig, strsignal(sig));
            countSignaled++;
        } else {
            printf("ended (status=%d)\n", status);
        }

        fflush(stdout);
    }
    //Final summary
    printf("Exited normally with code 0: %d\n", countExit0);
    printf("Exited normally with non-zero code: %d\n", countExitNonZero);
    printf("Terminated by a signal: %d\n", countSignaled);
    return 0;
}
