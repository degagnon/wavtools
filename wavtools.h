//
// wavtools.h
// Header file for wavtools
// Templated classes remain within this header file.
//
// Last Updated: September 2017
//       Author: David Gagnon
//
// This code requires little-endian architecture,
// which is adequate for local usage,
// but requires modification for porting to big-endian systems.

#ifndef WAVTOOLS_H
#define WAVTOOLS_H

#include <algorithm>  // find()
#include <cstdint>    // exact integer sizes used to interpret wav file format
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
class Series {
 public:
  Series(std::vector<T>& data_input) { values_ = std::move(data_input); };
  int GetNumSamples() { return values_.size(); };
  std::vector<T> GetValues() { return values_; };
  T GetOnePoint(int index) { return values_[index]; };
  Series<double> CreateTimeScale(int);
  void PrintHead(size_t segment_length);

 private:
  std::vector<T> values_;
};
template <typename T>
Series<double> Series<T>::CreateTimeScale(int sample_rate) {
  std::vector<double> time_scale(values_.size());
  for (size_t i = 0; i < values_.size(); ++i) {
    time_scale[i] = static_cast<double>(i) / sample_rate;
  }
  return Series<double>(time_scale);
}
template <typename T>
void Series<T>::PrintHead(size_t segment_length) {
  if (segment_length > 0 && segment_length < values_.size()) {
    for (size_t i = 0; i < segment_length; ++i) {
      std::cout << values_[i] << " ";
    }
    std::cout << '\n';
  } else {
    std::cout << "Segment length " << segment_length << " is not valid."
              << std::endl;
  }
}

class FileLoader {
  // Loads data into memory with minimal interpretation
 public:
  FileLoader(std::string);
  void LoadFile(std::string);
  void PrintChunks();
  const std::vector<std::string> GetIDs() { return chunk_ids_; };
  const std::vector<int32_t> GetSizes() { return chunk_sizes_; };
  const std::vector<std::vector<char>> GetData() { return chunk_data_; };
  bool load_success;

 private:
  std::string filename_;
  std::streampos filesize_;
  std::vector<std::string> chunk_ids_;
  std::vector<int32_t> chunk_sizes_;
  std::vector<std::vector<char>> chunk_data_;
  int kLabelSize_ = 4;
};

class FileParser {
 public:
  FileParser(FileLoader&);
  void PrintAllInfo();
  std::vector<Series<double>> ExtractChannels();
  int GetSampleRate() { return format_.sample_rate; };

 private:
  std::vector<std::string> chunk_ids_;
  std::vector<int32_t> chunk_sizes_;
  std::vector<std::vector<char>> chunk_data_;
  std::vector<std::string>::iterator id_finder_;
  RiffContents riff_;
  FmtContents format_;
  FactContents fact_;
  int data_index_;
  void ReadRiff();
  void ReadFmt();
  void FindData();
  void ReadFact();
  template <typename T>
  std::vector<std::vector<T>> ReadData();
  template <typename T>
  std::vector<std::vector<double>> DataToDouble(std::vector<std::vector<T>>);
};

template <typename T>
class Plotter {
  // Governs interactions with gnuplot, including plot settings
 public:
  Plotter(){};
  void AddSeriesPair(const Series<T>& time_series, const Series<T>& waveform);
  void Plot();

 private:
  std::string file_to_write_ = "plot_data.txt";
  std::vector<Series<T>> series_list_;
  int num_series_ = 0;
  void WriteToFile();
};
template <typename T>
void Plotter<T>::AddSeriesPair(const Series<T>& time_series,
                               const Series<T>& waveform) {
  series_list_.push_back(time_series);
  series_list_.push_back(waveform);
  num_series_ += 2;
}
template <typename T>
void Plotter<T>::WriteToFile() {
  std::ofstream plot_prep(file_to_write_, std::ios::out);
  if (plot_prep.is_open()) {
    plot_prep << "# This data has been exported for gnuplot." << std::endl;
    int max_length = 0;
    for (int i = 0; i < num_series_; ++i) {
      if (max_length < series_list_[i].GetNumSamples()) {
        max_length = series_list_[i].GetNumSamples();
      }
    }
    for (int point = 0; point < max_length; ++point) {
      char delimiter = '\t';
      for (int column = 0; column < num_series_; ++column) {
        if (column < num_series_ - 1) {
          delimiter = '\t';
        } else {
          delimiter = '\n';
        }
        plot_prep << std::fixed << std::setprecision(8)
                  << series_list_[column].GetOnePoint(point) << delimiter;
      }
    }
    plot_prep.close();
  } else {
    std::cout << "Plot data has not been exported." << std::endl;
  }
}
template <typename T>
void Plotter<T>::Plot() {
  WriteToFile();
  std::string system_command = "gnuplot -persist -e \"plot ";
  for (int i = 0; i < num_series_ / 2; ++i) {
    std::string delimiter = ", ";
    if (i < num_series_ / 2 - 1) {
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

#endif