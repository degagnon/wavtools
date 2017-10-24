# wavtools
**wavtools** is a small collection of classes to help with the interpretation of .wav files.

## Capabilities
The main executable accepts the name of a .wav file as a command line argument or as keyboard input during operation. The data is then loaded into memory and plotted via [gnuplot](http://www.gnuplot.info/). Major identifying characteristics such as format and sample rate are printed to the terminal, as well as the values of the first 10 samples of each channel. 

**wavtools** does not assume that the WAVE format chunks are structured in any particular order. Rather, it reads in all headings, identifies their locations, and parses each chunk according to its type.

Because series objects are implemented in template form, they can be created as vectors of arbitrary type. When a time scale is created from a series object, the time scale is assumed to consist of doubles.

At this time, the code is able to interpret both WAVE format type 0x0001 (16-bit) and WAVE format type 0x0003 (32-bit) encodings.

## Limitations
**wavtools** does not implement encodings other than those mentioned above.

Although each heading is identified, not all chunks are parsed. For example, the "LIST" and "id3_" chunks remain in binary only.

Plotting is achieved through a system call to gnuplot, as opposed to using direct gnuplot C++ classes or other graphical approaches. This approach was chosen as a simple way to view the waveform in a relatively short period of development time. Copying the waveform data into a text file for the system call can be a significant bottleneck in execution time.

Note that the code assumes little-endian architecture and would require additional refactoring for big-endian systems.

## Dependencies
**wavtools** does not require any libraries outside the STL in order to compile, but the plotting functionality requires [gnuplot](http://www.gnuplot.info/).

## Sample Audio Files
Audio files for testing this project were recorded in [Audacity](http://www.audacityteam.org/home/).
| File | Format |
| --- | --- |
| audiosample.wav | 16-bit sample with 2 channels |
| triaudio.wav | 16-bit sample with 3 channels |
| encoded32bit.wav | 32-bit sample with 2 channels |


## References
Thank you to the commenters and curators of these sites for helpful resources on this project:
- [A straightforward, visual explanation of 16-bit wav files](http://soundfile.sapp.org/doc/WaveFormat/)
- [A thorough description of wav format options](http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html)
- [A discussion on the cplusplus.com forum](http://www.cplusplus.com/forum/general/205408/)

## License
[MIT License](https://choosealicense.com/licenses/mit/)
