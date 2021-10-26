/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <fcntl.h>

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include "command.hh"
#include "shell.hh"

void src(const char *);
int return_code = 0;
int exclaim = 0;
std::string last = "";

Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _append = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    int check = 1;

    if (_outFile == _errFile) {
      delete _errFile;
      check = 0;
    }

    if ( _outFile && check == 1) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile && check == 1 ) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::prompt();
        return;
    }

    //exit
    if (strcmp(_simpleCommands[0]->_arguments[0]->c_str(), "exit") == 0) {
      printf("Goodbye\n");
      exit(1);
    }
   
    // Print contents of Command data structure
    //print();

    // Add execution here
    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec

    //temp variables
    int defaultIn = dup(0);
    int defaultOut = dup(1);
    int defaultErr = dup(2);

    int fdin;
    int fdout;
    int fderr;

    if (_inFile) { //input redirection exists
      const char * in = _inFile->c_str();
      fdin = open(in, O_RDONLY, 0600); //only read the input file
    }
    else { //default input
      fdin = dup(defaultIn);
    }

    int pid;
    for ( size_t i = 0; i < _simpleCommands.size(); i++ ) {
      dup2(fdin, 0); //the input comes from fdin
      close(fdin);
      if ( i == _simpleCommands.size() - 1) { //last command
        if (_outFile) { //output redirection exists
          const char * out = _outFile->c_str();
          if (_append == true) { //check the append flag
            fdout = open(out, O_WRONLY | O_CREAT | O_APPEND, 0600); //can write, create, append
          } 
          else {
            fdout = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0600); //cant append
          }
        } 
        else {//default
          fdout = dup(defaultOut);
        }
        if (_errFile) { //error out redirection exists
          const char * err = _errFile->c_str();
          if (_append == true) { //check the append flag
            fderr = open(err, O_WRONLY | O_CREAT | O_APPEND, 0600);
          }
          else {
            fderr = open(err, O_WRONLY | O_CREAT | O_TRUNC, 0600);
          }
        }
        else { //default input
          fderr = dup(defaultErr);
        }
      }
      else { //not the last command
        int fdpipe[2];
        pipe(fdpipe);
        fdout = fdpipe[1];
        fdin = fdpipe[0]; 
      }
      
      dup2(fdout, 1);
      dup2(fderr, 2);
      close(fdout);
      close(fderr);

     //cd
      if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "cd")) {
        int ret;
        if (_simpleCommands[i]->_arguments.size() == 1) {
          ret = chdir(getenv("HOME"));
        }
        else {
          ret = chdir(_simpleCommands[i]->_arguments[1]->c_str());
	}
	if (ret != 0) {
          fprintf(stderr, "cd: can't cd to notfound\n");
	}
	continue;
      }
      //setenv
      else if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "setenv")) {	 	
        setenv(_simpleCommands[i]->_arguments[1]->c_str(), _simpleCommands[i]->_arguments[2]->c_str(), 1);
	continue;
      //unsetenv
      } 
      else if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "unsetenv")) {
        unsetenv(_simpleCommands[i]->_arguments[1]->c_str());
	continue;
      }
      else if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "source")) {
        _simpleCommands.clear(); //fresh start
        src(_simpleCommands[i]->_arguments[1]->c_str()); //call the source
        return;
      }
      char **temp =  environ; //gets the global var from the parent NOT the child
      pid = fork(); //start the process
      if (pid == 0) { //child process
        //printenv
        if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv")) {
          char ** p = temp;
          while (*p != NULL) {
            printf("%s\n", *p);
            p++;
          }
          exit(0);
        }
        char * simpleCom[_simpleCommands[i]->_arguments.size() + 1]; // temp array to convert 
        size_t j;
        for ( j = 0; j < _simpleCommands[i]->_arguments.size(); j++) { //iterating through the arguments
          simpleCom[j] = strdup(_simpleCommands[i]->_arguments[j]->c_str());
        }
        simpleCom[j] = NULL;
        //printf("%d\n", j);
        //printf("%s\n", last);
        execvp(simpleCom[0], simpleCom);
        perror("execvp");
        exit(1);
      }
      else if (pid == -1) { //forking failed
        perror("fork");
        return;
      }
      last = strdup(_simpleCommands[i]->_arguments[_simpleCommands[i]->_arguments.size()-1]->c_str());
    }
    //restore in/out defaults & closes file descriptors
    dup2(defaultIn, 0);
    dup2(defaultOut, 1);
    dup2(defaultErr, 2);
    close(defaultIn);
    close(defaultOut);
    close(defaultErr);
    int status = 0;
    if (!_background) { //for the parent process
      waitpid(pid, &status, 0);
      return_code = WEXITSTATUS(status);     
    }
    else {
      exclaim = pid; //for last process run in the background
    }
    // Clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
