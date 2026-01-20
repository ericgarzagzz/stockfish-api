#ifndef CONFIG_H
#define CONFIG_H

#define STOCKFISH_TAR_FILENAME ".cache/stockfish.tar"
#define STOCKFISH_EXEC_REGEX_PATTERN "stockfish/stockfish-ubuntu-x86-64"
#define STOCKFISH_EXEC_PATH ".cache/stockfish/stockfish-ubuntu-x86-64"
// TODO: Support Windows and MacOS as well (once cross-platform compilation is
// implemented)
#define STOCKFISH_TAR_URL                                                      \
  "https://github.com/official-stockfish/Stockfish/releases/download/sf_17.1/" \
  "stockfish-ubuntu-x86-64.tar"

#endif
