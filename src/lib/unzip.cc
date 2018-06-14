#include <iostream>
#include <fstream>
#include <memory>

#include "zip.h"

int unzip(std::string path) {
  Zip zip(std::unique_ptr<std::istream>(new std::ifstream(path)));
  std::cout << zip.fileName() << std::endl;
  std::ofstream of(zip.fileName());
  auto data = zip.read();
  of.write(data.data(), data.size());
  return 0;
}

int main(int argc, char** argv) {
  int rv = 0;
  for (int i = 1; i < argc; i++) {
    rv = unzip(argv[i]);
    if (rv != 0) {
      return rv;
    }
  }
  return rv;
}
