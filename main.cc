// main file for using wavtools library

#include "wavtools.h"

int main(int argc, char** argv) {
  std::string file_name;
  if (argc == 1) {
    // No arguments given. Assign dummy name (for now)
    file_name = "triaudio.wav";
  } else {
    file_name = argv[1];
    if (argc > 2) {
      std::cout << "Extra arguments ignored.\n";
    }
  }
  wav::FileLoader file_raw(file_name);
  // TODO(David): Allow user to re-enter filename when not found
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