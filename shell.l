
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <iostream>
#include <string>
#include <cstring>
#include "y.tab.hh"
#include <sys/wait.h>
static  void yyunput (int c,char *buf_ptr  );

extern int return_code;
extern int exclaim;
extern std::string last;

void myunputc(int c) {
  unput(c);
}

void src(const char * filename) {
  FILE * fp = fopen(filename, "r");
  YY_BUFFER_STATE buffer = yy_create_buffer(fp, YY_BUF_SIZE); //creates a buffer
  yypush_buffer_state(buffer); //get the state of the buffer
  yyparse();
  yypop_buffer_state();
}

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}
  /* went down the list in the powerpoint for the grammar */
"|" {
  return PIPE;
}

"&" {
  return AMPERSAND;
}

"<" {
  return LESS;
}

">>" {
  return GREATGREAT;
}

">&" {
  return GREATAMP;
}

">>&" {
  return GREATGREATAMP;
}

"2>" {
  return TWOGREAT;
}



\$\{[^\t\n ]*\}([^ \t\n])* { //env var exp
  std::string str = std::string(yytext);
  std::size_t found = str.find('$'); //first find $
  std::string final = "";
  std::string temp = "";

  while (found != -1) { //get rid of $
    std::string parts;
    str.erase(found, 1);
    parts += str;
    if (str[found] == '{') {
      str.erase(found, 1);
      std::size_t close = str.find('}'); //there must be a closed one
      temp = str.substr(0, close); //gets the first
      if (close != -1) {
        str.erase(0, close+1); 
      }
      if (getenv(temp.c_str()) != NULL) {
        final += getenv(temp.c_str());
      }
      if (temp == "$") { //returns the pid of the shell process
        final += std::to_string(getpid());
      }
      if (temp == "!") { //pid of the last process run in background  
        final += std::to_string(exclaim);
      }
      if (temp == "?") { //the return code
        final = std::to_string(return_code);
      }
      if (temp == "_") {
        final = last;
      }
    }
    found = str.find('$'); //update found
    final += str.substr(0, found);
    str.erase(0, found);
    found = str.find('$');
  }
  yylval.cpp_string = new std::string(final);
  return WORD;
}

~[^ \n\t]* { //tilde regex
  std::string str = std::string(yytext);
  if (str.size() == 1) { //its only ~ given
    yylval.cpp_string = new std::string(getenv("HOME"));
  } 
  else { //there's more than ~
    str = std::string("/homes/") + str.substr(1,-1);
    yylval.cpp_string = new std::string(str);
  }
  return WORD;
}

$\([^\n]*\) { //subshell
  std::string str = std::string(yytext); //get the text
  str = str.substr(2, str.size() - 3); //take out the (,),$
  str = str.append("\nexit\n"); //end the str
 
  //create the pipes, default
  int pin[2], pout[2], tempin, tempout;
  tempin = dup(0);
  tempout = dup(1);
  pipe(pin);
  pipe(pout);

  //redirection
  dup2(pin[0], 0);
  dup2(pout[1], 1);
  close(pin[0]);
  close(pout[1]);

  //write
  write(pin[1], str.c_str(), str.size());
  close(pin[1]);

  int pid = fork(); //forking begins

  if (pid == 0) { //if fork() was successful
    char ** args = new char*[2];
    args[0] = (char*)"/proc/self/exe";
    args[1] = NULL; //null terminating
    execvp(args[0], args); //execute the command
    perror("execvp(subshell)");
    exit(1);
  } 
  else if (pid < 0) { //fork() was unsuccessful
    perror("fork");
    exit(1);
  }
    dup2(tempin, 0);
    dup2(tempout, 1);
    close(tempin);
    close(tempout);
    
    char* cp = new char[2048];
    int numR = read(pout[0], cp, 2048);
    close(pout[0]);

    for (int i = numR - 1; i >= 0; --i ) { //putting the char back
      if (cp[i] == '\n') {
        cp[i] = ' ';
      }
      myunputc(cp[i]);
    }
  }


[^ \\\n]*(\\[^nt])((\\[^nt])|[^ \\\n])* { //escape regex, the word between
  std::string str = std::string(yytext);
  std::size_t found = str.find('\\');
  while (found != -1) {//there may be more than one
    str = str.substr(0, found) + str.substr(found+1); //take out char at the found location
    found = str.find('\\', found+1); //reset found, change the starting point
  }
  yylval.cpp_string = new std::string(str);
  return WORD;
}  
  

\"[^\n"]*\"  { //quotes regex
  std::string str = std::string(yytext);
  str = str.substr(1, str.length() - 2); //-2 to take out the 2 quote chars
  yylval.cpp_string = new std::string(str); 
  return WORD;
}

[^ \t\n|<&>][^ \t\n|<&>]*  { //word definition changed due to symbols
  /* Assume that file names have only alpha chars */
//  printf("here");
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}
