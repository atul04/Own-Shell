/**
 * @Author: Atul Sahay <atul>
 * @Date:   2018-07-28T20:36:08+05:30
 * @Email:  atulsahay01@gmail.com
 * @Filename: develope.c
 * @Last modified by:   atul
 * @Last modified time: 2018-08-10T02:28:05+05:30
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")

int backProList[150]; // stores the pid that are processing at daemon thread
int foreProList[150]; // stores the pid that are processing at prompt
int backCount =  0;
int commandCount = 0; // No. of commands in case of parallel or sequential
// History Directory
char historyDir[1024] = "/home/";

//################### Function Decalarations #############################

// adding the process to the background List
void add_back(pid_t pid);
//adding process to the foreground list
void add_fore(pid_t pid);
// freeing up the space acquired by the back process
void reap_back(pid_t wid);
// freeing up the space acquired by the foreground process
void reap_fore(pid_t wid);
// command adder that provide the whole command tokens List
char **add_command(char **tokens, int start, int end);
// commands splitter
char ***commands_split(char** tokens,bool *seq, int tokenCount, int* commandCount, bool *mismatch);
// zombie child handler .........
void child_handler(int sig);
// For killing Foreground processes SIGINT is passed
void kill_fg(int sig);
// For killing the Background processes
void kill_bg();
//parallel processing - for parrallel processing of the processes
pid_t executeParallel(char** tokens);
//to check whether the process is background or not
int isBackground(char** tokens, int tokenCount);
//Truncating the tokens list in case of recieving &
char **trun(char** args, int tokenCount);
//Exceute SHELL
int executeShell(char** tokens, int tokenCount);
//launch SHELL
int inShellCommands(char** args, int bg);
// Initial things like welcome note, lists initalizations are done here
void init_shell();
// For the change directory functionality  --- cd
int changeDir(char **args);
// History command
int history();
//tokenize the input string
char **tokenize(char *line);


////////////////////// Driver function ///////////////////////////////
int  main(void)
{
     int status; // Used for taking the retrun executeShell Function
     init_shell();

     // ***** For Histroy Creation *****************
     char *username = getenv("USER");

     strcat(historyDir,username);
     strcat(historyDir,"/history.txt");

     printf("history Dir : %s\n",historyDir);

     // ********************************************

     // Signals such as /^C and when a child has died
     signal(SIGCHLD,child_handler);
     signal(SIGINT,kill_fg);

    // ****************************************
     char  line[MAX_INPUT_SIZE];
     char  **tokens;
     int i;

     // For no. of commands we need this in parallel or sequential execution
     char *** commands;
     commandCount = 0;
     bool seq = false;
     bool mismatch = false;
     // ...........................................

     while (1) {
         int tokenCount = 0;   // takes the account of total count
         // Opening history directory for writing up all commands
         FILE *fp;
         fp = fopen (historyDir,"a");
         printf("Hello>");
         bzero(line, MAX_INPUT_SIZE);
         gets(line);
         //printf("Got command %s\n", line);
         line[strlen(line)] = '\n'; //terminate with new line
         tokens = tokenize(line);


        //printf("Back count: %d\n",backCount);


         //do whatever you want with the commands, here we just print them
         for(i=0;tokens[i]!=NULL;i++){
  	        //printf("found token %s\n", tokens[i]);
            tokenCount++;
            if(fp!=NULL){
              fprintf(fp,"%s ",tokens[i]);
            }
         }

         fprintf(fp,"\n");
         fclose(fp);
         //printf("Token Count : %d\n",tokenCount);

         if(tokenCount == 0)
            continue;
         else if(tokenCount == 1 && strcmp(tokens[0],"exit")==0)
         {
           //printf("%s\n","Hello m here" );
           int parent_status;
           pid_t pid = getpid();
           kill_fg(SIGINT);
           kill_bg(SIGCHLD);
           //child_handler(SIGKILL);

           while(backCount>0); // for reaping all the background processes

           // After Reaping freeing up the allocated tokens
           for(i=0;tokens[i]!=NULL;i++){
    	         free(tokens[i]);
           }
           free(tokens);
           // Done.........................................................
           //Terminate the parent process
           kill(pid,SIGTERM);
         }

         commands = commands_split(tokens,&seq,tokenCount,&commandCount,&mismatch);

         // mismatched expression ----- no execution
         if(mismatch){
            printf("Illegal Command: Mismatch Expressions\n");
         }
         else{
           // Counting No. of commands after split
           for(i=0; i<commandCount;i++){
              //printf("Found %d command\n:",i+1);
              int j = 0;
              for(j=0;commands[i][j]!=NULL;j++){
       	      //printf("%s ", commands[i][j]);
              }
              //printf("\n");
           }
           //printf("seq%d",seq);

           //------------------ For single command--------------
           if(commandCount == 1)
           {
              status = executeShell(tokens,tokenCount);
           }

           // If the given commands to be executed sequentially
           else if(seq == true)
           {
                // For every command fork child, execute, wait and then repeat
                for(i = 0 ; i < commandCount ; i++)
                {
                    /// Here we also check whether the command is background or Foreground

                    int tokenCountofCommand = 0,j=0;
                    for(j = 0;commands[i][j]!=NULL;j++)
                        tokenCountofCommand+=1;
                    if(tokenCountofCommand == 0)
                       continue;
                    else if(tokenCountofCommand == 1 && strcmp(commands[i][0],"exit")==0)
                    {
                      //printf("%s\n","Hello m here" );
                      int parent_status;
                      pid_t pid = getpid();
                      kill_fg(SIGINT);
                      kill_bg(SIGCHLD);
                      //child_handler(SIGKILL);

                      while(backCount>0); // for reaping all the background processes

                      // After Reaping freeing up the allocated tokens
                      for(i=0;tokens[i]!=NULL;i++){
               	         free(tokens[i]);
                      }
                      free(tokens);
                      for(i = 0;i<commandCount;i++)
                      {
                          int j= 0;
                          for(j = 0 ;commands[i][j]!=NULL;j++)
                              free(commands[i][j]);
                          free(commands[i]);
                      }
                      free(commands);
                      // Done.........................................................
                      kill(pid,SIGTERM);
                    }
                    status = executeShell(commands[i],tokenCountofCommand);
                }
           }

           // ------->>>>>>>For parallel execution
           else
           {
              // First fork all the child1
              //pid_t *pidList = (pid_t *)malloc(commandCount*sizeof(pid_t));
              i=0; // Normal iterator
              pid_t pid;
              do {
                pid = executeParallel(commands[i]);
                if(pid>0){
                  //printf("Started : %d\n",pid);
                    add_fore(pid);
                  }
                if(pid == 0){
                    if(execvp(commands[i][0], commands[i]) == -1){
                        printf("Illegal Command: %s\n",commands[i][0]);
                        exit(1);
                    }
                }
                i++;
              } while(i<commandCount && pid!=0);

              if(pid>0){
                  int cstatus;
                  for(i=0;i<commandCount;i++)
                  {
                    pid_t wid = waitpid(-1,&cstatus,0);
                    reap_fore(wid);
                  }
              }
            }


         }

         // status for the execute of shell commands
         //status = executeSsshell(tokens,tokenCount);


         // freeing the allocated memory


         for(i=0;tokens[i]!=NULL;i++){
  	         free(tokens[i]);
         }
         free(tokens);

         for(i = 0;i<commandCount;i++)
         {
             int j= 0;
             for(j = 0 ;commands[i][j]!=NULL;j++)
                 free(commands[i][j]);
             free(commands[i]);
         }
         free(commands);
     }

     return 1;
}



////////////////////// Functions Definitions ///////////////////////////////


// adding the process to the background List
void add_back(pid_t pid)
{
    int index= 0;
    backCount+=1;
    while(backProList[index]!=-1)
          index+=1;
    backProList[index] = pid;
}

//adding process to the foreground list
void add_fore(pid_t pid)
{
    int index= 0;
    while(foreProList[index]!=-1)
          index+=1;
    foreProList[index] = pid;
}

// freeing up the space acquired by the back process
void reap_back(pid_t wid)
{
    backCount-=1;
    int i = 0;
    if(wid >0)
    {
        printf("[%d] Stopped process Id : %d\nHello>",backCount,wid);

        //Freeing up the Free Space
        for(i = 0 ;  i < 150 ; i++){
              if(backProList[i] == wid)
              {
                backProList[i] = -1;
                break;
              }
        }
    }
}

// freeing up the space acquired by the foreground process
void reap_fore(pid_t wid)
{
    int i = 0;
    if(wid >0)
    {
        //printf("Foreground process completed successfull Id : %d\n",wid);

        //Freeing up the Free Space
        for(i = 0 ;  i < 150 ; i++){
              if(foreProList[i] == wid)
              {
                foreProList[i] = -1;
                break;
              }
        }
    }
    int cstatus;
    wid = waitpid(-1,&cstatus,WNOHANG);
    while(wid>0)
    {
      for(i = 0 ;  i < 150 ; i++){
            if(foreProList[i] == wid)
            {
              foreProList[i] = -1;
              break;
            }
      }
    wid = waitpid(-1,&cstatus,WNOHANG);
    }
}

// command adder that provide th whole command tokens List
char **add_command(char **tokens, int start, int end)
{
    char **args = (char **)malloc(MAX_NUM_TOKENS*sizeof(char *));
    int index = 0;
    while(start<=end){
        args[index] = (char *)malloc(MAX_TOKEN_SIZE*sizeof(char*));
        strcpy(args[index++],tokens[start++]);
        //printf("token: %s copied: %s\n",tokens[start-1],args[index-1]);
      }
    args[index] = NULL;
    return args;

}

// commands splitter
char ***commands_split(char** tokens,bool *seq, int tokenCount, int* commandCount, bool *mismatch)
{
    char*** commands = (char ***)malloc(MAX_NUM_TOKENS*sizeof(char **));
    //char** args = (char **)malloc(MAX_NUM_TOKENS*sizeof(char *));
    *commandCount = 0;
    int start = 0;
    int i = 0;
    for(i = 0 ; i < tokenCount ; i++)
    {
        if(strcmp(tokens[i],"&&") == 0)
        {
            *seq = true;
            break;
        }

        else if(strcmp(tokens[i],"&&&") == 0)
        {
            *seq = false;
            break;
        }
    }

    // For finding mismatch expressions (like combination of parallel and series)
    for(i = 0 ; i < tokenCount ; i++)
    {
        if(strcmp(tokens[i],"&&") == 0 && *seq==false)
        {
            *mismatch = true;
            break;
        }

        else if(strcmp(tokens[i],"&&&") == 0 && *seq ==true)
        {
            *mismatch = true;
            break;
        }
    }

    // If mismatched expressions call it off
    if(*mismatch)
      return NULL;

    // else continue executing things as normal

    for( i = 0 ; i < tokenCount ; i++)
    {
        //printf("Token: %s\n",tokens[i]);
        if(strcmp(tokens[i],"&&") == 0 || strcmp(tokens[i],"&&&") == 0)
        {
            commands[*commandCount] = add_command(tokens,start,i-1);
            *commandCount+=1;
            start = i+1;
        }
        //printf("Command Count %d\n",*commandCount);
    }
    commands[*commandCount] = add_command(tokens,start,i-1);
    *commandCount+=1;
    //printf("Command Count %d\n",*commandCount);
    return commands;
}

// zombie child handler .........
void child_handler(int sig)
{
    pid_t pid;
    int cstatus;
    int i = 0;
    while ((pid = waitpid(-1, &cstatus, WNOHANG)) > 0) {
       for(i = 0 ; i < 150 ; i++)
       {
         if(backProList[i] == pid)
         {
            reap_back(pid);
            break;
         }
       }

       for(i = 0 ; i < 150 ; i++)
       {
         if(foreProList[i] == pid)
         {
            reap_fore(pid);
            break;
         }
       }
       //printf("%s\n","Here in child" );
    }
}

// For killing Foreground processes SIGINT is passed
void kill_fg(int sig)
{
    int i = 0;
    int cstatus;
    //printf("%s\n","Hey I have recieved for the kill" );
    commandCount = 0; // If any sequential thing exist all processes will be killed
    // Linux shell bash shell also does the same thing
    for(i = 0 ; i < 150 ; i++)
    {
        if(foreProList[i]>0)
        {
            kill((pid_t)foreProList[i],SIGKILL);
            pid_t wid = waitpid((pid_t)foreProList[i],&cstatus,0);
            reap_fore(foreProList[i]);
        }
    }
}

// For killing the Background processes
void kill_bg()
{
  int i = 0;
  //printf("%s\n","Hey I have recieved for the kill" );
  for(i = 0 ; i < 150 ; i++)
  {
      if(backProList[i]>0)
      {
          //printf("%s\n","Hey");
          kill((pid_t)backProList[i],SIGKILL);
      }
  }
}

//parallel processing - for parrallel processing of the processes
pid_t executeParallel(char** tokens)
{
    int status;
    pid_t pid=-1;
    //int bg = isBackground(tokens,tokenCount);
    //printf("Background : %d\n",bg);

    /*if(bg == 1)
    {
      tokens = trun(tokens,tokenCount);
    }*/

    if(strcmp(tokens[0],"cd")==0)
        status = changeDir(tokens);
    else if(strcmp(tokens[0],"history")==0)
    {
        status = history();
    }
    else{
        pid = fork();
    }

    return pid;
}

