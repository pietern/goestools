#pragma once

#include "config.h"
#include "file_writer.h"
#include "handler.h"
#include "types.h"

class DCSHandler : public Handler {
public:
  explicit DCSHandler(
    const Config::Handler& config,
    const std::shared_ptr<FileWriter>& fileWriter);

  struct Header {
  	std::string filename;
  	size_t file_size;
  	std::string file_source;
  	std::string file_type;
  	std::string exp_fill;
  	int crc32;
  };

  Header header;
  void getHeader(std::vector<char>& vec);

  struct DCPBlock {
    unsigned int type;
    uint16_t size;
    uint32_t num;
    uint8_t flags;
    int baud;
    int platform;
    uint8_t arm;
    int address;
    std::string start;
    std::string end;
    float strength;
    float offset;
    float noise;
    char mod_index;
    float good_phase;
    int channel;
    char spacecraft;
    std::string source_code;
    std::string source_sec;
    std::string data;
  };


  virtual void handle(std::shared_ptr<const lrit::File> f);

protected:
  Config::Handler config_;
  std::shared_ptr<FileWriter> fileWriter_;
};
