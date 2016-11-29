#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

struct command_t {
  char name[1024];
  int argc;
  char *argv[1024];
};


bool is_empty(const char *str) {
  while (*str != '\0') {
    if (!isspace(*str)) return false;
    str++;
  }
  return true;
}

void rstrip(char *str) {
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
      *end = '\0';
      end--;
    }
}

int main (int argc, char *argv[]){
  //variable initialization
  struct command_t *command; //Holds parsed command
  char *commandLine;     //Holds command before being parsed
  char *shellPath;           //Holds SHELLPATH environment variable
  char *commandPath[1024];        //Holds the tokenized paths
  char *tempCmd;             //Holds a path to be cat with command and exec'd
  int LENGTH=513;            //Maximum length of command
  FILE *inputSrc=stdin;      //Changes to batch file name in batch mode
  char *checkEOF;	     //Set to NULL if Ctrl-D is pressed
  int error=-1;              //Used to find valid path to command
  bool echoLine=0;           //If in batch mode and VERBOSE is set, echoLine=1
  bool execute = 1;          //Set to false if command should no longer be executed
  static bool TRUE=1;
  static char PROMPT[]="koblensk> "; //Prompt to appear for user

  //Shell Initialization

  //Checks whether to run in batch mode and echo commands
  if (argc == 1){

  } else if (argc == 2) {
      inputSrc=fopen(argv[1], "r");
      if (inputSrc == NULL) {
  puts("\nFile not found.");
  puts("Check your file name and path.\n");
	exit(0);
      } //if
      if (getenv("VERBOSE") != NULL) {
	echoLine=1;
      } //if

  } else {
    puts("\nToo many arguments for shell command.");
    puts("To run in batch mode use: shell <filename>.\n");
    exit(0);
  } //if - else if - else

  //Get SHELLPATH
  if (getenv("SHELLPATH") == NULL) {
    shellPath = NULL;

  } else {
      shellPath = (char *) malloc((strlen(getenv("SHELLPATH"))+1)*sizeof(char));
      strcpy(shellPath, getenv("SHELLPATH"));
  } //else

    //Find the full pathname for the file and execute command
    int pathCount = 0;

    if (shellPath != NULL) {

      commandPath[0] = (char *) \
	  malloc(LENGTH*sizeof(char));
      char *pathTmp = strtok(shellPath, ":");
      if (pathTmp) {
	  strcpy(commandPath[0], pathTmp);
      } else {
	  commandPath[0] = NULL;
	  printf("OOPS\n");
	  exit(1);
      }

      while (commandPath[pathCount] != NULL) {
	pathCount++;

	char *sTmp = strtok(NULL, ":");
	if (sTmp) {
	    commandPath[pathCount] = (char *) malloc(256*sizeof(char));
	    strcpy(commandPath[pathCount], sTmp);
	} else {
	    commandPath[pathCount] = NULL;
	}
      } //while

      //temp = strlen(commandPath[pathCount-1]) - 1;
      //commandPath[pathCount-1][temp] = '\0';
      commandPath[pathCount] = NULL;
    } //if

  //Main Loop
  while (TRUE) {
    printf("%s", PROMPT);

    //Read the command line and check for errors
    commandLine = (char *) malloc(LENGTH*sizeof(char));
    checkEOF=fgets(commandLine, LENGTH, inputSrc);
    printf("Command Line : %s", commandLine);
    rstrip(commandLine);

    if (checkEOF == NULL){
      printf("\n");
      exit(0);

    }
    if (is_empty(commandLine)) continue;
    if (strlen(commandLine) >= (unsigned)LENGTH-1) {
      puts("\nInput exceeds valid command length.");
      puts("Input must be at most 512 characters.");
	while(strlen(fgets(commandLine, LENGTH, inputSrc)) >= (unsigned)LENGTH-1);
	commandLine[0] = '\n';
    } //else if

    if (echoLine) {
      printf("%s\n", commandLine);
    } //if

    command  = (command_t *) malloc(sizeof(command));
    if (command == NULL) {
	fprintf(stderr, "ran out of mem\n");
	exit(1);
    }

    //Parse the executable and arguments from the command
    command->argc = 0;
    ///    command->argv = (char **) malloc(3 * sizeof(char *));
    command->argv[0] = (char *) malloc(LENGTH * sizeof(char));
    strcpy(command->argv[0], strtok(commandLine, " "));
    printf("   0 : %s\n", command->argv[0]);


    char *sTmp = strtok(NULL, " ");
    int cnt = 1;

    while (sTmp) {
	command->argv[cnt] = (char *) \
	    malloc((LENGTH * sizeof(char)));
	strcpy(command->argv[cnt], sTmp);

	printf("   %d : %s\n",
	       cnt,
	       command->argv[cnt]);

	cnt++;

	// do again.
	sTmp = strtok(NULL, " ");

    } //while

    command->argc = cnt;
    command->argv[cnt] = NULL;


    cnt = 0;
    while (command->argv[cnt]) {
	printf("  %d arg %s\n", cnt, command->argv[cnt]);
	cnt++;
    }

    //int temp = strlen(command->argv[command->argc-1])-1;
    //command->argv[command->argc-1][temp] = '\0';
    // command->argv[command->argc] = NULL;
    printf("%s\n", command->argv[0]);
    strcpy(command->name, command->argv[0]);
    printf("%s\n", command->name);


    if (!strcmp(command->name, "exit")){

      if (command->argc == 1) {
	exit(0);
      } else {
        puts("\nexit has too many arguments.");
        puts("Type \"exit\" or press Ctrl-D to exit.\n");
	  execute = 0;
      } //else
    } //if

    if (execute) {
      int i = 0;
      int pid, rc;
      rc = fork();
      if (rc == 0) {
	tempCmd = (char *) malloc((strlen(command->name)+1)*sizeof(char));
	strcpy(tempCmd, command->name);
	while (error == -1){
	  printf("Doing execv %s\n", tempCmd);
	  int cnt = 0;
	  while (command->argv[cnt]) {
	      printf("  %d %s\n", cnt, command->argv[cnt]);
	      cnt++;
	  }
	  execv(tempCmd, command->argv);

	  if (commandPath[i] != NULL){
	    tempCmd = (char *) realloc(tempCmd, (strlen(commandPath[i])+1)*sizeof(char));
	    strcpy(tempCmd, commandPath[i]);
	    strcat(tempCmd, "/");
	    strcat(tempCmd, command->name);
	    i++;

	  } else {
      puts("\nCommand Not Found.");
	    puts("Please check your path and filename.");
	    puts("Note: programs needing user input cannot be executed with this shell.\n");
	      exit(0);
	  } //else

	} //while

      } else {
	  pid = wait(&rc);
      } //else

    } //if

    //free(shellPath);
    //free(command->name);

    for (int i = 0; i <= command->argc; i++) {
	//free(command->argv[command->argc]);
    } //for

    //free(command->argv);

    for (int i = 0; i <= pathCount; i++) {
	//free(commandPath[i]);
    } //for

    //free(commandPath);
    //free(tempCmd);
  }
}
