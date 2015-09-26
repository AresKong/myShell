#include "myshell.h"

// A function that shows the command prompt on the console
void showPrompt() {
    extern struct passwd *pwd;
    char prompt[MAX_PROMPT];
    char hostname[MAX_NAME];
    char pathname[MAX_PATH];
    int offset;
    pwd = getpwuid(getuid());
    getcwd(pathname,MAX_PATH);				    						//pathname stores the current working directory	
    if(gethostname(hostname,MAX_NAME) == 0)								//hostname stores the host name
        sprintf(prompt,"myShell>%s@%s:",pwd->pw_name,hostname);  		//pwd->pw_name stores the user name
    else
        sprintf(prompt,"myShell>%s@unknown:",pwd->pw_name);
	offset = strlen(prompt);
    if(strlen(pathname) < strlen(pwd->pw_dir) || 						//to determain whether the current working directory
            strncmp(pathname,pwd->pw_dir,strlen(pwd->pw_dir)) != 0)		//includes home directory
        sprintf(prompt+offset,"%s",pathname);
    else
        sprintf(prompt+offset,"~%s",pathname+strlen(pwd->pw_dir));		//use ~ to replace the home directory in the path
    offset = strlen(prompt);
    if(geteuid() == 0)
        sprintf(prompt+offset,"# ");									//root user
    else
        sprintf(prompt+offset,"$ ");									//ordinary user
    printf("%s",prompt);												//print command prompt on the console
    return;
}

/* A function that initializes the environment variables
 * Use a two dimensional array of characters profile[][1024] to stores environment variables
 * The function stores the value of USER, PWD, HOME in profile[1], profile[2], profile[3] respectively
 * Other two environment variables SHELL(stores in profile[0]) 
 * and parent(stores in profile[4]) are set in the main(void)
 */
void setEnviron(char profile[][1024]) {
    extern struct passwd *pwd;
    char hostname[MAX_NAME];
    char pathname[MAX_PATH];
    char tmp1[MAX_PATH];
    char tmp2[MAX_PATH];
    char tmp3[MAX_PATH];
    pwd = getpwuid(getuid());					//Use getcwd() to get the current working directory
    getcwd(pathname,MAX_PATH);
    sprintf(tmp1,"USER=%s",pwd->pw_name);     	//Struct variable pwd->pw_name stores the user name
    strcpy(profile[1],tmp1);
    sprintf(tmp2,"PWD=%s",pathname);
    strcpy(profile[2],tmp2);
    sprintf(tmp3,"HOME=%s",pwd->pw_dir);		//Struct variable pwd->pw_dir stores the home directory
    strcpy(profile[3],tmp3);
}

/* A function that deal with the command stores in argument buffer
 * The name of the command is stored in argument command
 * The parameters are stored in the argument parameters
 * The function returns the number of the parameters
 * 0 represents only command without any parameters
 * -1 represents wrong input
 */
int readCommand(char **command,char **parameters, char *buffer) {
    if(buffer[0] == '\0')
        return -1;
    char *start, *end;
    int count = 0;
    int flag = 0;
    start = end = buffer;
    while(flag == 0) {
        while((*end==' ' && *start==' ') || (*end=='\t' && *start=='\t')) {  	//Simply omit the white space and tab
            start++;
            end++;
        }

        if(*end=='\0' || *end=='\n') {
            if(count == 0)
                return -1;                					//No commands nor parameters
            break;
        }

        while(*end != ' ' && *end != '\0' && *end != '\n')
            end++;

        if(count == 0) {
            char *tmp = end;
            *command = start;
            while(tmp!=start && *tmp!='/')
                tmp--;
            if(*tmp == '/')
                tmp++;
            parameters[0] = tmp;                //parameters[0] stores the fist segment of buffer
            count += 2;
        }
        else if(count <= MAXARG) {
            parameters[count-1] = start;		//parameters[n-1] stores the n th parameter
            count++;
        }
        else
        	break;

        if(*end == '\0' || *end == '\n') {
            *end = '\0';
            flag = 1;
        }
        else {
            *end = '\0';
            end++;
			start = end;
        }
    }
    parameters[count-1] = NULL;
    return count;
}

/* A function that defines built-in commands.
 * dir, environ, echo and help can redirect their outputs to given file
 * The argument info stores the information about redirection
 * If the argument command is a built-in command, the function returns 1
 * Otherwise the function returns 0
 */
