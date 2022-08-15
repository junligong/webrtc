#include "public.h"
#include <string>

FILE* bitrate_file = nullptr;
int start_bitrate_ms = 0;
void write_bitrate(int bitrate, int time_ms) {
  if (!bitrate_file){
    bitrate_file = fopen("bitrate_file.txt", "w+");
    start_bitrate_ms = time_ms;
  }

  std::string str_msg = std::to_string(bitrate) + "," +
                        std::to_string(time_ms - start_bitrate_ms) + "\n";
  fwrite(str_msg.c_str(), str_msg.length(), 1, bitrate_file);
  fflush(bitrate_file);
}
