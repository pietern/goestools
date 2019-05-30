#include "handler_dcs.h"

#include <regex>

#include "filename.h"
#include "string.h"

namespace {

// Expect the file to be named like this:
//
//   pH-19143220215-A.lrit
//
bool goesrParseDCSTime(const std::string& name, struct timespec& time) {
  auto pos = name.find('-');
  if (pos == std::string::npos) {
    return false;
  }

  if (pos + 1 >= name.size()) {
    return false;
  }

  const char* buf = name.c_str() + pos + 1;
  const char* format = "%y%j%H%M%S";
  struct tm tm;
  auto ptr = strptime(buf, format, &tm);

  // strptime was successful if it returned a pointer to the next char
  if (ptr == nullptr || ptr[0] != '-') {
    return false;
  }

  time.tv_sec = mktime(&tm);
  time.tv_nsec = 0;
  return true;
}

unsigned int bytes_to_uint(std::vector<char>::iterator iter, size_t len) {
  unsigned int result = 0;
  for (size_t i = 0; i < len; i++) {
    result += uint8_t(*(iter + i)) << (i*8); 
  }
  return result;
}

std::string bytes_to_time(std::vector<char>::iterator iter) {
  std::stringstream tmp;
  std::string s;
  char tmpout[20];
  struct tm tm;

  for (unsigned int i = 0; i < 7; i++)
    tmp << int(*(iter+i) & 0xF) << int(*(iter+i) >> 4);
  s = tmp.str();
  std::reverse(s.begin(),s.end());
  
  strptime(s.c_str(),"%y%j%H%M%S",&tm);
  strftime(tmpout,20,"%F %T",&tm);
  
  s = tmpout;
  return s;
}

std::string pseudo_decode(std::vector<char>::iterator iter,size_t len) {
  std::string s;
  for (unsigned int i = 0; i < len; i++)
    s += char(*(iter + i) & 0x7F);
  return s;
}

} // namespace

DCSHandler::DCSHandler(
  const Config::Handler& config,
  const std::shared_ptr<FileWriter>& fileWriter)
  : config_(config),
    fileWriter_(fileWriter) {
}

void DCSHandler::getHeader(std::vector<char> &vec) {
  std::vector<char>::iterator iter;
  iter = vec.begin();
  // 32 bytes hold the DCS file name (and trailing spaces)
  {
    constexpr unsigned n = 32;
    header.filename = std::string(iter, iter + n);
    iter += n;
  }

  // 8 bytes holding length of payload
  {
    constexpr unsigned n = 8;
    header.file_size = std::stoi(std::string(iter, iter + n));
    iter += n;
  }

  // 4 bytes holding the file's source (WCDA or NSOF)
  {
    constexpr unsigned n = 4;
    header.file_source = std::string(iter, iter + n);
    iter += n;
  }

  // 4 bytes holding the file type, should be DCSH (DCS HRIT)
  {
    constexpr unsigned n = 4;
    header.file_type = std::string(iter, iter + n);
    iter += n;
  }

  // 12 bytes holding spaces, reserved for future usage, should be all spaces
  {
    constexpr unsigned n = 12;
    header.exp_fill = std::string(iter, iter + n);
    iter += n;
  }

  // 4 bytes holding the CRC32 of the header, little-endian
  {
    constexpr unsigned n = 4;
    
    iter += n;
  }

}

