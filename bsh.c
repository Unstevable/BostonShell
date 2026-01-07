/* Author: Steven Carr
   Program Created: November 2023
A "shell" program (bsh stands for Boston shell). Intended to work like classic sh and bash.
Base-code was given for this program.  I was tasked specifically with coding the commands for:
env, setenv, unsetenv, cd, pwd and history.*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

//accept up to 16 command-line arguments
#define MAXARG 16

//allow up to 64 environment variables
#define MAXENV 64

//keep the last 500 commands in history
#define HISTSIZE 500

//accept up to 1024 bytes in one command
#define MAXLINE 1024

static char **parseCmd(char cmdLine[]) {
  char **cmdArg, *ptr;
  int i;

  //(MAXARG + 1) because the list must be terminated by a NULL ptr
  cmdArg = (char **) malloc(sizeof(char *) * (MAXARG + 1));
  if (cmdArg == NULL) {
    perror("parseCmd: cmdArg is NULL");
    exit(1);
  }
  for (i = 0; i <= MAXARG; i++) //note the equality
    cmdArg[i] = NULL;
  i = 0;
  ptr = strsep(&cmdLine, " ");
  while (ptr != NULL) {
    // (strlen(ptr) + 1)
    cmdArg[i] = (char *) malloc(sizeof(char) * (strlen(ptr) + 1));
    if (cmdArg[i] == NULL) {
      perror("parseCmd: cmdArg[i] is NULL");
      exit(1);
    }
    strcpy(cmdArg[ i++ ], ptr);
    if (i == MAXARG)
      break;
    ptr = strsep(&cmdLine, " ");
  }
  return(cmdArg);
}

// Function to print all the current environment variables
static void printEnv(char *environmentVariables[]){
    // Start at the first environment variable
    int environments = 0;

    // While the current environment variable is still not NULL, print the environment
    // variable, and increment to the next one until we reach the end (NULL)
    while (environmentVariables[environments] != NULL){
        printf("%s\n", environmentVariables[environments]);
        environments++;
    }
}

// Function to set/update the environment of an environment variable
static void setEnvFunc(char *environmentVariables[], char *envName, char *envValue){
    // Initialize a variable to keep track of what environment variable we are in
    int environments = 0;

    // While there are still environments that have a value
    while (environmentVariables[environments] != NULL){
        // The strstr() function finds a substring within a bigger string, it's almost like strcmp.
        // If strstr(currentEnv, envName) does not return NULL, then that means the environment we
        // are in and the one we are looking for matches and we need to update the already existing environment.
        if (strstr(environmentVariables[environments], envName) != NULL){
            // First, free the current environment variable because we want to update it
            free(environmentVariables[environments]);

            // Malloc new space for the environment variable; note that the "+ 2" here indicates space for the "="
            // inbetween the name and the value, and the NULL terminator after the value "\0"
            environmentVariables[environments] = (char *)malloc(strlen(envName) + strlen(envValue) + 2);
            // In case there was an issue in Mallocing the space, throw out error message
            if (environmentVariables[environments] == NULL){
                perror("Malloc in setenv failed.\n");
                exit(1);
            }

            // Otherwise, use sprintf to set the name of the environment and the value with the given parameters
            sprintf(environmentVariables[environments], "%s=%s", envName, envValue);
            // If we reach this case, we want to return to exit the function and NOT process the code outside of the loop
            return;
        }

        // If the environment variable that we're looking for was not found, increment to the next one
        environments++;
    }

    // If we reached here, then the environment variable is not present at all and must be added
    // Do the same Malloc process for the variable and use sprintf to set the name and value
    environmentVariables[environments] = (char *)malloc(strlen(envName) + strlen(envValue) + 2);
    if (environmentVariables[environments] == NULL){
        perror("Malloc in setenv failed.\n");
        exit(1);
    }
    sprintf(environmentVariables[environments], "%s=%s", envName, envValue);

    // We make the next environment variable NULL, because we want to make sure there's a NULL pointer
    // to represent the end of the environment variables
    environmentVariables[environments + 1] = NULL;
}

// Function to remove an environment variable
static void unSetEnvFunc(char *environmentVariables[], char *envName){
    // Initialize the starting point for out environment variables
    int environments = 0;

    // While we still have environment variables to process
    while (environmentVariables[environments] != NULL){
        // If the given environment variable exists within our current env variables
        if (strstr(environmentVariables[environments], envName)){
            // Free the memory for the given variable
            free(environmentVariables[environments]);

            // Now, we want to shift every variable one space to the left, so while
            // the next environment variable space is not NULL
            while (environmentVariables[environments + 1] != NULL){
                // Shift the variable to the left and increment to the next space
                environmentVariables[environments] = environmentVariables[environments + 1];
                environments++;
            }
            // Once we reach here, we are at the end of the variables, so make the last variable NULL
            environmentVariables[environments] = NULL;
            // Return to not process anymore code in the function
            return;
        }

        // Increment the environment variable space if we haven't found the given variable yet
        environments++;
    }
}

// Function to change the directory
static void changeDirectory(char **currentDirectory, const char *newDirectory, char *environmentVariables[]){
  // Initialize a variable for the old directory (currently our current) and the absolute path
  char *oldDirectory = *currentDirectory;
  char *absolutePath;

  // If the new directory begins with a '/', it represents an absolute path, copy it into the absolute path
  if (newDirectory[0] == '/'){
    absolutePath = strdup(newDirectory);
  } else {
    // Otherwise, malloc the memory for the absolute path, accounting for the length of the old and new directories
    // and the +2 accounts for '/' and the NULL terminator
    absolutePath = (char *)malloc(strlen(oldDirectory) + strlen(newDirectory) + 2);

    // If the absolute path is NULL, then the malloc failed, throw error message
    if (absolutePath == NULL){
      perror("Malloc in changeDirectory failed.\n");
      exit(1);
    }
    // Put the concatenation between the old and new directory into the absolute path
    sprintf(absolutePath, "%s/%s", oldDirectory, newDirectory);
  }

  // If chdir() returns 0, then it's a success; otherwise throw an error
  if (chdir(absolutePath) != 0){
    perror("chdir has failed; make sure the given directory exists.\n");
    free(absolutePath);
    return;
  }

  // Update the PWD environment variable using the current absolute path
  setEnvFunc(environmentVariables, "PWD", absolutePath);

  // Free the current directory and replace it with the absolute path
  free(*currentDirectory);
  *currentDirectory = strdup(absolutePath);

  // Free the absolute path now that the current directory is updated
  free(absolutePath);
}

// Function to print the working directory
static void printWorkingDirectory(char *currentDirectory){
  // If the current directory is not NULL, then print it
  // NOTE: I formatted the output for "pwd" based on how it looks via my Window's terminal.
  if (currentDirectory != NULL){
    printf("\nPath\n----\n");
    printf("%s\n", currentDirectory);
    printf("\n\n");
  } else {
    // Otherwise throw error message
    printf("There was an error printing the working directory.\n");
    exit(1);
  }
}

// Function to print the history of the input entered into the command-line
static void printHistory(char *history[], int size){
  // Starting from the beginning, if the current place in the history array is not null, print it
  for (int i = 0; i < size && history[i] != NULL; i++){
      // The i + 1 represents the normal digit number, so the first history shown is 1 instead of 0.
      printf("%d: %s\n", i + 1, history[i]);
  }
}

int main(int argc, char *argv[], char *envp[]) {
  char cmdLine[MAXLINE], **cmdArg;
  int status, i, debug;
  pid_t pid;

  // Initialize a copy of the environment variables and a variable to keep track of the count
  char *environmentVars[MAXENV + 1];
  int environmentCount = 0;
  while (envp[environmentCount] != NULL && environmentCount < MAXENV){
    // Copy the environment variables to a new array
    environmentVars[environmentCount] = strdup(envp[environmentCount]);
    environmentCount++;
  }
  // Set the last space in the new environment variable array to NULL to represent the end
  environmentVars[environmentCount] = NULL;

  debug = 0;
  i = 1;
  while (i < argc) {
    if (! strcmp(argv[i], "-d") )
      debug = 1;
    i++;
  }

  // Initialize the current directory to what the HOME environment variable is holding
  char *currentDirectory = strdup(getenv("HOME"));

  // Initialize an array to hold the history of the command-line, and a variable for the current index
  char *commandHistory[HISTSIZE];
  int historyIndex = 0;

  while (( 1 )) {
    // Make sure the prompt changes depending on what directory we're currently in
    if (currentDirectory != NULL){
      if (*currentDirectory == '/'){
        printf("bsh%s> ", currentDirectory);
      } else {
        printf("bsh/%s> ", currentDirectory);
      }
    } else {
      // Default prompt
      printf("bsh> ");
    }
    fgets(cmdLine, MAXLINE, stdin);       //get a line from keyboard
    cmdLine[strlen(cmdLine) - 1] = '\0';  //strip '\n'
    cmdArg = parseCmd(cmdLine);
    if (debug) {
      i = 0;
      while (cmdArg[i] != NULL) {
        printf("\t%d (%s)\n", i, cmdArg[i]);
        i++;
      }
    }

    //built-in command exit
    if (strcmp(cmdArg[0], "exit") == 0) {
      if (debug)
        printf("exiting\n");
      break;
    }

    //built-in command env
    else if (strcmp(cmdArg[0], "env") == 0) {
      if (cmdArg[1] != NULL){
        printf("Usage: env\n");
      } else {
        // Print the current environment variables
        printEnv(environmentVars);
      }
    }

    //built-in command setenv
    else if (strcmp(cmdArg[0], "setenv") == 0) {
        // If the command-line arguments after "setenv" are not null, call the setenv function
        if (cmdArg[1] != NULL && cmdArg[2] != NULL){
            setEnvFunc(environmentVars, cmdArg[1], cmdArg[2]);
        } else {
            // Otherwise print this command's usage
            printf("Usage: setenv <name> <value>\n");
        }
    }
    //built-in command unsetenv
    else if (strcmp(cmdArg[0], "unsetenv") == 0) {
        // If the next command-line argument is not NULL, then delete that variable
        if (cmdArg[1] != NULL){
            unSetEnvFunc(environmentVars, cmdArg[1]);
        } else {
            // Otherwise print this command's usage
            printf("Usage: unsetenv <name>\n");
        }
    }
    //built-in command cd
    else if (strcmp(cmdArg[0], "cd") == 0) {
      // If the next command-line argument is not NULL, then change the directory
      if (cmdArg[1] != NULL){
        changeDirectory(&currentDirectory, cmdArg[1], environmentVars);
      } else {
        // Otherwise print this command's usage
        printf("Usage: cd <directory>\n");
      }
    }

    //built-in command history
    else if (strcmp(cmdArg[0], "history") == 0) {
      // If there is another argument after history, print its usage
      if (cmdArg[1] != NULL) {
        printf("Usage: history\n");
      } else {
        // Otherwise, print the command-line history
        printHistory(commandHistory, historyIndex);
      }
    }

    //built-in command pwd
    else if (strcmp(cmdArg[0], "pwd") == 0){
      // If there is another argument after pwd, print its usage
      if (cmdArg[1] != NULL){
        printf("Usage: pwd\n");
      } else {
        // Otherwise, print the working directory
        printWorkingDirectory(currentDirectory);
      }
    }

    // We have to make sure we adjust the history array when the max size is reached
    // If the current history index is less than the size
    if (historyIndex < HISTSIZE){
      // Then copy the most recent command-line into the command history array and increment the index
      //commandHistory[historyIndex] = strdup(cmdLine);
      strcpy(commandHistory[historyIndex], cmdLine);
      historyIndex++;
    } else {
      // Otherwise, free the memory of the first array space
      free(commandHistory[0]);
      // Then shift every command-line history space one to the left
      for (int i = 1; i < HISTSIZE; i++){
        commandHistory[i - 1] = commandHistory[i];
      }
      // Now make the last space in the array the most recent command-line
      //commandHistory[HISTSIZE - 1] = strdup(cmdLine);
      strcpy(commandHistory[HISTSIZE - 1], cmdLine);
    }

    //clean up before running the next command
    i = 0;
    while (cmdArg[i] != NULL)
      free( cmdArg[i++] );
    free(cmdArg);
  }

  // Free the current directory
  free(currentDirectory);

  return 0;
}