//to check whether the process is background or not
int isBackground(char** tokens, int tokenCount)
{
    int last = 0;
    //printf("%c\n",tokens[tokenCount-1][last++]);
    while(tokens[tokenCount-1][last++]!='\0')
      //printf("%c\n",tokens[tokenCount-1][last++]);

    //printf("size: %d\n",last);
    if(strcmp(tokens[tokenCount-1],"&") == 0 || tokens[tokenCount-1][last-1] == '&')
      return 1;
    return 0;
}

//Truncating the tokens list in case of recieving &
char **trun(char** args, int tokenCount)
{
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    //char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));

    int i = 0, last = 0;
    while(args[tokenCount-1][last++]!='\0')

    if(strcmp(args[tokenCount-1],"&") == 0)
    {

        for(i = 0 ; i < tokenCount-1 ; i++)
        {
            tokens[i] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
            strcpy(tokens[i],args[i]);
            //printf("Trun ; %s\n",tokens[i]);
        }
    }
    else if(args[tokenCount-1][last-1] == '&')
    {
        for(i = 0 ; i < tokenCount ; i++)
        {
            tokens[i] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
            strcpy(tokens[i],args[i]);
            //printf("Trun ; %s\n",tokens[i]);
        }
        tokens[tokenCount-1][last-1] = '\0';
        //printf("Trun ; %s\n",tokens[i-1]);
    }

    return tokens;
}

