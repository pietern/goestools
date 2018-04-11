#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <map>
#include <vector>

#include "lib/dir.h"

#include "file.h"

struct Area {
  int minColumn = 0;
  int maxColumn = 0;
  int minLine = 0;
  int maxLine = 0;

  int width() const {
    return maxColumn - minColumn;
  }

  int height() const {
    return maxLine - minLine;
  }

  Area getIntersection(const Area& other) const {
    Area result;
    result.minColumn = std::max(minColumn, other.minColumn);
    result.maxColumn = std::min(maxColumn, other.maxColumn);
    result.minLine = std::max(minLine, other.minLine);
    result.maxLine = std::min(maxLine, other.maxLine);
    return result;
  }

  Area getUnion(const Area& other) const {
    Area result;
    result.minColumn = std::min(minColumn, other.minColumn);
    result.maxColumn = std::max(maxColumn, other.maxColumn);
    result.minLine = std::min(minLine, other.minLine);
    result.maxLine = std::max(maxLine, other.maxLine);
    return result;
  }
};

// Print area covered by specified images.
int main(int argc, char** argv) {
  std::map<int, std::vector<lrit::File>> all;

  // Group files by their image identifier
  for (int i = 1; i < argc; i++) {
    struct stat st;
    auto rv = stat(argv[i], &st);
    if (rv < 0) {
      perror("stat");
      exit(1);
    }

    std::vector<std::string> result;
    if (S_ISDIR(st.st_mode)) {
      Dir dir(argv[i]);
      result = dir.matchFiles("*.lrit*");
    } else {
      result.push_back(argv[i]);
    }

    for (const auto& path : result) {
      auto file = lrit::File(path);
      if (!file.hasHeader<lrit::ImageNavigationHeader>()) {
        continue;
      }

      auto sih = file.getHeader<lrit::SegmentIdentificationHeader>();
      auto& files = all[sih.imageIdentifier];
      files.push_back(std::move(file));
    }
  }

  // Compute area for all images
  for (auto& it : all) {
    auto& files = it.second;

    // Sort by segment identifier
    std::sort(
      files.begin(),
      files.end(),
      [](const auto& a, const auto& b) -> bool {
        auto sa = a.template getHeader<lrit::SegmentIdentificationHeader>();
        auto sb = b.template getHeader<lrit::SegmentIdentificationHeader>();
        return sa.segmentNumber < sb.segmentNumber;
      });

    // Ignore incomplete images
    auto si = files[0].getHeader<lrit::SegmentIdentificationHeader>();
    if (files.size() != si.maxSegment) {
      continue;
    }

    // Compute area covered by this segmented image.
    // Largely copied from the original version of goesproc,
    // which was removed in eaa56cb2.
    Area area;
    for (auto& file : files) {
      auto is = file.getHeader<lrit::ImageStructureHeader>();
      auto in = file.getHeader<lrit::ImageNavigationHeader>();
      auto nl = file.getHeader<lrit::NOAALRITHeader>();

      // Update relative area represented by this image
      Area tmp;
      tmp.minColumn = -in.columnOffset;
      tmp.maxColumn = -in.columnOffset + is.columns;
      tmp.minLine = -in.lineOffset;
      tmp.maxLine = -in.lineOffset + is.lines;

      // For Himawari-8, the line offset is an int32_t and can be negative
      if (nl.productID == 43) {
        tmp.minLine = -(int32_t)in.lineOffset;
        tmp.maxLine = -(int32_t)in.lineOffset + is.lines;
      }

      if (area.height() == 0) {
        area = tmp;
      } else {
        area = area.getUnion(tmp);
      }
    }

    std::cout << "crop = [ ";
    std::cout << area.minColumn << ", ";
    std::cout << area.maxColumn << ", ";
    std::cout << area.minLine << ", ";
    std::cout << area.maxLine << " ]";
    std::cout << std::endl;
  }
}
