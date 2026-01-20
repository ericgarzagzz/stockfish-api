#include "arena.h"
#include "constants.h"
#include "download.h"
#include <curl/curl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static Arena download_arena = {0};

void send_command(int stdin_fd, int stdout_fd, const char *command,
                  const char *exit_needle) {
  dprintf(stdin_fd, "%s\n", command);

  char buff[512];
  ssize_t n;
  while ((n = read(stdout_fd, buff, sizeof(buff) - 1)) > 0) {
    buff[n] = '\0';
    printf("Engine says: %s", buff);

    if (strstr(buff, exit_needle)) {
      break;
    }
  }
}

int main(void) {
  if (get_stockfish(&download_arena) == -1) {
    fprintf(stderr, "Failed to get stockfish engine\n");
    return -1;
  }

  int stdin_pipe[2]; // Parent writes to [1], child reads from [0]
  if (pipe(stdin_pipe) == -1) {
    fprintf(stderr, "Failed creating input pipe\n");
    return -1;
  }

  int stdout_pipe[2]; // Child writes to [1], parent reads from [0]
  if (pipe(stdout_pipe) == -1) {
    fprintf(stderr, "Failed creating output pipe\n");
    return -1;
  }

  pid_t pid = fork();

  if (pid == 0) {
    printf("Calling from child\n");
    dup2(stdin_pipe[0], STDIN_FILENO);   // Redirect child's input
    dup2(stdout_pipe[1], STDOUT_FILENO); // Redirect child's output

    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);

    char *stockfish_argv[] = {STOCKFISH_EXEC_PATH, NULL};
    if (execvp(stockfish_argv[0], stockfish_argv) < 0)
      exit(0);
  } else {
    printf("Calling from parent\n");

    // Close unused pipe ends
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    send_command(stdin_pipe[1], stdout_pipe[0], "uci", "uciok");

    // Set start position
    dprintf(stdin_pipe[1], "position startpos\n");

    send_command(stdin_pipe[1], stdout_pipe[0], "isready", "readyok");

    close(stdin_pipe[1]);
    close(stdout_pipe[0]);

    waitpid(pid, NULL, 0);
  }

  return 0;
}
