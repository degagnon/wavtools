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

uint16_t ReadLittleEndian2Bytes(ifstream& filestream) {
  return 0;
}
uint32_t ReadLittleEndian4Bytes(ifstream& filestream) {
  char bytes[4];
  filestream.read(bytes, 4);
//  uint32_t value = static_cast<uint32_t>(bytes[0] | bytes[1] << 8 |
//                                         bytes[2] << 16 | bytes[3] << 24);
  return 0;
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

//    data = new char [filesize];
    file.seekg(ios::beg);
    char buffer[4];
    file.read(buffer, 4);
    string chunk_id (buffer, 4);
    uint16_t chunk_size = ReadLittleEndian4Bytes(file);
    file.read(buffer, 4);
    string format (buffer, 4);
//    file.read(data, filesize);
    cout << chunk_id << endl << chunk_size << endl << format << endl;
    file.close();
    cout << "File " << argv[1] << " closed." << endl;
//    for (int i = 0; i < 50; ++i) {
//      cout << data[i];
//    }
    cout << endl;
//    delete[] data;
  } else {
    cout << "File was not opened.";
  }

  return 0;
}


