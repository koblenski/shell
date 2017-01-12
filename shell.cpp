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

  char *shell_path_dup = (char *)malloc((strlen(shell_path) + 1) * sizeof(char));
  if (shell_path_dup == NULL) {
    puts("ERROR: Out of memory in parse_shell_path() allocating shell_path_dup");
    exit(EXIT_FAILURE);
  }
  strcpy(shell_path_dup, shell_path);

  int path_count = count_char(shell_path_dup, ':') + 1;
  char **command_paths = (char **)malloc((path_count + 1) * sizeof(char *));
  if (command_paths == NULL) {
    puts("ERROR: Out of memory in parse_shell_path() allocating command_paths");
    exit(EXIT_FAILURE);
  }

  int i = 0;
  for (command_paths[i] = strtok(shell_path_dup, ":");
       command_paths[i] != NULL;
       command_paths[i] = strtok(NULL, ":")) {
    i++;
  }

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

void execute_command(struct command_t *command, char **command_paths) {
  int rc = fork();
  if (rc == 0) {
    int command_length = strlen(command->argv[0]);
    char *full_command = (char *)malloc((command_length + 1) * sizeof(char));
    strcpy(full_command, command->argv[0]);

    for (int i = 0; ; i++) {
      printf("Doing execv %s\n", full_command);
      int cnt = 0;
      while (command->argv[cnt]) {
        printf("  %d %s\n", cnt, command->argv[cnt]);
        cnt++;
      }
      execv(full_command, command->argv);

      if (command_paths[i] != NULL) {
        int command_path_length = strlen(command_paths[i]) + command_length + 2;
        full_command = (char *)realloc(full_command, command_path_length * sizeof(char));
        sprintf(full_command, "%s/%s", command_paths[i], command->argv[0]);
      } else {
        puts("\nCommand Not Found.");
        puts("Please check your path and filename.");
        puts("Note: programs needing user input cannot be executed with this shell.\n");
        exit(EXIT_SUCCESS);
      }
    }
  } else {
    int pid = wait(&rc);
  }
}

int main(int argc, const char *argv[]) {
  FILE *input_src = get_input_source(argc, argv);
  char **command_paths = parse_shell_path();

  for (;;) {
    printf("%s", PROMPT);

    char *command_line = get_command_line_from(input_src);
    if (command_line == NULL) continue;
    echo_command_from(input_src, command_line);

    struct command_t *command = parse_command_line(command_line);

    if (strcmp(command->argv[0], "exit") == 0) {
      if (command->argc == 1) {
        exit(EXIT_SUCCESS);
      } else {
        puts("\nexit has too many arguments.");
        puts("Type \"exit\" or press Ctrl-D to exit.\n");
      }
    } else {
      execute_command(command, command_paths);
    }

    free(command_line);
    free(command->argv);
    free(command);
  }
}