//Exceute SHELL
int executeShell(char** tokens, int tokenCount)
{
    int status;
    int bg = isBackground(tokens,tokenCount);
    //printf("Background : %d\n",bg);

    if(bg == 1)
    {
      tokens = trun(tokens,tokenCount);
    }

    if(strcmp(tokens[0],"cd")==0)
        status = changeDir(tokens);
    else if(strcmp(tokens[0],"history")==0)
    {
        status = history();
    }
    else{
        status = inShellCommands(tokens,bg);
    }

    return status;
}

//launch SHELL
int inShellCommands(char** args, int bg)
{
    pid_t pid, wid;
    int status;

    pid = fork();
    if(pid == 0)
    {//child process
      if(execvp(args[0], args) == -1)
      {
        printf("Illegal Command: %s\n",args[0]);
        exit(1);
      }
    }
    else if(pid<0)
    {
      //Error forking
      perror("shell");
    }
    else
    {
      //printf("Process Id :%d\n",pid);
      // parent process
      setpgid(pid,0);
      if(bg == 1)
      {
        add_back(pid);
      }

      else
      {
        add_fore(pid);
      }

      bool inBack = false;

      int i = 0;

      for(i = 0; i < 150 ; i++)
      {
        if(backProList[i] == pid)
          {
            inBack = true;
            break;
          }
      }

      int cstatus;

      if(inBack == true)
      {
        //printf("here\n");

      }

      else
      {
        wid = waitpid(pid,&cstatus,0);
        reap_fore(wid);
      }
    }

    //printf("pid %d wid %d\n",pid,wid);

    return 1;
}


