#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h> 
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h> 
#define MAX_PROMPT 1024  		//the maximum length of command prompt
#define MAXLINE 80 				//the maximum length of command
#define MAXARG 20				//the maximum number of args
#define MAX_NAME 256  			//the maximum length of host name
#define MAX_PATH 1024  			//the maximum length of path
#define MAXPIDTABLE 1024  		//the maximum number of process

struct parseInfo;				//a struct to store information after parsing a command
struct passwd *pwd;				//a struct to obtain home path and user name

void run(char *);
void setEnviron(char profile[][1024]);
void showPrompt();
int readCommand(char **,char **, char *);
int builtinCommand(char *,char **, char profile[][1024],struct parseInfo);
int parse(char **,int,struct parseInfo *);
void handleSig(int sig);

#ifndef __PARSE_INFO__
#define __PARSE_INFO__
#define BACKGROUND 			1       //some macros to denote background(&),redirection(<,<<,>,>>) and pipe(|)
#define IN_REDIRECT 		2
#define OUT_REDIRECT 		4
#define APPEND	            8
#define PIPE     			16
struct parseInfo 
{
    int flag;						//to show whether the command include background, redirection, pipe or not
    char* inputfile;				//the name of the input file used in redirection
    char* outputfile;				//the name of the output file used in redirection
    char* command2;					//the second commmand used in pipe
	char** parameters2;				//the parameters which belong to the second command used in pipe
};
#endif