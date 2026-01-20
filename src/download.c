#include "download.h"
#include "constants.h"
#include "tar.h"
#include "utils.h"
#include <curl/curl.h>

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

  curl_easy_setopt(curl, CURLOPT_URL, STOCKFISH_TAR_URL);
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

int get_stockfish(Arena *arena) {
  printf("Setting up stockfish...\n");

  if (!check_file_accessible(STOCKFISH_EXEC_PATH)) {
    if (!check_file_accessible(STOCKFISH_TAR_FILENAME)) {
      printf("Downloading stockfish...\n");

      int download_rc = download_stockfish_executable();

      if (download_rc != CURLE_OK) {
        return -1;
      }

      printf("Stockfish has been downloaded.\n");
    }

    const char *rootdir = ".cache/";
    if (extract_tar(arena, STOCKFISH_TAR_FILENAME, rootdir,
                    STOCKFISH_EXEC_REGEX_PATTERN) != 0) {
      arena_free(arena);
      fprintf(stderr, "Failed extracting stockfish tarball at %s\n",
              STOCKFISH_TAR_FILENAME);
      return -1;
    }
    arena_free(arena);
  }

  if (make_file_executable(STOCKFISH_EXEC_PATH) == -1) {
    fprintf(stderr,
            "Failed setting executable permissions to stockfish engine at %s\n",
            STOCKFISH_EXEC_PATH);
    return -1;
  }

  return 0;
}