void DCSHandler::handle(std::shared_ptr<const lrit::File> f) {
  auto ph = f->getHeader<lrit::PrimaryHeader>();
  if (ph.fileType != 130) {
    return;
  }

  auto text = f->getHeader<lrit::AnnotationHeader>().text;
  struct timespec time = {0, 0};

  if (!goesrParseDCSTime(text, time)) {
      // Unable to extract timestamp from file name
  }


  // Skip if the time could not be determined
  if (time.tv_sec == 0) {
    return;
  }

  // Pointer to beginning of the DCS file's data
  char *const buf = (f->read()).data();
  std::vector<char> vec = f->read();

  // Parse the DCS file's header
  DCSHandler::getHeader(vec);

  // Create the output data by sending text into a stream
  std::stringstream output_text;
  output_text << "[DCS Header]" << std::endl;
  output_text << "File Name: " << text << std::endl;
  output_text << "File Size: " << std::dec << header.file_size << " Bytes" << std::endl;
  output_text << "File Source: " << header.file_source << std::endl;
  output_text << "File Type: " << header.file_type << std::endl;
  output_text << "Header CRC32: " << std::hex << header.crc32 << std::endl << std::endl;

  std::vector<char>::iterator iter;
  DCPBlock block;

  iter = vec.begin() + 0x40;
  while(iter < vec.end() - 4) {
    block = DCPBlock();
    auto block_begin = iter - vec.begin();

    { // Block Type
      constexpr unsigned n = 1;
      block.type = bytes_to_uint(iter, n);
      iter += n;
    }
    { // Block Length
      constexpr unsigned n = 2;
      block.size = bytes_to_uint(iter, n);
      iter += n;
    }
    if (block.type == 1) {
      { // Sequence Number
        constexpr unsigned n = 3;
        block.num = bytes_to_uint(iter, n);
        iter += n;
      }
      { // Flags (Includes Baud Rate, Platform Type and Parity Error)
        constexpr unsigned n = 1;
        const int bauds[4] = {0,100,300,1200};
        block.flags = bytes_to_uint(iter, n);
        block.baud = bauds[(block.flags & 0x7)];
        block.platform = ((block.flags & 8) >> 3) + 1;
        iter += n;
      }
      { // ARM Flags (Abnormal Received Message)
        constexpr unsigned n = 1;
        block.arm = bytes_to_uint(iter, n);
        iter += n;
      }
      { // Platform Address, this can be searched at https://dcs1.noaa.gov/Account/FieldTest
        // Sometimes these addresses can be searched for on the web for more info on the data
        constexpr unsigned n = 4;
        block.address = bytes_to_uint(iter, n);
        iter += n;
      }
      { // Carrier Start Time, converted to YYYY-MM-DD HH:MM:SS
        constexpr unsigned n = 7;
        block.start = bytes_to_time(iter);
        iter += n;
      }
      { // Carrier End Time, converted to YYYY-MM-DD HH:MM:SS 
        constexpr unsigned n = 7;
        block.end = bytes_to_time(iter);
        iter += n;
      }
      { // Signal Strength in dBm EIRP
        constexpr unsigned n = 2;
        block.strength = bytes_to_uint(iter,n) & 0x03FF;
        block.strength = block.strength / 10.0f;
        iter += n;
      }
      { // Frequency offset in Hz
        constexpr unsigned n = 2;
        block.offset = bytes_to_uint(iter,n) & 0x3FFF;
        if (block.offset > 8191)  // Signed 2's complement, so let's fix that
          block.offset = block.offset - 16384;
        block.offset = block.offset / 10.0f;
        iter += n;
      }
      { // Signal Noise in deg RMS, as well as the modulation index
        constexpr unsigned n = 2;
        uint16_t num = bytes_to_uint(iter,n);
        block.noise = num & 0x0FFF;
        block.noise = block.noise / 100.0f;
        
        uint16_t i = (num & 0xC000) >> 14;
        const char indicies[5] = "XNHL";
        block.mod_index = indicies[i];
        iter += n;
      }
      { // Phase Score, represents the signal quality 0-100%
        constexpr unsigned n = 1;
        block.good_phase = bytes_to_uint(iter,n) & 0x3FFF;
        block.good_phase = block.good_phase / 2.0f;
        iter += n;
      }
      { // Channel Number and the Spacecraft ID: GOES-East, GOES-West, GOES-Central, GOES-Test
        constexpr unsigned n = 2;
        uint16_t num = bytes_to_uint(iter,n);
        block.channel = num & 0x03FF;

        const char spcrft[6] = "XEWCT";
        uint16_t i = (num & 0xF000) >> 12;
        block.spacecraft = spcrft[i];
        iter += n;
      }
      { // DRGS Source Code, see https://dcs1.noaa.gov/documents/HRIT%20DCS%20File%20Format%20Rev1.pdf
        // Table is on page 8
        constexpr unsigned n = 2;
        block.source_code = std::string(iter, iter + n);
        iter += n;
      }
      { // Source Secondary information, currently unused
        constexpr unsigned n = 2;
        block.source_sec = std::string(iter, iter + n);
        iter += n;
      }
      { // Data converted from Pseudo-Binary to an ASCII string
        const unsigned int n = block.size - 41;
        block.data = pseudo_decode(iter,n);
        iter += n;
      }
      { // Block CRC16
        constexpr unsigned n = 2;
        iter += n;
      }
    }

    if (block.type == 2) {
      { // Sequence Number
        constexpr unsigned n = 3;
        block.num = bytes_to_uint(iter, n);
        iter += n;
      }
      { // Flags (Includes Baud Rate, Platform Type and Parity Error)
        constexpr unsigned n = 1;
        const int bauds[4] = {0,100,300,1200};
        block.flags = bytes_to_uint(iter, n);
        block.baud = bauds[(block.flags & 0x7)];
        iter += n;
      }
      { // Platform Address, this can be searched at https://dcs1.noaa.gov/Account/FieldTest
        // Sometimes these addresses can be searched for on the web for more info on the data
        constexpr unsigned n = 4;
        block.address = bytes_to_uint(iter, n);
        iter += n;
      }
      { // Window Start Time, converted to YYYY-MM-DD HH:MM:SS
        constexpr unsigned n = 7;
        block.start = bytes_to_time(iter);
        iter += n;
      }
      { // Window End Time, converted to YYYY-MM-DD HH:MM:SS 
        constexpr unsigned n = 7;
        block.end = bytes_to_time(iter);
        iter += n;
      }
      { // Channel Number and the Spacecraft ID
        constexpr unsigned n = 2;
        uint16_t num = bytes_to_uint(iter,n);
        block.channel = num & 0x03FF;

        const char spcrft[6] = "XEWCT";
        uint16_t i = (num & 0xF000) >> 12;
        block.spacecraft = spcrft[i];
        iter += n;
      }
      { // Block CRC16
        constexpr unsigned n = 2;
        iter += n;
      }
    }
    
    if (block.type == 1) {
      output_text << "[DCP Block] " << std::endl;
      output_text << "Block Size: " << std::dec << block.size << " Bytes" << std::endl;
      output_text << "Sequence Number: " << std::dec << block.num << std::endl;
      output_text << "Address: 0x" << std::hex << block.address << std::endl;
      output_text << "Data Rate: " << std::dec << block.baud << " Baud" << std::endl;
      output_text << "Carrier Start: " << block.start << std::endl;
      output_text << "Message End: " << block.end << std::endl;
      output_text << "Signal Strength: " << block.strength << " dBm EIRP" << std::endl;
      output_text << "Frequency Offset: " << block.offset << " Hz" << std::endl;
      output_text << "Phase Noise: " << block.noise << "\xB0 RMS" << std::endl;
      output_text << "Modulation Index: " << block.mod_index << std::endl;
      output_text << "Good Phase: " << block.good_phase << "%" << std::endl;
      output_text << "Channel: " << std::dec << block.channel << std::endl;
      output_text << "Spacecraft: " << block.spacecraft << std::endl;
      output_text << "Source Code: " << block.source_code << std::endl;
      output_text << "Data: " << block.data << std::endl;
    }

    if (block.type == 2) {
      output_text << "[Missed Message Block]" << std::endl;
      output_text << "Block Size: " << std::dec << block.size << " Bytes" << std::endl;
      output_text << "Sequence Number: " << std::dec << block.num << std::endl;
      output_text << "Address: 0x" << std::hex << block.address << std::endl;
      output_text << "Window Start: " << block.start << std::endl;
      output_text << "Window End: " << block.end << std::endl;
      output_text << "Channel: " << std::dec << block.channel << std::endl;
      output_text << "Spacecraft: " << block.spacecraft << std::endl;
    } 
    
    output_text << std::endl;

  }



  // Convert the stream data into a vector of char's that the file_writer understands
  std::string os = output_text.str();
  std::vector<char> output_vec(os.begin(), os.end());


  FilenameBuilder fb;
  fb.dir = config_.dir;
  fb.filename = removeSuffix(f->getHeader<lrit::AnnotationHeader>().text);
  fb.time = time;
  auto path = fb.build(config_.filename, "dcs");
  fileWriter_->write(path, output_vec);
  if (config_.json) {
    fileWriter_->writeHeader(*f, path);
  }
}
