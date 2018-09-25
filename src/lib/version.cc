#include "version.h"
#include "version-gen.h"

#include <iostream>

void version(int argc, char** argv) {
  std::string argv0(argv[0]);
  auto pos = argv0.rfind('/');
  if (pos != std::string::npos) {
    argv0 = argv0.substr(pos + 1);
  }

  std::cout
    << argv0
    << " -- " << GIT_COMMIT_HASH
    << " (" << GIT_COMMIT_DATE << ")"
    << std::endl
    << std::endl;
  std::cout
    << "Part of goestools (https://github.com/pietern/goestools)"
    << std::endl;
  std::cout
    << "Written by Pieter Noordhuis and contributors"
    << std::endl;
}
