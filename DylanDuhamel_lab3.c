// Dylan Duhamel - duhadm19@wfu.edu - 10/11/22
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void commandParse(char* string, const char* delim, char* token, char** argv, int* argc, int* numPipes, int* pipeList, int* ampCheck);
void commandExec(char** argv, int* numPipes, int* pipeList, int* ampCheck);

// Global process id
int pid;
int main () { // could also use char ** argv

  char *input; // input list from stdin
  size_t inpSize = 32; // input buffer size
  size_t chars; // char size
  int exitCondition = 0; // exit switch
  int argc = 0; // argc number of commands per line
  int pipeList[3];
  int numPipes = 0;
  int ampCheck = 0;

  // Setup for strtok
  const char delim[2] = " \"";  // token delimiter
  char *token;
  char **argv; // double poointer string list of strings

  // allocating for stdin input
  input = (char*)malloc(inpSize * sizeof(char));
  if (input == NULL) {
    perror("Unable to allocate for input");
    return -1;
  }

  // Ignoring the sigint so that only exit is "exit"
  signal(SIGINT, SIG_IGN);

  // looping and getting stdin input until exit is typed
  while(!exitCondition) {

    argv = malloc(1014 * sizeof(char *));
    printf("$ "); // prompt

    // getting stdin line
    chars = getline(&input, &inpSize, stdin);
    // calling parse function
    commandParse(input, delim, token, argv, &argc, &numPipes, pipeList, &ampCheck);

    // Checking for exit condition
    if(argc == 1 && strcmp(argv[0], "exit") == 0) {
      exitCondition = 1;
      break;
    }

    //exec
    commandExec(argv, &numPipes, pipeList, &ampCheck);

    // Clearing memory
    for(int i = 0; i < argc; i++) {
      free(argv[i]);
    }
    free(argv);

    // Setting command total to 0
    argc = 0;
    numPipes = 0;
    ampCheck = 0;
  }

  printf("OK close shop and go home\n");
  return 0;
}

// Executing commands
// Using fork to make a child process
void commandExec(char **argv, int* numPipes, int* pipeList, int *ampCheck) {
  int f_des[2];
  int f_des2[2];
  int pid, pid2, pid3;

  // No pipes
  if (*numPipes == 0) {
    // Creating a child process
    pid = fork();

    // Error handling
    if (pid == -1) {
      perror("Error creating process");
      exit(1);
    }

    // Child process
    if (pid == 0) {
      execvp(argv[0], &argv[0]);
      perror("Child returned from exec, bad");
      exit(1);
    } else if (pid > 0) {
      if (!*ampCheck) {
        wait(NULL); // Here we should check of command has &
    }
  }

  } else if(*numPipes == 1) { // 1 pipe
    // Creating a pipe
    if (pipe(f_des) == -1) {
      perror("Pipe");
      exit(2);
    }
    // Creating fork process
    pid = fork();
    if(pid == -1) {
      perror("Fork");
      exit(2);
    }
    else if (pid == 0) {
      dup2(f_des[1], fileno(stdout));
      close(f_des[0]);
      close(f_des[1]);
      execvp(argv[0], &argv[0]);
      exit(3);
    } else {
      pid2 = fork();
      if (pid2 == 0) {
        dup2(f_des[0], fileno(stdin));
        close(f_des[0]);
        close(f_des[1]);
        execvp(argv[pipeList[0]+1], &argv[pipeList[0]+1]);
        exit(4);
      }
    }
    close(f_des[0]);
    close(f_des[1]);

    if (!*ampCheck) {
      waitpid(pid, NULL, 0);
      waitpid(pid2, NULL, 0);
    }

  } else if(*numPipes == 2) {
    // Creating a pipe
    if (pipe(f_des) == -1) {
      perror("Pipe");
      exit(2);
    }

    pid = fork();
    if(pid == -1) {
      perror("Fork");
      exit(2);
    }
    // In child
    else if(pid == 0) {
      dup2(f_des[1], fileno(stdout));
      close(f_des[0]);
      close(f_des[1]);
      execvp(argv[0], &argv[0]);
      // do better error handling here LIKE PERROR
      exit(3);
    }

    if(pipe(f_des2) == -1) {
      perror("Pipe");
      exit(2);
    }

    pid2 = fork();
    if (pid2 == -1) {
      perror("Fork");
      exit(4);
    }
    // In child
    else if(pid2 == 0) {
      dup2(f_des[0], fileno(stdin));
      dup2(f_des2[1], fileno(stdout));
      close(f_des[0]);
      close(f_des[1]);
      close(f_des2[0]);
      close(f_des2[1]);
      execvp(argv[pipeList[0]+1], &argv[pipeList[0]+1]);
      // do better error handling here
      exit(5);
    }

    // Close fd since not in use
    close(f_des[0]);
    close(f_des[1]);

    if (!*ampCheck) {
      waitpid(pid, NULL, 0);
      waitpid(pid2, NULL, 0);
    }

    pid3 = fork();
    if(pid3 == -1) {
      perror("Fork");
      exit(6);
    }
    // In Child
    else if(pid3 == 0) {
      dup2(f_des2[0], fileno(stdin));
      close(f_des2[0]);
      close(f_des2[1]);

      // EXEC
      execvp(argv[pipeList[1]+1], &argv[pipeList[1]+1]);
    }

    // close since not in use
    close(f_des2[0]);
    close(f_des2[1]);

    if (!*ampCheck) {
      waitpid(pid3, NULL, 0);
    }
  }
}

// parse function that uses strtok
void commandParse(char* string, const char* delim, char* token, char** argv, int* argc, int* numPipes, int* pipeList, int* ampCheck) {
  /* get the first token */
  token = strtok(string, delim);
  int wordLen = 0;

  // if the only thing entered is the newline character, we can skip all of this
  if (token[0] == '\n') {
    return;
  } else {
    /* walk through other tokens */
    while( token != NULL ) {
      wordLen = strlen(token);

      if(token[wordLen-1] == '\n')
        token[wordLen-1] = '\0'; // makes the last 2 char == "\0"

      if(strcmp(token, "|") == 0) {
        pipeList[*numPipes] = *argc;
        *numPipes = *numPipes + 1;

        argv[*argc] = malloc(wordLen + 1);
        argv[*argc] = NULL;
        // inc of argc counter
        *argc = *argc+1;
        token = strtok(NULL, delim);
      } else if (*token == '&') {
        *ampCheck = 1;
        token = strtok(NULL, delim);
      } else if(*token == ' ' || *token == '\0') {
        token = strtok(NULL, delim);
      } else {
        argv[*argc] = malloc(wordLen + 1);
        // copying parsed string to list
        strcpy(argv[*argc], token);
        token = strtok(NULL, delim);
        // inc of argc counter
        *argc = *argc+1;
      }
    }
  }
}
