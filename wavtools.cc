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

#include <cstdint>  // exact integer sizes used to interpret wav file format
#include <fstream>
#include <iomanip>  // setprecision()
#include <iostream>
#include <string>
#include <vector>

namespace wav {

struct ChunkHeader {
  char id[4];
  uint32_t size;
};

inline void PrintFourChars(const char* label) {
  int kLabelSize = 4;
  std::cout.write(label, kLabelSize);
}

struct RiffContents {
  char format[4];
};

struct FmtContents {
  uint16_t audio_format;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
};

struct FactContents {
  uint32_t num_samples;
};

template <typename T>
class Signal {
  // Handles analysis for signal-type vectors
 public:
  Signal(std::vector<T>&, int);
  std::vector<T> GetWaveform() { return waveform_; };
  std::vector<double> GetTimeScale() { return time_scale_; };
  T GetWaveformPoint(int index) { return waveform_[index]; };
  double GetTimeScalePoint(int index) { return time_scale_[index]; };
  int GetSampleRate() { return sample_rate_; };
  int GetNumSamples() { return num_samples_; };

 private:
  std::vector<T> waveform_;
  std::vector<double> time_scale_;
  int sample_rate_;
  int num_samples_;
};
template <typename T>
Signal<T>::Signal(std::vector<T>& data_input, int sample_rate_input) {
  sample_rate_ = sample_rate_input;
  waveform_ = std::move(data_input);
  num_samples_ = waveform_.size();
  for (int i = 0; i < num_samples_; ++i) {
    time_scale_.push_back(static_cast<double>(i) / sample_rate_);
  }
}

class FileLoader {
  // Loads data into memory with minimal interpretation
 public:
  FileLoader(std::string);
  void PrintChunks();  

 private:
  std::string filename_;
  std::streampos filesize_;
  std::vector<std::string> chunk_ids_;
  std::vector<int32_t> chunk_sizes_;
  std::vector<std::vector<char> > chunk_data_;
  int kLabelSize_ = 4;
};
FileLoader::FileLoader(std::string filename_input) {
  filename_ = filename_input;
  std::ifstream file(filename_,
                     std::ios::in | std::ios::binary | std::ios::ate);
  if (file.is_open()) {
    std::cout << "File " << filename_ << " opened." << std::endl;
    filesize_ = file.tellg();
    file.seekg(std::ios::beg);
    ChunkHeader temp_header;
    while (file.tellg() < filesize_) {
      file.read(reinterpret_cast<char*>(&temp_header), sizeof(temp_header));
      chunk_ids_.push_back(std::string(temp_header.id, kLabelSize_));
      chunk_sizes_.push_back(temp_header.size);
      int bytes_to_read =
          ((chunk_ids_.back().compare("RIFF") == 0) ? kLabelSize_
                                                    : chunk_sizes_.back());
      std::vector<char> char_buffer(bytes_to_read, '0');
      file.read(&char_buffer[0], bytes_to_read);
      chunk_data_.push_back(char_buffer);
    }
    file.close();
    std::cout << "File " << filename_ << " closed." << std::endl;
  } else {
    std::cout << "File was not opened." << std::endl;
  }
}
void FileLoader::PrintChunks() {
  std::cout << "Chunk Names | Chunk Sizes (Bytes)\n";
  for (int i = 0; i < chunk_ids_.size(); ++i) {
    std::cout << "       " << chunk_ids_[i] << " | " << chunk_sizes_[i] << '\n';
  }
  std::cout << std::endl;
}

class WavFile {
  // Loads wav file data into memory and provides access to it
 public:
  WavFile(std::string);
  void PrintInfo();
  void PrintAllInfo();
  void PrintHead(int);
  void PrintChunks();
  int GetNumChannels() { return format_.num_channels; };
  int GetNumSamples() { return num_samples_; };
  int GetSampleRate() { return format_.sample_rate; };
  Signal<int16_t> ExtractSignal(int);

