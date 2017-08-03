//
// wavtools.cpp
//
//  Created on: Jul 19, 2017
//      Author: David Gagnon
//
// No license has been specifically selected for this code.
//
// This code requires little-endian architecture,
// which is adequate for local usage,
// but requires modification for porting to big-endian systems.

#include <iostream>
#include <fstream>
#include <cstdint>  // exact integer sizes used to interpret wav file format
#include <string>
#include <vector>
#include <iomanip>  // setprecision()

using namespace std;

namespace wav {

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

class WavFile {
 public:
  WavFile(string);
  void PrintInfo();
  // TODO(David): Add function to read in waveform data into Signal objects
  // TODO(David): Add function to read in trailing data chunk(s)
  // TODO(David): Add functions to access specific attributes

 private:
  string filename;
  streampos filesize;
  RiffHeader riff_header;
  FmtHeader format_header;
  DataHeader data_header;
  int num_samples;
  vector<vector<int16_t>> data;
  vector<char> remaining_chunks;
};
WavFile::WavFile(string filename_input) {
  // The constructor handles file access and data loading, which might be a
  // significant amount of work, but the contents and functionality of the
  // object are tightly tied to the actual file, and using a separate ReadWav()
  // routine could create initialization issues. Possibly worth revisiting.
  filename = filename_input;
  ifstream file (filename, ios::in|ios::binary|ios::ate);
  if (file.is_open()) {
    std::cout << "File " << filename << " opened." << endl;
    filesize = file.tellg();
    file.seekg(ios::beg);
    file.read(reinterpret_cast<char*>(&riff_header), sizeof(riff_header));
    file.read(reinterpret_cast<char*>(&format_header), sizeof(format_header));
    file.read(reinterpret_cast<char*>(&data_header), sizeof(data_header));
    // The data vector is structured such that we load the values for each
    // channel into a separate sub-vector. Element access is [channel][sample].
    num_samples = data_header.subchunk2_size / format_header.block_align;
    vector<vector<int16_t>> data(format_header.num_channels,
                                 vector<int16_t>(num_samples));
    for (int j = 0; j < num_samples; ++j) {
      for (int i = 0; i < format_header.num_channels; ++i) {
        file.read(reinterpret_cast<char*>(&data[i][j]), sizeof(data[i][j]));
      }
    }
    vector<char> remaining_chunks(filesize - file.tellg(), '0');
    file.read(&remaining_chunks[0], remaining_chunks.size());
    file.close();
    std::cout << "File " << filename << " closed." << endl;
  } else {
    std::cout << "File was not opened." << endl;
  }
}
void WavFile::PrintInfo() {
  cout << "Data is organized into " << format_header.num_channels
      << " channels, each with " << num_samples << " samples.\n"
      << "Sample rate = " << format_header.sample_rate
      << " samples per second.\n"
      << "Block Align = " << format_header.block_align
      << " bytes per sample, including all channels.\n"
      << "Data point size: " << format_header.bits_per_sample
      << " bits per sample, single channel." << endl;
}

class Signal{
 public:
  // TODO(David): Add constructor

 private:
  // TODO(David): Create vector variables for waveform and timescale
};

}  // namespace wav_names

