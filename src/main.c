#include "arena.h"
#include "constants.h"
#include "tar.h"
#include "utils.h"
#include <curl/curl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static Arena tar_arena = {0};

int download_stockfish_executable() {
  CURLcode result;
  CURL *curl;

  result = curl_global_init(CURL_GLOBAL_ALL);
  if (result) {
    fprintf(stderr, "Failed to initialize curl library\n");
    return (int)result;
  }

  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "Failed to initialize curl handle\n");
    curl_global_cleanup();
    return CURLE_FAILED_INIT;
  }

  FILE *stockfish_tar;

  // TODO: Support Windows and MacOS as well (once cross-platform compilation is
  // implemented)
  char *download_url = "https://github.com/official-stockfish/Stockfish/"
                       "releases/download/sf_17.1/stockfish-ubuntu-x86-64.tar";
  curl_easy_setopt(curl, CURLOPT_URL, download_url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

  if (ensure_directory_exists(".cache") != 0) {
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return -1;
  }

  stockfish_tar = fopen(STOCKFISH_TAR_FILENAME, "wb");
  if (stockfish_tar) {
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, stockfish_tar);

    result = curl_easy_perform(curl);

    fclose(stockfish_tar);
  }

  curl_easy_cleanup(curl);

  curl_global_cleanup();

  return (int)result;
}

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

int main(int argc, char **argv) {
  printf("Stockfish API called with %d arguments.\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("Argument %d: %s\n", (i + 1), argv[i]);
  }

  printf("Setting up stockfish...\n");

  if (!check_file_accessible(STOCKFISH_EXEC_PATH)) {
    if (!check_file_accessible(STOCKFISH_TAR_FILENAME)) {
      printf("Downloading stockfish...\n");

      int download_rc = download_stockfish_executable();

      if (download_rc != CURLE_OK) {
        return 1;
      }

      printf("Stockfish has been downloaded.\n");
    }

    const char *rootdir = ".cache/";
    if (extract_tar(&tar_arena, STOCKFISH_TAR_FILENAME, rootdir,
                    STOCKFISH_EXEC_REGEX_PATTERN) != 0) {
      arena_free(&tar_arena);
      fprintf(stderr, "Failed extracting stockfish tarball at %s\n",
              STOCKFISH_TAR_FILENAME);
      return -1;
    }
    arena_free(&tar_arena);
  }

  if (make_file_executable(STOCKFISH_EXEC_PATH) == -1) {
    fprintf(stderr,
            "Failed setting executable permissions to stockfish engine at %s\n",
            STOCKFISH_EXEC_PATH);
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