// Greeting shell during startup
void init_shell()
{
    // Memory allocation for the background processes
    int i = 0;
    for(i = 0 ; i < 150 ; i++)
        backProList[i] = -1;
    // Memory allocation for the foreground processes
    for(i = 0 ; i < 150 ; i++)
        foreProList[i] = -1;

    clear();
    printf("\n\n\n\n******************************************");
    printf("\n\n\n\t****SAHAY SHELL****\n\n\n");
    printf("***1.\tls \n");
    printf("***2.\tcat \n");
    printf("***3.\techo \n");
    printf("***4.\tsleep \n");
    printf("***5.\tcd \n");
    printf("***6.\tps \n");
    printf("***7.\thistory \n");
    printf("***8.\tBackground execution\t& \n");
    printf("***9.\tParallel execution\t&&&\n");
    printf("***10.\tSequential execution\t&&\n");
    printf("***11.\texit");
    printf("\n\n\n\n*******************"
        "***********************");

    char* username = getenv("USER");
    printf("\n\n\nUSER is: @%s", username);
    printf("\n");
    sleep(2);
    clear();
}

// For the change directory functionality  --- cd
int changeDir(char **args)
{
  if(args[1] == NULL)
  {
    fprintf(stderr, "Syntax Error : expected arguement after cd \n" );
  }
  else {
    if(chdir(args[1]) == 0){
    char cwd[1024];
    getcwd(cwd,sizeof(cwd));
    printf("Directory has been changed to: %s\n",cwd);

  }
    else
    fprintf(stderr, "Syntax Error : error path \n" );
  }
}

// History command
int history()
{
    FILE *fp;
    fp = fopen(historyDir,"r+");

    if(fp!=NULL)
    {
      int count = 1;
      char pre,curr;
       printf("\n\n*********History Of Commands***********\n\n");
       printf("%d. ",count++);
       pre = fgetc(fp);
       while((curr = fgetc(fp)) != EOF && pre!=EOF){
         if(pre == '\n' && curr!=EOF){
             printf("\n");
             printf("%d. ",count++);
           }
         else
             printf("%c", pre);
         pre = curr;
       }
       printf("\n\n***************END****************\n\n");
    }
    fclose(fp);
}

//tokenize the input string
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
	strcpy(tokens[tokenNo++], token);
	tokenIndex = 0;
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }

  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}
