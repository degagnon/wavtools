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
  void PrintChunks();
  const std::vector<std::string> GetIDs() { return chunk_ids_; };
  const std::vector<int32_t> GetSizes() { return chunk_sizes_; };
  const std::vector<std::vector<char>> GetData() { return chunk_data_; };

 private:
  std::string filename_;
  std::streampos filesize_;
  std::vector<std::string> chunk_ids_;
  std::vector<int32_t> chunk_sizes_;
  std::vector<std::vector<char>> chunk_data_;
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
  for (size_t i = 0; i < chunk_ids_.size(); ++i) {
    std::cout << "       " << chunk_ids_[i] << " | " << chunk_sizes_[i] << '\n';
  }
  std::cout << std::endl;
}

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
FileParser::FileParser(FileLoader& source) {
  chunk_ids_ = source.GetIDs();
  chunk_sizes_ = source.GetSizes();
  chunk_data_ = source.GetData();
  ReadRiff();
  ReadFmt();
  FindData();
  ReadFact();
}
void FileParser::ReadRiff() {
  id_finder_ = std::find(chunk_ids_.begin(), chunk_ids_.end(), "RIFF");
  if (id_finder_ != chunk_ids_.end()) {
    int riff_index_ = distance(chunk_ids_.begin(), id_finder_);
    std::cout << "RIFF found at " << riff_index_ << std::endl;
    for (char& item : chunk_data_[riff_index_]) {
      std::cout << item;
    }
    std::cout << std::endl;
    riff_ = reinterpret_cast<RiffContents&>(chunk_data_[riff_index_][0]);
  } else {
    std::cout << "RIFF chunk not found.\n";
  }
}
void FileParser::ReadFmt() {
  id_finder_ = std::find(chunk_ids_.begin(), chunk_ids_.end(), "fmt ");
  if (id_finder_ != chunk_ids_.end()) {
    int fmt_index_ = distance(chunk_ids_.begin(), id_finder_);
    std::cout << "fmt found at " << fmt_index_ << std::endl;
    format_ = reinterpret_cast<FmtContents&>(chunk_data_[fmt_index_][0]);
  } else {
    std::cout << "fmt chunk not found.\n";
  }
}
void FileParser::FindData() {
  id_finder_ = std::find(chunk_ids_.begin(), chunk_ids_.end(), "data");
  if (id_finder_ != chunk_ids_.end()) {
    data_index_ = distance(chunk_ids_.begin(), id_finder_);
    std::cout << "data found at " << data_index_ << std::endl;
  } else {
    std::cout << "data chunk not found.\n";
  }
}
void FileParser::ReadFact() {
  id_finder_ = std::find(chunk_ids_.begin(), chunk_ids_.end(), "fact");
  if (id_finder_ != chunk_ids_.end()) {
    int fact_index_ = distance(chunk_ids_.begin(), id_finder_);
    std::cout << "fact found at " << fact_index_ << std::endl;
    fact_ = reinterpret_cast<FactContents&>(chunk_data_[fact_index_][0]);
  } else {
    std::cout << "fact chunk not found.\n"
              << "Using alternate calculation for number of samples.\n";
    fact_.num_samples = chunk_sizes_[data_index_] / format_.block_align;
  }
}
void FileParser::PrintAllInfo() {
  std::cout << "Riff Type: \n";
  PrintFourChars(riff_.format);
  std::cout << "\nFormat: " << format_.audio_format << '\n';
  std::cout << "Number of Channels: " << format_.num_channels << '\n';
  std::cout << "Sample Rate: " << format_.sample_rate << '\n';
  std::cout << "Byte Rate: " << format_.byte_rate << '\n';
  std::cout << "Block Align: " << format_.block_align << '\n';
  std::cout << "Bits per Sample: " << format_.bits_per_sample << '\n';
  std::cout << "Number of Samples: " << fact_.num_samples << '\n';
}
template <typename T>
std::vector<std::vector<T>> FileParser::ReadData() {
  int kBitsPerByte = 8;
  int bytes_per_sample = format_.bits_per_sample / kBitsPerByte;
  int byte_counter = 0;
  std::vector<T> single_channel(fact_.num_samples);
  std::vector<std::vector<T>> data_parse(format_.num_channels, single_channel);
  // Wav data format is interleaved, so we cycle rapidly across channels here.
  for (size_t j = 0; j < fact_.num_samples; ++j) {
    for (size_t i = 0; i < format_.num_channels; ++i) {
      data_parse[i][j] =
          reinterpret_cast<T&>(chunk_data_[data_index_][byte_counter]);
      byte_counter += bytes_per_sample;
    }
  }
  return data_parse;
}
template <typename T>
std::vector<std::vector<double>> FileParser::DataToDouble(
    std::vector<std::vector<T>> pre_process) {
  std::vector<std::vector<double>> post_process(format_.num_channels);
  for (int i = 0; i < format_.num_channels; ++i) {
    post_process[i].reserve(fact_.num_samples);
  }
  for (int i = 0; i < format_.num_channels; ++i) {
    for (size_t j = 0; j < fact_.num_samples; ++j) {
      post_process[i].push_back(static_cast<double>(pre_process[i][j]));
    }
  }
  return post_process;
}
std::vector<Series<double>> FileParser::ExtractChannels() {
  std::vector<std::vector<double>> output_vectors;
  if (format_.audio_format == 1) {
    output_vectors = DataToDouble(ReadData<int16_t>());
  } else if (format_.audio_format == 3) {
    output_vectors = DataToDouble(ReadData<float>());
  } else {
    std::cout << "Audio data type not recognized.\n";
  }
  std::vector<Series<double>> output_series;
  for (int i = 0; i < format_.num_channels; ++i) {
    Series<double> temp_series(output_vectors[i]);
    output_series.push_back(temp_series);
  }
  return output_series;
}

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

int main(int argc, char** argv) {
  // TODO(David): Clean out comments and extraneous material from main()
  std::cout << "Number of input args is " << argc << std::endl;
  std::cout << "Arg loc is " << argv << std::endl;
  std::cout << "Arg values are:" << std::endl;
  for (int i = 0; i < argc; ++i) {
    std::cout << argv[i] << std::endl;
  }

  wav::FileLoader file_raw(argv[1]);
  file_raw.PrintChunks();
  wav::FileParser file_parse(file_raw);
  file_parse.PrintAllInfo();
  std::vector<wav::Series<double>> waveforms = file_parse.ExtractChannels();
  wav::Series<double> time_axis(
      waveforms[0].CreateTimeScale(file_parse.GetSampleRate()));
  wav::Plotter<double> plot;
  for (size_t i = 0; i < waveforms.size(); ++i) {
    std::cout << "Channel " << i << ": ";
    waveforms[i].PrintHead(10);
    plot.AddSeriesPair(time_axis, waveforms[i]);
  }
  plot.Plot();

  return 0;
}
