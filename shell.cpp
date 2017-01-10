#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PROMPT "koblensk> "
#define MAX_COMMAND_LENGTH 512

struct command_t {
  int argc;
  char **argv;
};

bool is_empty(const char *str) {
  while (*str != '\0') {
    if (!isspace(*str))
      return false;
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

FILE *get_input_source(int argc, const char *argv[]) {
  if (argc == 1) {
    return stdin;
  } else if (argc == 2) {
    FILE *input_src = fopen(argv[1], "r");
    if (input_src == NULL) {
      puts("\nFile not found.");
      puts("Check your file name and path.\n");
      exit(EXIT_FAILURE);
    }
    return input_src;
  } else {
    puts("\nToo many arguments for shell command.");
    puts("To run in batch mode use: shell <filename>.\n");
    exit(EXIT_FAILURE);
  }
}

int count_char(const char *str, char ch) {
  int count = 0;
  for (; *str; str++) {
    if (*str == ch) count++;
  }
  return count;
}

char **parse_shell_path() {
  char *shell_path = getenv("SHELLPATH");
  if (shell_path == NULL) return NULL;

  int path_count = count_char(shell_path, ':') + 1;
  char **command_paths = (char **)malloc((path_count + 1) * sizeof(char *));
  if (command_paths == NULL) {
    puts("ERROR: Out of memory in parse_shell_path() allocating command_paths");
    exit(EXIT_FAILURE);
  }

  int i = 0;
  for (char *path = strtok(shell_path, ":"); path != NULL; path = strtok(NULL, ":"), i++) {
    command_paths[i] = (char *)malloc(strlen(path) * sizeof(char));
    if (command_paths[i] == NULL) {
      printf("ERROR: Out of memory in parse_shell_path() allocating command_paths[%d]\n", i);
      exit(EXIT_FAILURE);
    }
    strcpy(command_paths[i], path);
  }

  command_paths[i] = NULL;

  return command_paths;
}

char *get_command_line_from(FILE *input_src) {
  char *command_line = (char *)malloc((MAX_COMMAND_LENGTH + 1) * sizeof(char));
  if (fgets(command_line, (MAX_COMMAND_LENGTH + 1), input_src) == NULL) {
    printf("\n");
    exit(EXIT_SUCCESS);
  }

  printf("Command Line : %s", command_line);
  if (strlen(command_line) >= (unsigned)MAX_COMMAND_LENGTH) {
    puts("\nInput exceeds valid command length.");
    printf("Input must be at most %d characters.\n", MAX_COMMAND_LENGTH);
    while (strlen(fgets(command_line, (MAX_COMMAND_LENGTH + 1), input_src)) >=
            (unsigned)MAX_COMMAND_LENGTH) ;
    return NULL;
  }

  rstrip(command_line);
  if (is_empty(command_line)) return NULL;

  return command_line;
}

void echo_command_from(const FILE *input_src, const char *command_line) {
  if (input_src != stdin && getenv("VERBOSE") != NULL) {
    printf("%s\n", command_line);
  }
}

struct command_t *parse_command_line(char *command_line) {
  struct command_t *command = (command_t *)malloc(sizeof(command));
  if (command == NULL) {
    fprintf(stderr, "ERROR: Out of memory in parse_command_line() allocating command\n");
    exit(EXIT_FAILURE);
  }

  command->argc = count_char(command_line, ' ') + 1;
  command->argv = (char **)malloc((command->argc + 1) * sizeof(command->argv));
  if (command->argv == NULL) {
    fprintf(stderr, "ERROR: Out of memory in parse_command_line() allocating command->argv\n");
    exit(EXIT_FAILURE);
  }

  int cnt = 0;
  for (command->argv[cnt] = strtok(command_line, " ");
       command->argv[cnt];
       command->argv[cnt] = strtok(NULL, " ")) {
    printf("   %d : %s\n", cnt, command->argv[cnt]);
    cnt++;
  }

  for (cnt = 0; command->argv[cnt]; cnt++) {
    printf("  %d arg %s\n", cnt, command->argv[cnt]);
  }

  printf("%s\n", command->argv[0]);
  printf("%s\n", command->argv[0]);

  return command;
}

int main(int argc, const char *argv[]) {
  // variable initialization
  char *tempCmd;             // Holds a path to be cat with command and exec'd
  int error = -1;            // Used to find valid path to command
  bool execute = 1; // Set to false if command should no longer be executed

  FILE *input_src = get_input_source(argc, argv);

  char **command_paths = parse_shell_path();

  for (;;) {
    printf("%s", PROMPT);

    char *command_line = get_command_line_from(input_src);
    if (command_line == NULL) continue;

    echo_command_from(input_src, command_line);

    struct command_t *command = parse_command_line(command_line);

    if (!strcmp(command->argv[0], "exit")) {

      if (command->argc == 1) {
        exit(0);
      } else {
        puts("\nexit has too many arguments.");
        puts("Type \"exit\" or press Ctrl-D to exit.\n");
        execute = 0;
      } // else
    }   // if

    if (execute) {
      int i = 0;
      int pid, rc;
      rc = fork();
      if (rc == 0) {
        tempCmd = (char *)malloc((strlen(command->argv[0]) + 1) * sizeof(char));
        strcpy(tempCmd, command->argv[0]);
        while (error == -1) {
          printf("Doing execv %s\n", tempCmd);
          int cnt = 0;
          while (command->argv[cnt]) {
            printf("  %d %s\n", cnt, command->argv[cnt]);
            cnt++;
          }
          execv(tempCmd, command->argv);

          if (command_paths[i] != NULL) {
            tempCmd = (char *)realloc(tempCmd, (strlen(command_paths[i]) + 1) *
                                                   sizeof(char));
            strcpy(tempCmd, command_paths[i]);
            strcat(tempCmd, "/");
            strcat(tempCmd, command->argv[0]);
            i++;

          } else {
            puts("\nCommand Not Found.");
            puts("Please check your path and filename.");
            puts("Note: programs needing user input cannot be executed with "
                 "this shell.\n");
            exit(0);
          } // else

        } // while

      } else {
        pid = wait(&rc);
      } // else

    } // if

    free(command_line);
    free(command->argv);
    free(command);
  }
}