 private:
  std::string filename_;
  std::streampos filesize_;
  RiffContents riff_;
  FmtContents format_;
  FactContents fact_;
  int num_samples_;
  std::vector<std::vector<int16_t> > data_int_;
  std::vector<std::vector<float> > data_float_;
  std::vector<std::string> chunk_ids_;
  std::vector<int32_t> chunk_sizes_;
  std::vector<std::vector<char> > other_data_;
  int kLabelSize_ = 4;
};
WavFile::WavFile(std::string filename_input) {
  // The constructor handles file access and data loading, which might be a
  // significant amount of work, but the contents and functionality of the
  // object are tightly tied to the actual file, and using a separate ReadWav()
  // routine could create initialization issues. Possibly worth revisiting.
  filename_ = filename_input;
  std::ifstream file(filename_,
                     std::ios::in | std::ios::binary | std::ios::ate);
  if (file.is_open()) {
    std::cout << "File " << filename_ << " opened." << std::endl;
    filesize_ = file.tellg();
    file.seekg(std::ios::beg);
    while (file.tellg() < filesize_) {
      ChunkHeader new_header;
      file.read(reinterpret_cast<char*>(&new_header), sizeof(new_header));
      chunk_ids_.push_back(std::string(new_header.id, kLabelSize_));
      chunk_sizes_.push_back(new_header.size);
      if (chunk_ids_.back().compare("RIFF") == 0) {
        file.read(reinterpret_cast<char*>(&riff_), kLabelSize_);
      } else if (chunk_ids_.back().compare("fmt ") == 0) {
        file.read(reinterpret_cast<char*>(&format_), sizeof(format_));
        if (format_.audio_format == 1 || format_.audio_format == 3) {
          continue;
        } else {
          std::cout << "This program can only handle 16-bit PCM (fmt code 1)\n"
                    << "or 32-bit IEEE float (fmt code 3).\n"
                    << "Audio data not readable.";
          break;
        }
      } else if (chunk_ids_.back().compare("fact") == 0) {
        file.read(reinterpret_cast<char*>(&fact_), sizeof(fact_));
        num_samples_ = fact_.num_samples;
      } else if (chunk_ids_.back().compare("PEAK") == 0) {
        std::vector<char> char_buffer(chunk_sizes_.back(), '0');
        file.read(&char_buffer[0], char_buffer.size());
        other_data_.push_back(char_buffer);
      } else if (chunk_ids_.back().compare("data") == 0) {
        // Data vector element access is [channel][sample].
        // The vector needs resizing before elements can be accessed.
        if (format_.audio_format == 1) {
          num_samples_ = chunk_sizes_.back() / format_.block_align;
          data_int_.resize(format_.num_channels);
          for (int i = 0; i < format_.num_channels; ++i) {
            data_int_[i].resize(num_samples_);
          }
          for (int j = 0; j < num_samples_; ++j) {
            for (int i = 0; i < format_.num_channels; ++i) {
              file.read(reinterpret_cast<char*>(&data_int_[i][j]),
                        sizeof(data_int_[i][j]));
            }
          }
        } else if (format_.audio_format == 3) {
          data_float_.resize(format_.num_channels);
          for (int i = 0; i < format_.num_channels; ++i) {
            data_float_[i].resize(num_samples_);
          }
          for (int j = 0; j < num_samples_; ++j) {
            for (int i = 0; i < format_.num_channels; ++i) {
              file.read(reinterpret_cast<char*>(&data_float_[i][j]),
                        sizeof(data_float_[i][j]));
            }
          }
        }
      } else {
        std::vector<char> char_buffer(chunk_sizes_.back(), '0');
        file.read(&char_buffer[0], char_buffer.size());
        other_data_.push_back(char_buffer);
      }
    }
    file.close();
    std::cout << "File " << filename_ << " closed." << std::endl;
  } else {
    std::cout << "File was not opened." << std::endl;
  }
}
void WavFile::PrintInfo() {
  std::cout << "Data is organized into " << format_.num_channels
            << " channels, each with " << num_samples_ << " samples.\n"
            << "Sample rate = " << format_.sample_rate
            << " samples per second.\n"
            << "Block Align = " << format_.block_align
            << " bytes per sample, including all channels.\n"
            << "Data point size: " << format_.bits_per_sample
            << " bits per sample, single channel." << std::endl;
}
void WavFile::PrintAllInfo() {
  std::cout << "Riff Type: \n";
  PrintFourChars(riff_.format);
  std::cout << "\nFormat: " << format_.audio_format << '\n';
  std::cout << "Number of Channels: " << format_.num_channels << '\n';
  std::cout << "Sample Rate: " << format_.sample_rate << '\n';
  std::cout << "Byte Rate: " << format_.byte_rate << '\n';
  std::cout << "Block Align: " << format_.block_align << '\n';
  std::cout << "Bits per Sample: " << format_.bits_per_sample << '\n';
  std::cout << "Number of Samples: " << num_samples_ << '\n';
}
void WavFile::PrintHead(int segment_length) {
  if (segment_length > 0 && segment_length < num_samples_) {
    for (int i = 0; i < format_.num_channels; ++i) {
      std::cout << "Channel " << i << ": ";
      // Ternary operator helps avoid accessing nonexistent data
      if (format_.audio_format == 1) {
        for (int j = 0;
             j < ((segment_length < data_int_[i].size()) ? segment_length
                                                         : data_int_[i].size());
             ++j) {
          std::cout << data_int_[i][j] << " ";
        }
      } else if (format_.audio_format == 3) {
        for (int j = 0; j < ((segment_length < data_float_[i].size())
                                 ? segment_length
                                 : data_float_[i].size());
             ++j) {
          std::cout << data_float_[i][j] << " ";
        }
      }
      std::cout << '\n';
    }
  } else {
    std::cout << segment_length << " is not a valid length." << std::endl;
  }
}
void WavFile::PrintChunks() {
  std::cout << "Chunk Names | Chunk Sizes (Bytes)\n";
  for (int i = 0; i < chunk_ids_.size(); ++i) {
    std::cout << "       " << chunk_ids_[i] << " | " << chunk_sizes_[i] << '\n';
  }
  std::cout << std::endl;
}
Signal<int16_t> WavFile::ExtractSignal(int selected_channel) {
  std::vector<int16_t> contents;
  if (selected_channel >= 0 && selected_channel < format_.num_channels) {
    contents = std::move(data_int_[selected_channel]);
  } else {
    std::cout << "Invalid channel. Empty signal returned.";
  }
  Signal<int16_t> exported_signal(contents, format_.sample_rate);
  return exported_signal;
}

class Plotter {
  // Governs interactions with gnuplot, including plot settings
 public:
  Plotter(){};
  void AddSignal(const Signal<int16_t>& signal_input);
  void Plot();

