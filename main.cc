// main file for using wavtools library

#include "wavtools.h"

int main(int argc, char** argv) {
  std::string file_name;
  if (argc > 1) {
    file_name = argv[1];
    if (argc > 2) {
      std::cout << "Extra arguments ignored.\n";
    }
  }
  std::cout << "Input file name is: " << file_name << '\n';
  wav::FileLoader file_raw(file_name);
  while (!file_raw.load_success) {
    std::cout << "File " << file_name << " was not loadable.\n";
    std::cout << "Choose file name: ";
    // getline's functionality on the terminal is satisfactory.
    // It appears that the debugger in VS Code does not stop to allow
    // user input via getline.
    std::getline(std::cin, file_name);
    file_raw.LoadFile(file_name);
  }
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