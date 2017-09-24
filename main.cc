// main file for using wavtools library

#include "wavtools.h"

int main(int argc, char** argv) {
  // TODO(David): Handle crash when input argument does not exist
  wav::FileLoader file_raw(argv[1]);
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