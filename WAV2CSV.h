#pragma once

#include <iostream>
#include "fftw3.h"

using namespace std;

struct arguments
{
	string input;
	string output;
	int code = 0;//-1 - end program, 0 - continue executing program without changes, 1 - in parameters is something useful
	bool quiet = false;
};

arguments prepare_input_parameters(int argc, char **argv);
vector<string> list_directory(arguments *arg);
void fill_with_data(double *in, float *data);
void complex_2_real(fftw_complex *in, double *out);
int read_file(string filename, float **samples, int iterations);
void CSV(string path, string filename, double *out);
void double2char(double in, char *out, int lenght);
string del_ex(string filename);
void suprise();