int builtinCommand(char *command, char **parameters, char profile[][1024], struct parseInfo info) {
	extern struct passwd *pwd;
	char pathname[MAX_PATH];
	getcwd(pathname,MAX_PATH);
    int out_fd;
    int sfd = dup(STDOUT_FILENO);           
    if(strcmp(command,"quit") == 0)
        exit(0);
    else if(strcmp(command,"clr") == 0) {
        printf("\e[1;1H\e[2J");						//clear the screen
        return 1;
    }
    else if(strcmp(command,"dir") == 0) {
    	char *dir = parameters[1];					//parameters[1] stores the name of the directory
    	DIR *dp;
    	struct dirent *entry;
    	struct stat statbuf;

    	if((dp = opendir(dir)) == NULL) {			//To determain whether it is a directory or not
    		printf("-myShell: dir: %s:%s\n",dir,strerror(errno));
    		return 1;
    	}
        if(info.flag & OUT_REDIRECT) {				//Redirection >
            out_fd = open(info.outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
            close(fileno(stdout)); 
            dup2(out_fd, fileno(stdout));
            close(out_fd);
        }
        if(info.flag & APPEND) {					//Redirection >>
            out_fd = open(info.outputfile, O_CREAT|O_APPEND|O_WRONLY, 0666);
            close(fileno(stdout)); 
            dup2(out_fd, fileno(stdout));
            close(out_fd);
        }
    	chdir(dir);									//Go into the directory dir
    	while((entry = readdir(dp)) != NULL) {		//Scan every file/directory in the directory dir
    		lstat(entry->d_name,&statbuf);
    		if(S_ISDIR(statbuf.st_mode)) {			//Determain if it is a directory or just a file
    			if(strcmp(".",entry->d_name) == 0	
    			||strcmp("..",entry->d_name) == 0)	//Omit . and .. directory
    				continue;
    			printf("%s/\n",entry->d_name);   	//Print the directory name ended with /
    		}
    		else
    			printf("%s\n",entry->d_name);		//Print the file name
    	}
    	closedir(dp);
    	chdir(pathname);							//Come back to the previou directory
        dup2(sfd,STDOUT_FILENO);					//Make the output back to the stdout after redirection
    	return 1;
    }
    else if(strcmp(command,"environ") == 0) {
        if(info.flag & OUT_REDIRECT) {
            out_fd = open(info.outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);		//Redirection >
            close(fileno(stdout)); 
            dup2(out_fd, fileno(stdout));
            close(out_fd);
        }
        if(info.flag & APPEND) {
            out_fd = open(info.outputfile, O_CREAT|O_APPEND|O_WRONLY, 0666);	//Redirection >>
            close(fileno(stdout)); 
            dup2(out_fd, fileno(stdout));
            close(out_fd);
        }
        for (int i = 0; i < 5; ++i)				//Print all the environment variables stores in the array profile
            printf("%s\n", profile[i]);
        dup2(sfd,STDOUT_FILENO);				//Make the output back to the stdout after redirection
        return 1;
    }
    else if(strcmp(command,"echo") == 0) {
        int i = 1;
        if(info.flag & OUT_REDIRECT) {
            out_fd = open(info.outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);		//Redirection >
            close(fileno(stdout)); 
            dup2(out_fd, fileno(stdout));
            close(out_fd);
        }
        if(info.flag & APPEND) {
            out_fd = open(info.outputfile, O_CREAT|O_APPEND|O_WRONLY, 0666);	//Redirection >>
            close(fileno(stdout)); 
            dup2(out_fd, fileno(stdout));
            close(out_fd);
        }
        while(parameters[i] != NULL)					//Echo what the user types. Tab or more than one white spaces
        	printf("%s ",parameters[i++]);				//turn into only one white space
        printf("\n");
        dup2(sfd,STDOUT_FILENO);						//Make the output back to the stdout after redirection
        return 1;
    }
    else if(strcmp(command,"help") == 0) {
        extern char exepath[MAX_PATH];
        char readme[MAX_PATH];
        sprintf(readme,"%s/readme",exepath);
        char buf[1024];
        FILE *fp;
        if((fp = fopen(readme,"r")) == NULL) {			//Open readme
            perror("fail to open readme");
            exit (1) ;
        }
        if(info.flag & OUT_REDIRECT) {
            out_fd = open(info.outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);		//Redirection >
            close(fileno(stdout)); 
            dup2(out_fd, fileno(stdout));
            close(out_fd);
        }
        if(info.flag & APPEND) {
            out_fd = open(info.outputfile, O_CREAT|O_APPEND|O_WRONLY, 0666);	//Redirection >>
            close(fileno(stdout)); 
            dup2(out_fd, fileno(stdout));
            close(out_fd);
        }
        while(fgets(buf,1024,fp) != NULL)
            printf("%s",buf);
        fclose(fp);
        printf("\n");
        dup2(sfd,STDOUT_FILENO);					//Make the output back to the stdout after redirection
        return 1;
    }
    else if(strcmp(command,"cd") == 0) {
        if(parameters[1] == NULL) {					//If no path is given, then print the current working directory
        	printf("%s\n", pathname);
        }
        else {
        	char *mycd = NULL;
        	if(parameters[1][0] == '~') {
            	mycd = malloc(strlen(pwd->pw_dir)+strlen(parameters[1]));
            	if(mycd == NULL)
                	printf("cd:malloc failed.\n");
            	strcpy(mycd,pwd->pw_dir);				//Turn ~ to /home/username
            	strncpy(mycd+strlen(pwd->pw_dir),parameters[1]+1,strlen(parameters[1]));
        	}
        	else {
            	mycd = malloc(strlen(parameters[1]+1));
            	if(mycd == NULL) {
                	printf("cd:malloc failed.\n");
            	}
            	strcpy(mycd,parameters[1]);
        	}
        	if(chdir(mycd)!= 0)
            	printf("-myShell: cd: %s:%s\n",mycd,strerror(errno));
            free(mycd);
        }

    	return 1;
    }
    return 0;
}

extern pid_t BPTable[MAXPIDTABLE];

/* A function that deals with background processes 
 * and used in signal(SIGCHLD,handleSig)
 */
void handleSig(int sig) {
    pid_t pid;
    for(int i = 0; i < MAXPIDTABLE; i++)
        if(BPTable[i] != 0) {       //only handle the background processes
            pid = waitpid(BPTable[i],NULL,WNOHANG);
            if(pid > 0) {
                printf("process %d exited.\n",pid);
                BPTable[i] = 0; 	//clear
            }
            else if(pid < 0) {
                if(errno != ECHILD)
                    perror("waitpid error");
            }
            //else:do nothing
            //Non background processses have their waitpid() in myshell
        }
    return;
}

/* A function that parse the parameters 
 * and assign proper values to struct parseInfo *info
 * The struct variable flag shows if background, redirection or pipe are needed
 * The struct variable inputfile stores the name of the file used in input redirection
 * The struct variable outputfile stores the name of the file used in output redirection
 * The struct variable command2 stores the second command if pipe is used
 * The struct variable parameter2 stores parameters belong to command2 if pipe is used
 */
int parse(char **parameters,int paraNum,struct parseInfo *info) {
    int i;
    info->flag = 0;
    info->inputfile = NULL;
    info->outputfile = NULL;
    info->command2 = NULL;
    info->parameters2 = NULL;
    if(strcmp(parameters[paraNum-1],"&") == 0) {		//Background process
        info->flag |= BACKGROUND;
        parameters[paraNum-1] = NULL;
        paraNum--;
    }
    for(i = 0; i < paraNum;) {
        if(strcmp(parameters[i],"<<")==0 || strcmp(parameters[i],"<")==0) { //Input redirection
            info->flag |= IN_REDIRECT;
            info->inputfile = parameters[i+1];
            parameters[i] = NULL;
            i+=2;
        }
        else if(strcmp(parameters[i],">")==0) {		//Output redirection(overlap)
            info->flag |= OUT_REDIRECT;
            info->outputfile = parameters[i+1];
            parameters[i] = NULL;
            i+=2;
        }
        else if(strcmp(parameters[i],">>")==0) {    //Output redirection(append)
            info->flag |= APPEND;
            info->outputfile = parameters[i+1];
            parameters[i] = NULL;
            i+=2;
        }
        else if(strcmp(parameters[i],"|")==0) {		//Pipe
            char *p;
            info->flag |= PIPE;
            parameters[i] = NULL;
            info->command2 = parameters[i+1];
            info->parameters2 = &parameters[i+1];
            for(p = info->parameters2[0]+strlen(info->parameters2[0]); p!= &(info->parameters2[0][0]) && *p!='/'; p--)
                ;
            if(*p == '/')
                p++;
            info->parameters2[0] = p;
            break;
        }
        else
            i++;
    }
	return 1;
}

/* A function that calls those functions defined above
 * and execuates the commmand stores in global variable buffer.
 * 
 * If the commmand is not a built-in command, then use fork() to
 * make a child process, which uses execvp(command,parameters) to
 * execuate non built-in commands.
 */
void run(char *buffer) {
    int status, i;
    char *command = NULL;
    extern char **parameters;
    extern char profile[5][1024];
    int paraNum;
    struct parseInfo info;
    pid_t ChdPid,ChdPid2;
    
    if(signal(SIGCHLD,handleSig) == SIG_ERR)				//Signal handler
        perror("signal() error");
    
    int pipe_fd[2],in_fd,out_fd;
    setEnviron(profile);
        
    paraNum = readCommand(&command,parameters,buffer);
    if(paraNum == -1)
        return;
    paraNum--;												//Number of units in the buffer
    parse(parameters,paraNum,&info);
    if(builtinCommand(command,parameters,profile,info))		//If it is a bulit-in command, then execuate it and return
        return;
    if(info.flag & PIPE) {									//If pipe is included, then command2 is not null            
        if(pipe(pipe_fd) < 0) {
            printf("myShell error:pipe failed.\n");
            exit(0);
        }
    }  
    if((ChdPid = fork())!= 0) {								//Use fork() to make a child process
        if(info.flag & PIPE) {
            if((ChdPid2=fork()) == 0) {						//Deal with command2
                close(pipe_fd[1]);
                close(fileno(stdin)); 
                dup2(pipe_fd[0], fileno(stdin));
                close(pipe_fd[0]); 
                execvp(info.command2,info.parameters2);		//Execute non built-in command2 using execvp()
            }
            else {
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                waitpid(ChdPid2,&status,0); 				//Wait for command2
            }
        }

        if(info.flag & BACKGROUND) {						//Not wait for background process
            printf("Child PID:%u\n",ChdPid);
            int i;
            for(i = 0; i < MAXPIDTABLE; i++)
                if(BPTable[i]==0) {
                    BPTable[i] = ChdPid; 					//Register a background process
                    break;
                }
        }
        else {          
            waitpid(ChdPid,&status,0);						//Wait for command1
        } 
    }
    else {													//Deal with command1
        if(info.flag & PIPE) {            
            if(!(info.flag & OUT_REDIRECT) && !(info.flag & APPEND)) {	//Only pipe is included
                close(pipe_fd[0]);
                close(fileno(stdout)); 
                dup2(pipe_fd[1], fileno(stdout));
                close(pipe_fd[1]);
            }
            else {											//Both output redirection and pipe are included
                close(pipe_fd[0]);
                close(pipe_fd[1]);							//Send a EOF to command2
                if(info.flag & OUT_REDIRECT)
                    out_fd = open(info.outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);		// >
                else
                    out_fd = open(info.outputfile, O_CREAT|O_APPEND|O_WRONLY, 0666);	// >>
                close(fileno(stdout)); 
                dup2(out_fd, fileno(stdout));
                close(out_fd);          
            }
        }
        else {
            if(info.flag & OUT_REDIRECT) {					//Only output redirection > is included
                out_fd = open(info.outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
                close(fileno(stdout)); 
                dup2(out_fd, fileno(stdout));
                close(out_fd);
            }
            if(info.flag & APPEND) {						//Only output redirection >> is included
                out_fd = open(info.outputfile, O_CREAT|O_APPEND|O_WRONLY, 0666);
                close(fileno(stdout)); 
                dup2(out_fd, fileno(stdout));
                close(out_fd);
            }
        }
            
        if(info.flag & IN_REDIRECT) {						//Input redirection is included
            in_fd = open(info.inputfile, O_CREAT |O_RDONLY, 0666);
            close(fileno(stdin)); 
            dup2(in_fd, fileno(stdin));
            close(in_fd); 
        }
        execvp(command,parameters);							//Execuate command1 using execvp()
    }
}