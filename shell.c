#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wordexp.h>
#include <sys/wait.h>

// |

char cmdString[100];
int pid;

// Generates Prompt [USER]@[DEVICE]:[Current Directory]
void prompt() {
    char *user = getenv("USER");
    char device[256];
    gethostname(device, sizeof(device));
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s@%s:%s$ ", user ? user : "unknown", device, cwd);
}

void changeDirectory(const char *path) {
    if (chdir(path) != 0) {
        perror("ERROR: chdir()\n");
    }
}

void exec_cmd(char *cmd) {
    

    if ( (pid = fork()) < 0){
        perror("ERROR: fork()\n");
    } else if (pid == 0) {
        wordexp_t p;
        wordexp(cmd, &p, 0);
        char ** w=p.we_wordv;
        
        execvp(w[0], w);
        perror("ERROR: execvp()\n");
        wordfree(&p);
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
    }
}

void setEnvironmentVariable(const char *var) {
    char *envName = strtok(cmdString + 4, "=");
    char *envValue = strtok(NULL, "=");

    int result = setenv(envName, envValue, 1);
}

void exec_pipe(char *cmd1, char *cmd2) {
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("ERROR: pipe()\n");
        return;
    }

    if ( (pid = fork()) < 0){
        perror("ERROR: fork()\n");
        return;
    } else if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        exec_cmd(cmd1);
        exit(EXIT_FAILURE);
    }

    if ( (pid = fork()) < 0){
        perror("ERROR: fork()\n");
        return;
    } else if (pid == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        exec_cmd(cmd2);
        exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
    wait(NULL);

}

int main() {
    printf("---- STARTING MINI-SHELL  (Close with 'exit')----\n");
    
    while (1)
    {
        prompt();
        fgets(cmdString, 99, stdin);

        /* eliminate \n */
        cmdString[strcspn(cmdString, "\n")] = 0;

        if (strchr(cmdString, '|') != NULL)
        {
            char *cmd1 = strtok(cmdString, "|");
            char *cmd2 = strtok(NULL, "|");

            exec_pipe(cmd1, cmd2);
            continue;
        }
        
        

        if (strncmp(cmdString, "exit", 4) == 0) {
            exit(EXIT_SUCCESS);
        }

        if (strncmp(cmdString, "cd ", 2) == 0) {
            char *path = cmdString + 2;
            if(*path == 0) {
                path = getenv("HOME");
            } 
            changeDirectory(path);
            continue;
        }

        if (strncmp(cmdString, "set", 3) == 0) {
            setEnvironmentVariable(cmdString);
            continue;
        }

        exec_cmd(cmdString);

    }

    exit(EXIT_SUCCESS);
}