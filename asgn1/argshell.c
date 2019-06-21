#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <sys/stat.h>

#define MAXDIRSIZE 256
#define SPLITSTRSIZE 50

/* Sources
https://www.geeksforgeeks.org
https://www.geeksforgeeks.org/making-linux-shell-c/
https://github.com/rehassachdeva/C-Shell/blob/master/parse.c
*/

/*--------------------FCNS-----------------*/

// set the command flag
int checkArgs(char** parsedArgs, char* *fName, int *fileDesc, int *operatorPos)
{
    // check for any pipes
    for (int i = 0; parsedArgs[i] != NULL; i++)
    {
        if ( parsedArgs[i] != NULL && strcmp(parsedArgs[i], "|") == 0 && parsedArgs[i+1] != NULL)
        {
            return 2;
        }
    }
    
    // otherwise, if no exit or nothing, is simple command
    if (parsedArgs[0] != NULL && strcmp (parsedArgs[0], "exit") != 0)
    {
        return 1;
    }
    
    return 0;
}

// set the file descriptor based on operator
void setFileDescriptors(char** parsedArgs, int parsedArgsLength, char* *fName, int *fileDesc, int *operatorPos)
{
    
    for (int i = 0; parsedArgs[i] != NULL; i++)
    {
        // for pipes, input and output redirect
        if ( parsedArgs[i] != NULL && strcmp(parsedArgs[i], "|") == 0 && parsedArgs[i+1] != NULL)
        {
            *operatorPos = i;
        }
	    else if ( parsedArgs[i] != NULL && (strcmp(parsedArgs[i], ">>") == 0 || strcmp(parsedArgs[i], ">") == 0) && parsedArgs[i+1] != NULL )
	    {
            *fName = parsedArgs[i+1];

            if ( strcmp(parsedArgs[i], ">>") == 0 )
            {
                *fileDesc = open(*fName, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
            }
            else if ( strcmp(parsedArgs[i], ">") == 0 )
            {
                *fileDesc = open(*fName, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
            } 
            else
            {
                // should never happen
                printf("major error\n");
            }

            if (*fileDesc < 0)
            {
                printf("error opening the file %s\n", *fName);
                return;
            }

            *operatorPos = i;
            
	    }
	    else if ( parsedArgs[i] != NULL && strcmp(parsedArgs[i], "<") == 0 && parsedArgs[i+1] != NULL)
	    {
            *fName = parsedArgs[i+1];
            *fileDesc = open(*fName, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);
            if (*fileDesc < 0)
            {
                printf("error opening file %s\n", *fName);
                return;
            }

            *operatorPos = i;
            
	    }
    }

}

// passing in pointers to many vars to modify the vars in the fcn
// only need 1 fork
void execSimpleCmd(char** pArgs, int parsedArgsLength, char* *fName, int *fileDesc, int *savedStdout, int *operatorPos, char* savedDir, char* currDir, int forkV, int retCode, int* stat)
{
    /*
    Check for cd commands
    */
    
    if ( strcmp(pArgs[0], "cd") == 0 && parsedArgsLength == 1 )
	{
	    if (chdir(savedDir) != 0)
            perror("chdir() to shell dir failed\n");
	}
	else if ( strcmp(pArgs[0], "cd") == 0 && parsedArgsLength == 2 )
	{
	    if (chdir(pArgs[1]) != 0)
            perror("chdir() to directory failed\n");
	}
    
    /*
    End of cd commands
    */
    else
    {
        forkV = fork();
        if (forkV == -1)
        {
            printf("fork() error\n");
            return;
        }
        
        if (forkV == 0)
        {
                // child

                if ( (pArgs[1] != NULL && pArgs[2] != NULL) && (  (strcmp(pArgs[1], ">>") == 0 || strcmp(pArgs[1], ">") == 0)  || (strcmp(pArgs[2], ">>") == 0 || strcmp(pArgs[2], ">") == 0)   ) )
                {

                    pArgs[*operatorPos] = NULL;
                    
                    // stdout file desc == 1
                    close(1);
                    
                    dup2(*fileDesc, 1);	
            
                    retCode = execvp(pArgs[0], pArgs);
                    
                    if (retCode == -1)
                    {
                        // restore stdout to print error
                        dup2(*savedStdout, 1);	
                        printf("execvp() error\n");
                        close(*savedStdout);
                        return;
                    }
                    
                    close(*fileDesc);

                }
                else if ( (pArgs[1] != NULL && pArgs[2] != NULL) && ( strcmp(pArgs[1], "<") == 0 || strcmp(pArgs[2], "<") == 0   ) )
                {
                    pArgs[*operatorPos] = NULL;
                    
                    // stdin file desc == 0
                    close(0);
                    dup2(*fileDesc, 0);

                    retCode = execvp(pArgs[0], pArgs);

                    if (retCode == -1)
                    {
                        printf("execvp() error\n");
                        return;
                    }
                    
                    close(*fileDesc);

                }
                else
                {
                    // execute command normally
                    retCode = execvp(pArgs[0], pArgs);
                
                    if (retCode == -1)
                    {
                        printf("execvp() error\n");
                        return;
                    }
                    //exit(0); not needed, execvp exits
                }
                
            }
            else
            {
                // parent
                wait(stat);
            }
    }
}

// parse = split the argument into 2 string arrays, 1 for read and 1 for write

int parsePipe1(char** pArgs, int parsedArgsLength, int operatorPos, char** cmd1)
{
    char* parsedCmd1[parsedArgsLength];
    
    // counter for the split string array
    int parseCounter = 0;
    
    int i;
    for (i = 0; (pArgs[i] != NULL && strcmp(pArgs[i], "|") != 0); i++)
    {
        parsedCmd1[parseCounter] = pArgs[i];
        if (sizeof(parsedCmd1[parseCounter]) > sizeof(cmd1[i]))
        {
            printf("Error! Size of args bigger than space of split args 1\n");
            return -1;
        }
        strcpy(cmd1[parseCounter], parsedCmd1[parseCounter]);
        parseCounter++;
    }
    
    return parseCounter;
    
}

int parsePipe2(char** pArgs, int parsedArgsLength, int operatorPos, char** cmd2)
{
    char* parsedCmd2[parsedArgsLength];
    
    // counter for 2nd split string
    int parseCounter = 0;
    
    int i;
    for (i = operatorPos+1; (pArgs[i] != NULL && strcmp(pArgs[i], "|") != 0); i++)
    {
        parsedCmd2[parseCounter] = pArgs[i];
        if (sizeof(parsedCmd2[parseCounter]) > sizeof(cmd2[i]))
        {
            printf("Error! Size of args bigger than space of split args 2\n");
            return -1;
        }
        strcpy(cmd2[parseCounter], parsedCmd2[parseCounter]);
        parseCounter++;
    }
    
    return parseCounter;
}



// need 2 forks
void execPipedCmd(char** pArgs, int* pipeDescs, int *operatorPos, int forkV, int retCode, int* stat, char** cmd1, char** cmd2)
{
    if ( (pArgs[1] != NULL && pArgs[2] != NULL) && (strcmp(pArgs[1], "|") == 0 || strcmp(pArgs[2], "|") == 0) )
    {
                
        if (pipe(pipeDescs) == -1)
        {
            perror("pipe() error\n");
        }
        
        forkV = fork();
	    if (forkV == 0)
	    {
            // child process 1
                
            // close stdout
            close(1);
            // stdout becomes desc[1]
            dup2(pipeDescs[1], 1);
            // close reading part of pipe
            close(pipeDescs[0]);
            // exec
            if (pArgs[*operatorPos] != NULL && strcmp(pArgs[*operatorPos], "|") == 0)
            {                
                pArgs[*operatorPos] = NULL;
                retCode = execvp(pArgs[0], cmd1);
            }
            else
            {
                printf("error in number of arguments for pipe\n");
                return;
            }
            // close writing part of pipe
            close(pipeDescs[1]);
                
            if (retCode == -1)
            {
                printf("execvp() error\n");
                return;
            }
        }
	    else
	    {
            // parent process
            close(pipeDescs[1]);
            wait(stat);
	    }
        
        forkV = fork();
        if (forkV == 0)
        {
            // child 2
            
            // close stdin
            close(0);
            // stdin becomes desc[0]
            dup2(pipeDescs[0], 0);
            // close writing part of pipe
            close(pipeDescs[1]);
            //retCode = execlp(pArgs[*operatorPos+1], pArgs[*operatorPos+1], NULL);
            retCode = execvp(pArgs[*operatorPos+1], cmd2);
            // close reading part of pipe
            close(pipeDescs[0]);
                    
            if (retCode == -1)
            {
                printf("execlp() error\n");
                return;
            }
            
        }
        else
        {
            // parent process
            close(pipeDescs[0]);
            wait(stat);
        }
    }    
}

/*------------------MAIN--------------------*/

extern char ** get_args();

int
main()
{
    int         i;
    char **     args;

    int argLength = 0;

    // save the current directory
    char cwd[MAXDIRSIZE];

    char shellDir[MAXDIRSIZE];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd() error\n");
    }
    else
    {
        strcpy(shellDir, cwd);
    }

    // for commands
    int returnCode;

    int forkVal;

    // for exit()
    int* status = NULL;

    // for opening file and redirection
    int file_desc; 
    char* fileName;
    int saved_stdout;
    int saved_stdin;
    
    // for pipes
    
    /* file descriptors for pipes,
       descs[1] = writing over the pipe, stdout
       descs[0] = reading over the pipe, stdin */
    int pipe_descs[2];

    // position = index of operator symbol, ex. > or |, same as operatorPos
    int operatorIndex = -1;
    
    /* cmdFlag == 0 no command
       cmdFlag == 1 regular command
       cmdFlag == 2 piped command */
    int cmdFlag = 0;
    
    // index of end of split pipe string arrays for null char
    int splitPipeCmdEnd;
    
    // to parse args
    // by default assume 1 command entered
    int numCmds = 1;
    // counter for the split = parsed arg, same as splitArgsLength
    int parseCounter = 0;

    while (1) {
	printf ("Command ('exit' to quit): ");
	args = get_args();
	for (i = 0; args[i] != NULL; i++) {
	    argLength = i + 1;
	}
    
    // two string arrays when splitting pipe
    // size has to be less than argument itself
    char* pipeCmd1[argLength];
    char* pipeCmd2[argLength];
    
    // allocate, each string of size SPLITSTRSIZE chars + null char
    for (int i = 0; i < argLength; i++)
    {
        pipeCmd1[i] = malloc(sizeof(char) * SPLITSTRSIZE + 1);
        pipeCmd2[i] = malloc(sizeof(char) * SPLITSTRSIZE + 1);
    }
    //printf("allocated %lu and %lu bits for pipecmd1 and 2\n", sizeof(pipeCmd1), sizeof(pipeCmd2));
    
    // parse args
    char* splitArgs[argLength];
    
    // parse args and execute split args, one at a time
    // = break down multiple commands separated by semicolon
    if (args[0] != NULL && strcmp (args[0], "exit") != 0)
    {
        
        // reset numCmds
        numCmds = 1;
        
        // initialize string array which holds split args
        for (int i = 0; i < argLength; i++)
        {
            splitArgs[i] = NULL;
        }
        
        int i;
        for (i = 0; i < argLength; i++)
        {
            if (strcmp(args[i], ";") != 0)
            {
                splitArgs[parseCounter] = args[i];
                parseCounter++;
            }
            else
            {
                // multiple cmds except last
                numCmds++;
                splitArgs[parseCounter] = NULL;
                // exe
                cmdFlag = checkArgs(splitArgs, &fileName, &file_desc, &operatorIndex);
                setFileDescriptors(splitArgs, parseCounter, &fileName, &file_desc, &operatorIndex);
                if (cmdFlag == 1)
                {
                    execSimpleCmd(splitArgs, parseCounter, &fileName, &file_desc, &saved_stdout, &operatorIndex, shellDir, cwd, forkVal, returnCode, status);
                }
                else if (cmdFlag == 2)
                {
                    splitPipeCmdEnd = parsePipe1(splitArgs, parseCounter, operatorIndex, pipeCmd1);
                    pipeCmd1[splitPipeCmdEnd] = NULL;
                    splitPipeCmdEnd = parsePipe2(splitArgs, parseCounter, operatorIndex, pipeCmd2);
                    pipeCmd2[splitPipeCmdEnd] = NULL;
                    execPipedCmd(splitArgs, pipe_descs, &operatorIndex, forkVal, returnCode, status, pipeCmd1, pipeCmd2);
                }
                // end exe
                
                parseCounter = 0;
                
                // reset
                for (int i = 0; i < parseCounter; i++)
                {
                    splitArgs[i] = NULL;
                }
            }
        }
        
        // the last command of multi cmd won't end in NULL
        if (i == argLength && numCmds > 1)
        {
            // multi cmds, last cmd
            splitArgs[parseCounter] = NULL;
            // exe
            cmdFlag = checkArgs(splitArgs, &fileName, &file_desc, &operatorIndex);
            setFileDescriptors(splitArgs, parseCounter, &fileName, &file_desc, &operatorIndex);
            if (cmdFlag == 1)
            {
                execSimpleCmd(splitArgs, parseCounter, &fileName, &file_desc, &saved_stdout, &operatorIndex, shellDir, cwd, forkVal, returnCode, status);
            }
            else if (cmdFlag == 2)
            {
                splitPipeCmdEnd = parsePipe1(splitArgs, parseCounter, operatorIndex, pipeCmd1);
                pipeCmd1[splitPipeCmdEnd] = NULL;
                splitPipeCmdEnd = parsePipe2(splitArgs, parseCounter, operatorIndex, pipeCmd2);
                pipeCmd2[splitPipeCmdEnd] = NULL;
                execPipedCmd(splitArgs, pipe_descs, &operatorIndex, forkVal, returnCode, status, pipeCmd1, pipeCmd2);
            }
            // end exe
            
            parseCounter = 0;
            
        }
        
        // 1 cmd, take args and argLength
        if (numCmds == 1)
        {
            // exe normally
            cmdFlag = checkArgs(args, &fileName, &file_desc, &operatorIndex);
            setFileDescriptors(args, argLength, &fileName, &file_desc, &operatorIndex);
            if (cmdFlag == 1)
            {
                execSimpleCmd(args, argLength, &fileName, &file_desc, &saved_stdout, &operatorIndex, shellDir, cwd, forkVal, returnCode, status);
            }
            else if (cmdFlag == 2)
            {
                splitPipeCmdEnd = parsePipe1(args, argLength, operatorIndex, pipeCmd1);
                pipeCmd1[splitPipeCmdEnd] = NULL;
                splitPipeCmdEnd = parsePipe2(args, argLength, operatorIndex, pipeCmd2);
                pipeCmd2[splitPipeCmdEnd] = NULL;
                execPipedCmd(args, pipe_descs, &operatorIndex, forkVal, returnCode, status, pipeCmd1, pipeCmd2);
            }
            // end exe
            
            // reset parseCounter for next args line
            parseCounter = 0;
        }
        
    }
 
    /* exe by itself, uncomment this block and comment parsed args if block to test
    cmdFlag = checkArgs(args, &fileName, &file_desc, &operatorIndex);
    
    setFileDescriptors(args, argLength, &fileName, &file_desc, &operatorIndex);
    if (cmdFlag == 1)
    {
        execSimpleCmd(args, argLength, &fileName, &file_desc, &saved_stdout, &operatorIndex, shellDir, cwd, forkVal, returnCode, status);
    }
    else if (cmdFlag == 2)
    {
        //parsePipe(args, argLength, operatorIndex, pipeCmd1, pipeCmd2);
        
        splitPipeCmdEnd = parsePipe1(args, argLength, operatorIndex, pipeCmd1);
        // set the last string to null to terminate exec
        pipeCmd1[splitPipeCmdEnd] = NULL;
        
        splitPipeCmdEnd = parsePipe2(args, argLength, operatorIndex, pipeCmd2);
        // set the last string to null to terminate exec
        pipeCmd2[splitPipeCmdEnd] = NULL;
        
        execPipedCmd(args, pipe_descs, &operatorIndex, forkVal, returnCode, status, pipeCmd1, pipeCmd2);
    }
    */
    
    // args will be unchanged, fcns get a copy of args
    
	if (args[0] == NULL) {
	    printf ("No arguments on line!\n");
	} else if ( !strcmp (args[0], "exit")) {
	    printf ("Exiting...\n");
        
        // free allocated memory on exit
        // printf("freeings %lu and %lu bits for pipecmd1 and 2\n", sizeof(pipeCmd1), sizeof(pipeCmd2));
        for (int i = 0; i < argLength; i++)
        {
            free(pipeCmd1[i]);
            free(pipeCmd2[i]);
        }
        
        // these shouldn't be needed
	    //free(cwd);
        //free(shellDir);
	    //free(cmd);
        
	    exit(0);
	}
    
    // free allocated memory
    //printf("freeing %lu and %lu bits for pipecmd1 and 2\n", sizeof(pipeCmd1), sizeof(pipeCmd2));
    for (int i = 0; i < argLength; i++)
    {
        free(pipeCmd1[i]);
        free(pipeCmd2[i]);
    }
	
  }
}
