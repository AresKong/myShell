#include "myshell.h"

id_t BPTable[MAXPIDTABLE];
char profile[5][1024];									//Store environment variables
char **parameters;										//Store parameters belong to a certain command
char exepath[MAX_PATH];

int main(int argc, char* argv[]) {
    for(int i = 0; i < MAXPIDTABLE; i++)
        BPTable[i] = 0;
    char *buffer;                                       //Store what user types
    buffer = malloc(sizeof(char) * MAXLINE);
    parameters = malloc(sizeof(char *)*(MAXARG+2));
    if(parameters == NULL || buffer == NULL) {
        printf("myShell error: malloc failed.\n");
        return 1;
    }
    char tmp0[MAX_PATH];
    getcwd(exepath,MAX_PATH);
    sprintf(tmp0,"SHELL=%s/myshell",exepath);		//profile[0] stores $SHELL, which is the execuate path of myshell
    strcpy(profile[0],tmp0);							
    sprintf(tmp0,"PARENT=%s/myshell",exepath);
    strcpy(profile[4],tmp0);

    if(argc == 2) {                                 //If user uses myshell xxxfile to start myshell, 
        char buf[1024];                             // then commmands are from xxxfile.  
        FILE *fp;
        if((fp = fopen(argv[1],"r")) == NULL) {
            perror("fail to open file");
            exit (1) ;
        }                                       
        while(fgets(buf,1024,fp) != NULL)           //Read commmands from xxxfile one line per time and store it in buf.
            run(buf);                               //Run the command stored in the buf
        fclose(fp);
        return 0;                                   //Exit myshell when the file reaches EOF
    }
    int shouldrun = 1;
    while(shouldrun) {                              //If the user doesn't use myshell xxxfile, then command is from stdin
        showPrompt();
        char* fget = fgets(buffer,MAXLINE,stdin);   //Store what the user types into variable buffer
        if(fget == NULL) {
            printf("\n");
            exit(0);
        }       
        if(buffer[0] == '\0')
            return 1;
        run(buffer);                                //Run the command stored in the buf
    }
    free(buffer);
    free(parameters);
	return 0;
}
