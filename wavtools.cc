//
// wavtools.cc
// Implementation file for FileLoader and FileParser classes of wavtools
//
// Last Updated: September 2017
//       Author: David Gagnon
//
// No license has been specifically selected for this code.
//
// This code requires little-endian architecture,
// which is adequate for local usage,
// but requires modification for porting to big-endian systems.

#include "wavtools.h"

namespace wav {

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
    format_ = reinterpret_cast<FmtContents&>(chunk_data_[fmt_index_][0]);
  } else {
    std::cout << "fmt chunk not found.\n";
  }
}
void FileParser::FindData() {
  id_finder_ = std::find(chunk_ids_.begin(), chunk_ids_.end(), "data");
  if (id_finder_ != chunk_ids_.end()) {
    data_index_ = distance(chunk_ids_.begin(), id_finder_);
  } else {
    std::cout << "data chunk not found.\n";
  }
}
void FileParser::ReadFact() {
  id_finder_ = std::find(chunk_ids_.begin(), chunk_ids_.end(), "fact");
  if (id_finder_ != chunk_ids_.end()) {
    int fact_index_ = distance(chunk_ids_.begin(), id_finder_);
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

}  // namespace wav_names
