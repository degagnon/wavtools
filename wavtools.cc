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
  filestream.read((char*)bytes, 2);
  for (int i = 0; i < 2; ++i) {
    cout << "Byte " << i << " is " << hex << (int)bytes[i] << dec << endl;
  }
  uint32_t value = static_cast<uint32_t>(bytes[0] | bytes[1] << 8);
  return value;
}

uint32_t ReadLittleEndian4ByteUint(ifstream& filestream) {
  uint8_t bytes[4];
  filestream.read((char*)bytes, 4);
  for (int i = 0; i < 4; ++i) {
    cout << "Byte " << i << " is " << hex << (int)bytes[i] << dec << endl;
  }
  uint32_t value = static_cast<uint32_t>(bytes[0] | bytes[1] << 8 |
                                         bytes[2] << 16 | bytes[3] << 24);
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
    cout << "Chunk ID: " << chunk_id << endl
         << "Chunk Size: " << chunk_size << endl
         << "Format: " << format << endl
         << "Subchunk 1 ID: " << subchunk1_id << endl
         << "Subchunk 1 Size: " << subchunk1_size << endl;
    file.close();
    cout << "File " << argv[1] << " closed." << endl;
  } else {
    cout << "File was not opened.";
  }
  return 0;
}
