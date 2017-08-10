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
  // Data size could vary by file and therefore is handled separately.
};

class Signal {
  // Handles analysis for signal-type vectors
  // Intended as a variant of vector<> that adds a time scale and facilitates
  // signal-processing functions.
  // TODO: Make signal handle both ints and doubles, possibly via templates
 public:
  Signal(std::vector<int>, int);
  std::vector<int> GetWaveform() {return waveform_;};
  std::vector<double> GetTimeScale() {return time_scale_;};
  int GetWaveformPoint(int index) {return waveform_[index];};
  double GetTimeScalePoint(int index) {return time_scale_[index];};
  int GetSampleRate() {return sample_rate_;};
  int GetNumSamples() {return num_samples_;};

 private:
  std::vector<int> waveform_;
  std::vector<double> time_scale_;
  int sample_rate_;
  int num_samples_;
};
Signal::Signal(std::vector<int> data_input, int sample_rate_input) {
  sample_rate_ = sample_rate_input;
  waveform_ = data_input;
  num_samples_ = waveform_.size();
  for (int i = 0; i < num_samples_; ++i) {
    time_scale_.push_back(static_cast<double>(i) / sample_rate_);
  }
}

class WavFile {
  // Loads wav file data into memory and provides access to it
 public:
  WavFile(std::string);
  void PrintInfo();
  void PrintHead(int);
  int GetNumChannels() {return format_header_.num_channels;};
  int GetNumSamples() {return num_samples_;};
  int GetSampleRate() {return format_header_.sample_rate;};
  Signal ExtractSignal(int);

 private:
  // TODO(David): Update names of private variables with trailing underscore
  std::string filename_;
  std::streampos filesize_;
  RiffHeader riff_header_;
  FmtHeader format_header_;
  DataHeader data_header_;
  int num_samples_;
  std::vector<std::vector<int16_t> > data_;
  std::vector<char> remaining_chunks_;
};
WavFile::WavFile(std::string filename_input) {
  // The constructor handles file access and data loading, which might be a
  // significant amount of work, but the contents and functionality of the
  // object are tightly tied to the actual file, and using a separate ReadWav()
  // routine could create initialization issues. Possibly worth revisiting.
  filename_ = filename_input;
  std::ifstream file (filename_, std::ios::in|std::ios::binary|std::ios::ate);
  if (file.is_open()) {
    std::cout << "File " << filename_ << " opened." << std::endl;
    filesize_ = file.tellg();
    file.seekg(std::ios::beg);
    file.read(reinterpret_cast<char*>(&riff_header_), sizeof(riff_header_));
    file.read(reinterpret_cast<char*>(&format_header_), sizeof(format_header_));
    file.read(reinterpret_cast<char*>(&data_header_), sizeof(data_header_));
    // The data vector is structured such that we load the values for each
    // channel into a separate sub-vector. Element access is [channel][sample].
    // Within the class, the data 2D vector needs resizing to fit the data.
    num_samples_ = data_header_.subchunk2_size / format_header_.block_align;
    data_.resize(format_header_.num_channels);
    for (int i = 0; i < format_header_.num_channels; ++i) {
      data_[i].resize(num_samples_);
    }
    for (int j = 0; j < num_samples_; ++j) {
      for (int i = 0; i < format_header_.num_channels; ++i) {
        file.read(reinterpret_cast<char*>(&data_[i][j]), sizeof(data_[i][j]));
      }
    }
    std::vector<char> remaining_chunks(filesize_ - file.tellg(), '0');
    file.read(&remaining_chunks[0], remaining_chunks.size());
    file.close();
    std::cout << "File " << filename_ << " closed." << std::endl;
  } else {
    std::cout << "File was not opened." << std::endl;
  }
}
void WavFile::PrintInfo() {
  std::cout << "Data is organized into " << format_header_.num_channels
      << " channels, each with " << num_samples_ << " samples.\n"
      << "Sample rate = " << format_header_.sample_rate
      << " samples per second.\n"
      << "Block Align = " << format_header_.block_align
      << " bytes per sample, including all channels.\n"
      << "Data point size: " << format_header_.bits_per_sample
      << " bits per sample, single channel." << std::endl;
}
void WavFile::PrintHead(int segment_length) {
  if (segment_length > 0 && segment_length < num_samples_) {
    for (int i = 0; i < format_header_.num_channels; ++i) {
      std::cout << "Channel " << i << ": ";
      for (int j = 0; j < segment_length; ++j) {
         std::cout << data_[i][j] << " ";
      }
      std::cout << '\n';
    }
  } else {
    std::cout << segment_length << " is not a valid length." << std::endl;
  }
}
Signal WavFile::ExtractSignal(int selected_channel) {
  std::vector<int> contents;
  if (selected_channel >= 0 && selected_channel < format_header_.num_channels) {
    for (int i = 0; i < num_samples_; ++i) {
      contents.push_back(data_[selected_channel][i]);
    }
  } else {
    std::cout << "Invalid channel. Empty signal returned.";
  }
  Signal exported_signal (contents, format_header_.sample_rate);
  return exported_signal;
}

class Plotter{
  // Governs interactions with gnuplot, including plot settings
 public:
  Plotter() {};
  void AddSignal(const Signal& signal_input);
  void Plot();

 private:
  std::string file_to_write_ = "plot_data.txt";
  std::vector<Signal> signals_;
  int num_signals_ = 0;
  void WriteToFile();
};
void Plotter::AddSignal(const Signal& signal_input) {
  signals_.push_back(signal_input);
  num_signals_ += 1;
}
void Plotter::WriteToFile() {
  std::ofstream plot_prep(file_to_write_, std::ios::out);
  if (plot_prep.is_open()) {
    plot_prep << "# This data has been exported for gnuplot." << std::endl;
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
        plot_prep << std::fixed << std::setprecision(8) <<
            signals_[channel].GetTimeScalePoint(point) << '\t';
        plot_prep << signals_[channel].GetWaveformPoint(point) << delimiter;
      }
    }
    plot_prep.close();
  } else {
    std::cout << "Plot data has not been exported." << std::endl;
  }
}
void Plotter::Plot() {
  WriteToFile();
  std::string system_command = "gnuplot -persist -e \"plot ";
  for (int i = 0; i < num_signals_; ++i) {
    std::string delimiter = ", ";
    if (i < num_signals_ - 1) {
      delimiter = ", ";
    } else {
      delimiter = "\"";
    }
    system_command += "'" + file_to_write_ + "' using " +
                      std::to_string(i*2+1) + ":" + std::to_string(i*2 + 2) +
                      " title 'Channel " + std::to_string(i + 1) +
                      "' with lines" + delimiter;
  }
  std::cout << "Plotting instruction: \n" << system_command << std::endl;
  // The system() command is expedient here for calling gnuplot.
  // If this were production code, it may be appropriate to use
  // methods that are faster and more secure.
  system(system_command.c_str());
}

}  // namespace wav_names

int main(int argc, char** argv) {
  std::cout << "Number of input args is " << argc << std::endl;
  std::cout << "Arg loc is " << argv << std::endl;
  std::cout << "Arg values are:" << std::endl;
  for (int i = 0; i < argc; ++i) {
    std::cout << argv[i] << std::endl;
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