 private:
  std::string file_to_write_ = "plot_data.txt";
  std::vector<Signal<int16_t> > signals_;
  int num_signals_ = 0;
  void WriteToFile();
};
void Plotter::AddSignal(const Signal<int16_t>& signal_input) {
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
        plot_prep << std::fixed << std::setprecision(8)
                  << signals_[channel].GetTimeScalePoint(point) << '\t';
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
                      std::to_string(i * 2 + 1) + ":" +
                      std::to_string(i * 2 + 2) + " title 'Channel " +
                      std::to_string(i + 1) + "' with lines" + delimiter;
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

  wav::FileLoader file_raw (argv[1]);
  file_raw.PrintChunks();

  wav::WavFile wav_file(argv[1]);
  wav_file.PrintInfo();
  wav_file.PrintAllInfo();
  wav_file.PrintChunks();
  wav_file.PrintHead(10);
  //  wav::Plotter plot;
  //  for (int i = 0; i < wav_file.GetNumChannels(); ++i) {
  //    wav::Signal<int16_t> temporary_signal = wav_file.ExtractSignal(i);
  //    plot.AddSignal(temporary_signal);
  //  }
  //  plot.Plot();
  //  // Test whether signals have been fully extracted
  //  wav_file.PrintHead(10);

  return 0;
}
