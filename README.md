# WAV2CSV
Program that converts wav file (with time as x axis) to csv file (with frequency as x axis) 

How to use:

wav2csv [-q] [input] [output]

-q - quiet, don't display progres

input - folder with input files

output - folder with output files

For linux users:

Requirements:
FFTW 3 - http://www.fftw.org/

to compile, in WAV2CSV/WAV2CSV/ execute:

g++ WAV2CSV.cpp -o wav2csv -lfftw3 -std=c++17 -lstdc++fs

# Examples

wav2csv input output

wav2csv -q input output

wav2csv .\data\input .\data\output
