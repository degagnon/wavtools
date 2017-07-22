/*
 * wavtools.cpp
 *
 *  Created on: Jul 19, 2017
 *      Author: david
 */

#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>

using namespace std;

uint16_t ReadLittleEndian2ByteUint(ifstream& filestream) {
  uint8_t bytes[2];
  filestream.read(reinterpret_cast<char*>(bytes), 2);
  for (int i = 0; i < 2; ++i) {
    cout << "Byte " << i << " is " << hex << static_cast<int>(bytes[i]) << dec << endl;
  }
  uint32_t value = static_cast<uint32_t>(bytes[0] | bytes[1] << 8);
  return value;
}

uint32_t ReadLittleEndian4ByteUint(ifstream& filestream) {
//  uint8_t bytes[4];
//  filestream.read(reinterpret_cast<char*>(bytes), 4);
//  for (int i = 0; i < 4; ++i) {
//    cout << "Byte " << i << " is " << hex << static_cast<int>(bytes[i]) << dec << endl;
//  }
//  uint32_t value = static_cast<uint32_t>(bytes[0] | bytes[1] << 8 |
//                                         bytes[2] << 16 | bytes[3] << 24);
  uint32_t value;
  cout << "Value is size " << sizeof(value) << endl;
  filestream.read(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
  
}

int main(int argc, char** argv) {
  cout << "Number of input args is " << argc << endl;
  cout << "Arg loc is " << argv << endl;
  cout << "Arg values are:" << endl;
  for (int i = 0; i < argc; ++i) {
    cout << argv[i] << endl;
  }

//  char* data;
  streampos filesize;
  ifstream file (argv[1], ios::in|ios::binary|ios::ate);
  if (file.is_open()) {
    cout << "File " << argv[1] << " opened." << endl;
    filesize = file.tellg();
    cout << "File size is " << filesize << " bytes" << endl;

    file.seekg(ios::beg);
    char buffer[4];
    file.read(buffer, 4);
    string chunk_id (buffer, 4);
    uint32_t chunk_size = ReadLittleEndian4ByteUint(file);
    file.read(buffer, 4);
    string format (buffer, 4);
    file.read(buffer, 4);
    string subchunk1_id (buffer, 4);
    uint32_t subchunk1_size = ReadLittleEndian4ByteUint(file);
    uint16_t audio_format = ReadLittleEndian2ByteUint(file);
    uint16_t num_channels = ReadLittleEndian2ByteUint(file);
    uint32_t sample_rate = ReadLittleEndian4ByteUint(file);
    uint32_t byte_rate = ReadLittleEndian4ByteUint(file);
    uint16_t block_align = ReadLittleEndian2ByteUint(file);
    uint16_t bits_per_sample = ReadLittleEndian2ByteUint(file);
    file.read(buffer, 4);
    string subchunk2_id (buffer, 4);
    uint32_t subchunk2_size = ReadLittleEndian4ByteUint(file);
    cout << "Chunk ID: " << chunk_id << endl
         << "Chunk Size: " << chunk_size << endl
         << "Format: " << format << endl
         << "Subchunk 1 ID: " << subchunk1_id << endl
         << "Subchunk 1 Size: " << subchunk1_size << endl
         << "Audio Format: " << audio_format << endl
         << "Number of Channels: " << num_channels << endl
         << "Sample Rate: " << sample_rate << endl
         << "Byte Rate: " << byte_rate << endl
         << "Block Align: " << block_align << endl
         << "Bits per Sample: " << bits_per_sample << endl
         << "Subchunk 2 ID: " << subchunk2_id << endl
         << "Subchunk 2 Size: " << subchunk2_size << endl;
    cout << "File reading is now at position " << file.tellg() << endl;
    file.close();
    cout << "File " << argv[1] << " closed." << endl;
  } else {
    cout << "File was not opened.";
  }
  return 0;
}