int main(int argc, char** argv) {
  cout << "Number of input args is " << argc << endl;
  cout << "Arg loc is " << argv << endl;
  cout << "Arg values are:" << endl;
  for (int i = 0; i < argc; ++i) {
    cout << argv[i] << endl;
  }

  wav::WavFile wav_file (argv[1]);
  wav_file.PrintInfo();

  streampos filesize;
  ifstream file (argv[1], ios::in|ios::binary|ios::ate);
  if (file.is_open()) {
    cout << "File " << argv[1] << " opened." << endl;
    filesize = file.tellg();
    cout << "File size is " << filesize << " bytes" << endl;

    wav::RiffHeader riff_info;
    wav::FmtHeader format_info;
    wav::DataHeader data_info;
    file.seekg(ios::beg);
    file.read(reinterpret_cast<char*>(&riff_info), sizeof(riff_info));
    file.read(reinterpret_cast<char*>(&format_info), sizeof(format_info));
    file.read(reinterpret_cast<char*>(&data_info), sizeof(data_info));
    // TODO(David): Improve printing for conciseness and reusability,
    // possibly via a class.
    cout << "Chunk ID: ";
    cout.write(riff_info.chunk_id, sizeof(riff_info.chunk_id)) << endl;
    cout << "Chunk Size: " << riff_info.chunk_size << endl;
    cout << "Format: ";
    cout.write(riff_info.format, sizeof(riff_info.format)) << endl;
    cout << "Subchunk 1 ID: ";
    cout.write(format_info.subchunk1_id, sizeof(format_info.subchunk1_id)) << endl;
    cout << "Subchunk 1 Size: " << format_info.subchunk1_size << endl
         << "Audio Format: " << format_info.audio_format << endl
         << "Number of Channels: " << format_info.num_channels << endl
         << "Sample Rate: " << format_info.sample_rate << endl
         << "Byte Rate: " << format_info.byte_rate << endl
         << "Block Align: " << format_info.block_align << endl
         << "Bits per Sample: " << format_info.bits_per_sample << endl;
    cout << "Subchunk 2 ID: ";
    cout.write(data_info.subchunk2_id, sizeof(data_info.subchunk2_id)) << endl;
    cout << "Subchunk 2 Size: " << data_info.subchunk2_size << endl;
    cout << "File reading is now at position " << file.tellg() << endl;

    // The data vector is structured such that we load the values for each
    // channel into a separate column.
    int num_samples = data_info.subchunk2_size / format_info.block_align;
    vector<vector<int16_t>> data(format_info.num_channels,
                                 vector<int16_t>(num_samples));
    for (int j = 0; j < num_samples; ++j) {
      for (int i = 0; i < format_info.num_channels; ++i) {
        file.read(reinterpret_cast<char*>(&data[i][j]), sizeof(data[i][j]));
      }
    }
    cout << "File reading is now at position " << file.tellg() << endl
         << "Vector has " << data.size() << " channels and "
         << data[0].size() << " elements per channel." << endl;

    vector<char> remaining_chunks(filesize - file.tellg(), '0');
    cout << "Remaining chunks bytes: " << remaining_chunks.size() << endl;
    file.read(&remaining_chunks[0], remaining_chunks.size());

    file.close();
    cout << "File " << argv[1] << " closed." << endl;

    // A time scale is needed for plotting the time series data.
    vector<double> seconds(num_samples, 0);
    for (int i = 0; i < num_samples; ++i) {
      seconds[i] = static_cast<double>(i) / format_info.sample_rate;
    }

    ofstream plot_data("plot_data.txt", ios::out);
    if (plot_data.is_open()) {
      cout << "Writing plot data to file." << endl;
      plot_data << "# This data has been exported for gnuplot." << endl;
      for (int point = 0; point < num_samples; ++point) {
        char delimiter = '\t';
        plot_data << fixed << setprecision(10) << seconds[point] << delimiter;
        for (int channel = 0; channel < format_info.num_channels; ++channel) {
          if (channel < format_info.num_channels - 1) {
            delimiter = '\t';
          } else {
            delimiter = '\n';
          }
          plot_data << data[channel][point] << delimiter;
        }
      }
      plot_data.close();
      cout << "Plot data has been exported to \"plot_data.txt\"." << endl;
    } else {
      cout << "Plot data has not been exported." << endl;
    }
    // The system() command is expedient here for calling gnuplot.
    // If this were production code, it may be appropriate to use
    // methods that are faster and more secure.
    string system_command_front = "gnuplot -persist -e \"plot ";
    string system_command_contents;
    for (int i = 0; i < format_info.num_channels; ++i) {
      string delimiter = ", ";
      if (i < format_info.num_channels - 1) {
        delimiter = ", ";
      } else {
        delimiter = "\"";
      }
      system_command_contents += "'plot_data.txt' using 1:" + to_string(i + 2) +
                                 " title 'Channel " + to_string(i + 1) +
                                 "' with lines" + delimiter;
    }
    string system_command = system_command_front + system_command_contents;
    cout << "Plotting instruction: " << endl << system_command << endl;
    system(system_command.c_str());

  } else {
    cout << "File was not opened.";
  }

  return 0;
}
