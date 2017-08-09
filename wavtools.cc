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
// TODO(David): Handle extended formats with extra bytes (low priority).

struct DataHeader {
  char subchunk2_id[4];
  uint32_t subchunk2_size;
  // Data size will vary by file and therefore is handled separately.
};

class Signal {
  // Handles analysis for signal-type vectors
  // Intended as a variant of vector<> that adds a time scale and facilitates
  // signal-processing functions.
  // TODO: Make signal handle both ints and doubles, possibly via templates
 public:
  Signal(vector<int>, int);
  vector<int> GetWaveform() {return waveform;};
  vector<double> GetTimeScale() {return time_scale;};
  int GetSampleRate() {return sample_rate;};
  int GetNumSamples() {return num_samples;};
  vector<int> waveform;
  vector<double> time_scale;
  vector<int> spectrum;
  vector<double> frequency_scale;

 private:
  int sample_rate;
  int num_samples;
};
Signal::Signal(vector<int> data_input, int sample_rate_input) {
  sample_rate = sample_rate_input;
  waveform = data_input;
  num_samples = waveform.size();
  for (int i = 0; i < num_samples; ++i) {
    time_scale.push_back(static_cast<double>(i) / sample_rate);
  }
};

class WavFile {
  // Loads wav file data into memory and provides access to it
 public:
  WavFile(string);
  void PrintInfo();
  void PrintHead(int);
  int GetNumChannels() {return format_header.num_channels;};
  int GetNumSamples() {return num_samples;};
  int GetSampleRate() {return format_header.sample_rate;};
  Signal ExtractSignal(int);

 private:
  // TODO(David): Update names of private variables with trailing underscore
  string filename;
  streampos filesize;
  RiffHeader riff_header;
  FmtHeader format_header;
  DataHeader data_header;
  int num_samples;
  vector<vector<int16_t> > data;
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
    // Within the class, the data 2D vector needs resizing to fit the data.
    num_samples = data_header.subchunk2_size / format_header.block_align;
    data.resize(format_header.num_channels);
    for (int i = 0; i < format_header.num_channels; ++i) {
      data[i].resize(num_samples);
    }
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
};
void WavFile::PrintInfo() {
  cout << "Data is organized into " << format_header.num_channels
      << " channels, each with " << num_samples << " samples.\n"
      << "Sample rate = " << format_header.sample_rate
      << " samples per second.\n"
      << "Block Align = " << format_header.block_align
      << " bytes per sample, including all channels.\n"
      << "Data point size: " << format_header.bits_per_sample
      << " bits per sample, single channel." << endl;
};
void WavFile::PrintHead(int segment_length) {
  if (segment_length > 0 && segment_length < num_samples) {
    for (int i = 0; i < format_header.num_channels; ++i) {
      cout << "Channel " << i << ": ";
      for (int j = 0; j < segment_length; ++j) {
         cout << data[i][j] << " ";
      }
      cout << '\n';
    }
  } else {
    std::cout << segment_length << " is not a valid length." << endl;
  }
}
Signal WavFile::ExtractSignal(int selected_channel) {
  vector<int> contents;
  if (selected_channel >= 0 && selected_channel < format_header.num_channels) {
    for (int i = 0; i < num_samples; ++i) {
      contents.push_back(data[selected_channel][i]);
    }
  } else {
    std::cout << "Invalid channel. Empty signal returned.";
  }
  Signal exported_signal (contents, format_header.sample_rate);
  return exported_signal;
};

class Plotter{
  // Governs interactions with gnuplot, including plot settings
 public:
  Plotter() {};
  void AddSignal(Signal signal_input);
  void Plot();

 private:
  string file_to_write_ = "plot_data.txt";
  vector<Signal> signals_;
  int num_signals_ = 0;
  void WriteToFile();
};
void Plotter::AddSignal(Signal signal_input) {
  signals_.push_back(signal_input);
  num_signals_ += 1;
};
void Plotter::WriteToFile() {
  ofstream plot_prep(file_to_write_, ios::out);
  if (plot_prep.is_open()) {
    plot_prep << "# This data has been exported for gnuplot." << endl;
    int max_signal_length = 0;
    for (int i = 0; i < num_signals_; ++i) {
      if (max_signal_length < signals_[i].GetNumSamples()) {
        max_signal_length = signals_[i].GetNumSamples();
      }
    }
    for (int point = 0; point < max_signal_length; ++point) {
      char delimiter = '\t';
      for (int channel = 0; channel < num_signals_; ++channel) {
        if (channel < num_signals_ - 1) {
          delimiter = '\t';
        } else {
          delimiter = '\n';
        }
        plot_prep << fixed << setprecision(8) <<
            signals_[channel].time_scale[point] << '\t';
        plot_prep << signals_[channel].waveform[point] << delimiter;
      }
    }
    plot_prep.close();
  } else {
    cout << "Plot data has not been exported." << endl;
  }
};
void Plotter::Plot() {
  WriteToFile();
  string system_command = "gnuplot -persist -e \"plot ";
  for (int i = 0; i < num_signals_; ++i) {
    string delimiter = ", ";
    if (i < num_signals_ - 1) {
      delimiter = ", ";
    } else {
      delimiter = "\"";
    }
    system_command += "'" + file_to_write_ + "' using " +
                      to_string(i*2+1) + ":" + to_string(i*2 + 2) +
                      " title 'Channel " + to_string(i + 1) +
                      "' with lines" + delimiter;
  }
  cout << "Plotting instruction: " << endl << system_command << endl;
  // The system() command is expedient here for calling gnuplot.
  // If this were production code, it may be appropriate to use
  // methods that are faster and more secure.
  system(system_command.c_str());
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
  wav_file.PrintHead(10);
  wav::Plotter plot;
  for (int i = 0; i < wav_file.GetNumChannels(); ++i) {
    wav::Signal temporary_signal = wav_file.ExtractSignal(i);
    plot.AddSignal(temporary_signal);
  }
  plot.Plot();

  return 0;
}
