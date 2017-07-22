//
// wavtools.cpp
//
//  Created on: Jul 19, 2017
//      Author: david
//
// This code requires little-endian architecture,
// which is adequate for local usage,
// but requires modification for porting to big-endian systems.


#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>

using namespace std;

struct RiffHeader {
  char chunk_id[4];
  uint32_t chunk_size;
  char format[4];
};

struct FmtHeader {
  char subchunk1_id[4];
  uint32_t subchunk1_size;
  uint16_t audio_format;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
};

// TODO(David): Add flexibility to handle extended formats with extra bytes.

struct DataHeader {
  char subchunk2_id[4];
  uint32_t subchunk2_size;
  // Data size will vary by file and therefore is handled separately.
};

int main(int argc, char** argv) {
  cout << "Number of input args is " << argc << endl;
  cout << "Arg loc is " << argv << endl;
  cout << "Arg values are:" << endl;
  for (int i = 0; i < argc; ++i) {
    cout << argv[i] << endl;
  }

  streampos filesize;
  ifstream file (argv[1], ios::in|ios::binary|ios::ate);
  if (file.is_open()) {
    cout << "File " << argv[1] << " opened." << endl;
    filesize = file.tellg();
    cout << "File size is " << filesize << " bytes" << endl;

    RiffHeader riff_info;
    FmtHeader format_info;
    DataHeader data_info;
    file.seekg(ios::beg);
    file.read(reinterpret_cast<char*>(&riff_info), sizeof(riff_info));
    file.read(reinterpret_cast<char*>(&format_info), sizeof(format_info));
    file.read(reinterpret_cast<char*>(&data_info), sizeof(data_info));
    cout << "Chunk ID: " << riff_info.chunk_id << endl
         << "Chunk Size: " << riff_info.chunk_size << endl
         << "Format: " << riff_info.format << endl
         << "Subchunk 1 ID: " << format_info.subchunk1_id << endl
         << "Subchunk 1 Size: " << format_info.subchunk1_size << endl
         << "Audio Format: " << format_info.audio_format << endl
         << "Number of Channels: " << format_info.num_channels << endl
         << "Sample Rate: " << format_info.sample_rate << endl
         << "Byte Rate: " << format_info.byte_rate << endl
         << "Block Align: " << format_info.block_align << endl
         << "Bits per Sample: " << format_info.bits_per_sample << endl
         << "Subchunk 2 ID: " << data_info.subchunk2_id << endl
         << "Subchunk 2 Size: " << data_info.subchunk2_size << endl;
    cout << "File reading is now at position " << file.tellg() << endl;
    file.close();
    cout << "File " << argv[1] << " closed." << endl;
  } else {
    cout << "File was not opened.";
  }
  return 0;
}
