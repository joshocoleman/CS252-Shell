#include <cstdio>
#include <unistd.h>
#include "shell.hh"
#include <signal.h>
#include <sys/wait.h>

int yyparse(void);

void Shell::prompt() {
  if (isatty(0)) {
    printf("myshell>");
    fflush(stdout);
  }
}

extern "C" void ctrl(int sig) {
  Shell::_currentCommand.clear();
  printf("\n");
  Shell::prompt();
}

extern "C" void zombie(int sig) {
  while (waitpid(-1, NULL, WNOHANG) > 0) { //closes the zombie processes
  }
}

int main(int, char ** argv) {
  struct sigaction signalAction;
  signalAction.sa_handler = ctrl;
  sigemptyset(&signalAction.sa_mask);
  signalAction.sa_flags = SA_RESTART;

  struct sigaction sa;
  sa.sa_handler = zombie;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction (SIGCHLD, &sa, NULL);
  
  char * dir = argv[0]; //gets the arg
  char * expand = realpath(dir, NULL); //uses the realpath
  setenv("SHELL", expand, 1); //set the environment variable
  free(expand);

  if (sigaction(SIGINT, &signalAction, NULL)) { //check
    perror("sigaction");
    exit(-1);
  }
  Shell::prompt();
  yyparse();
}

Command Shell::_currentCommand;
