#include "stdafx.h"
#include "WAV2CSV.h"

using namespace std;
using namespace std::chrono;


namespace fs = std::experimental::filesystem;

const int N = 320;//number of samples
const int iterations = 40;// length of sound = N * iterations

int main(int argc, char **argv)
{
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	arguments arg = prepare_input_parameters(argc, argv);

	if (arg.code == -1)
	{
		return -1;
	}

	vector<string> files = list_directory(&arg);
	
	if (files.empty())
	{
		if (fs::is_regular_file(arg.input))
		{
			files.push_back(arg.input);
		}
		else
		{
			cout << "Input directory empty, ending..." << endl;
			return -1;
		}
	}

	float **samples;
	samples = new float*[iterations];
	for (int i = 0; i < iterations; i++)
		samples[i] = new float[N];

	double *real;
	real = new double[N];

	int num = 1;

	double *in;
	fftw_complex *out;
	fftw_plan p;

	in = (double*)fftw_malloc(sizeof(double) * N);//allocate memory for input
	out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);//and output
	p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_MEASURE);

	for (int i = 0; i < files.size(); i++, num++)
	{
		{
			if (fs::exists(files[i]) == false)//check for sound file, if not exists, basically end program
			{
				break;
			}

			int read_result = read_file(files[i], samples, iterations);
			if (read_result == 1)
			{
				return 1;
			}
			else if (read_result == 2)
			{
				continue;
			}
		}
		for (int j = 0; j < iterations; j++)//calculate dtf
		{
			copy(samples[j], samples[j] + N, in);
			fftw_execute(p);
			complex_2_real(out, real);
			CSV(argv[2], del_ex(files[i]), real);

		}
		if(!arg.quiet) cout << "Done nr." << num << endl;
	}
	fftw_destroy_plan(p);
	fftw_free(in);
	fftw_free(out);

	for (int i = 0; i < iterations; i++)
		delete[] samples[i];
	delete[] samples;
	delete[] real;
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	cout << duration_cast<microseconds>(t2 - t1).count()/ 1000 << endl;
	return 0;
}

arguments prepare_input_parameters(int argc, char **argv)
{
	arguments arg;
	if (argc == 1)
	{
		return arg;
	}
	for (int i = 1; i < argc; i++)
	{
		string tmp = argv[i];
		tmp = tmp[0];
		if (tmp == "-")
		{
			bool anything = true;

			if (!strcmp(argv[i], "-q")) arg.quiet = true;
			else if (!strcmp(argv[i], "-S")) suprise();

			else anything = false;

			if (!anything && argc == 2)//no arguments match
			{
				if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-?") || !strcmp(argv[1], "/help") || !strcmp(argv[1], "/?"))
				{
					cout << "program [-S] <input> <output>" << endl;
				}
				else
				{
					cout << "see --help" << endl;
				}
				arg.code = -1;
			}
		}
		else
		{
			if (argc == 2)
			{
				arg.code = -1;
				cout << "No output directory. Ending..." << endl;
			}
			else
			{
				arg.input = argv[i];
				arg.output = argv[i + 1];
				arg.code = 1;
			}
			
			break;
		}
	}
	return arg;
}

vector<string> list_directory(arguments *arg)
{
	vector<string> files;
	for (auto & p : fs::directory_iterator(arg->input))
	{
		files.push_back(p.path().string());
	}

	return files;
}

void fill_with_data(double *in, float *data)
{
	for (int i = 0; i < N; i++)
	{
		in[i] = data[i];
	}
	return;
}

void complex_2_real(fftw_complex *in, double *out)//dtf output complex numbers, this function convert it to real numbers
{
	for (int i = 0; i < N / 2; i++)
	{
		out[i] = sqrt(pow(in[i][0], 2) + pow(in[i][1], 2));
		out[i] *= 2;
		out[i] /= N;
	}
	return;
}

int read_file(string filename, float **samples, int iterations)
{
	fstream file;
	file.open(filename, ios::binary | ios::in);

	int channels;
	int sampling_rate;
	int bits_per_sample;
	int size;

	if (!file.good())
	{
		cout << "Error while loading file" << endl;
		file.close();
		return 1;
	}

	char buff[16];
	file.read(buff, 4);
	buff[4] = 0;

	if (strcmp(buff, "RIFF") != 0)
	{
		cout << "File is not valid WAV PCM file" << endl;//valid WAV PCM file consists of more things, but checking if file is even pretending to be sound file is good enough for me
		file.close();
		return 2;//skip
	}
	memset(buff, 0, 16);

	int pos = 4;
	char data[4] = { 'd','a','t','a' };
	for (int i = 0; i < 4;) //look for "data", which indicate start of usefull data
	{
		file.seekg(pos + i);
		file.read(buff, 1);
		if (buff[0] == data[i])
		{
			i++;
		}
		else
		{
			i = 0;
			pos++;
		}
	}
	pos += 4;//skip "data"

	file.seekg(pos);
	file.read(buff, 4);
	//i = (t * f) / 1000

	bool eof = false; //end of file
	for (int i = 0; i < iterations; i++)
	{
		for (int j = 0; j < N; j++)
		{
			file.seekg(pos);//set position
			file.read(buff, 4);//read sample
			if ((file.rdstate() & std::ifstream::eofbit) != 0)
			{
				file.close();
				eof = true;
				break;
			}
			float f;
			char b[] = { (buff[0]), (buff[1]), (buff[2]), (buff[3]) };//litle-endian to big-endian
			memcpy(&f, &b, sizeof(f));//char array to proper float value
			samples[i][j] = f;
			pos += 4;
		}
		if (eof)
			break;
	}

	file.close();
	return 0;
}

void CSV(string path, string filename, double *out)
{
	fstream file;
	file.open(path + "/" + filename + ".csv", ios::app);

	if (!file.good())
	{
		cout << "Error while creating/opening file" << endl;
	}
	string to_Save = "";
	std::ostringstream s;
	for (int j = 0; j < (N / 2); j++)
	{
		s << out[j];
		to_Save += s.str();
		s.clear();
		s.str("");
		to_Save += ";";
	}
	file.write(to_Save.c_str(), to_Save.size());
	file << "\n";

	file.close();

	return;
}

void double2char(double in, char *out, int lenght)
{
	int tmp = 0;

	memcpy(&tmp, &in, sizeof(tmp));

	int shift = (lenght * 8) - 8;

	for (int i = 0; i < lenght; i++)
	{
		out[i] = (tmp >> shift) & 0xFF;
		shift -= 8;
	}
	return;
}

string del_ex(string filename)
{
	return filename.substr(
		filename.find_last_of("\\") + 1,
		filename.find_last_of(".") - filename.find_last_of("\\") + 1
	);
}

void  suprise()
{
	cout <<

		"                                                                                                    " << endl <<
		"                                            mMMMMMMMMMMMMMMMMMMM                                    " << endl <<
		"                                       MMMMMMMMMMMMMMMMMMMMMMMMMMMMM                                " << endl <<
		"                                    MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM                             " << endl <<
		"                                 NMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMy                           " << endl <<
		"                                MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM                          " << endl <<
		"                              MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM                        " << endl <<
		"                             MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMN                      " << endl <<
		"                            MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM                     " << endl <<
		"                          `MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM                   " << endl <<
		"                         MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM                  " << endl <<
		"                        MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM                 " << endl <<
		"                       MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNNMMMMMMMMMMMMMMMMMMMMMMMMMMMMM               " << endl <<
		"                      MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNNNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM`              " << endl <<
		"                     MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM              " << endl <<
		"                    MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM              " << endl <<
		"                   MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMmNmmmmmmmmmmNMMMMMMMMMMMMMMMMMMMMM              " << endl <<
		"                  MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNNmmmmmmmmmmmmmmmmmmMMMMMMMMMMMMMMMM              " << endl <<
		"                 MMMMMMMMMMMMMMMMMMMMMMMMMMMMMNMMNNNmmmmmmmmmmmmmmmmmmmmmmMMMMMMMMMMMMm             " << endl <<
		"                 MMMMMMMMMMMMMMMMMMMMMMMM:  :+o+++++               odmmmmmmmMMMMMMMMMMM             " << endl <<
		"                 MMMMMMMMMMMMMMMMMMMMMN  oooo+++++++                  dmmmmmmmMMMMMMMMM             " << endl <<
		"                NMMMMMMMMMMMMMMMMMMM M oooo+++++++oo                    mmdmmmmNMMMMNNMN            " << endl <<
		"                MMMMMMMMMMMMMMMMMM M ooooo+++++++sso                     `mdmmmmmMMMMNMM            " << endl <<
		"                MMMMMMMMMMMMMMMMMM ooooooo+++++oosoo                       mmmmmmNMMMMMM            " << endl <<
		"                MMMMMMMMMMMMMMM/N ooooosoo++++ooosoo                        mmmmmmmMMMNM            " << endl <<
		"                MMMMMMMMMMMMMMN +oooossooooo+ooooooo                     /   mmmmmmmMMNMo           " << endl <<
		"                MMMMMMMMMMMMMM`+ooosysoooooooo++o+o+              /      +    ymmmmmmMNMM           " << endl <<
		"               `MMMMMMMMMMNNM++sosyyysooo++++m+o+++o             /:     /      smmmmmmMNM           " << endl <<
		"               hMMMMMMMMMNNd++sosyyyoooo++d/m/++++++ -  :       ::     :/       ommmmmmMM           " << endl <<
		"               MMMMMMMMMNNo++oosysyoooo++d/ /++/+///-  .       ::s    -s         smmmmmmN           " << endl <<
		"               NMMMMMMMNN ++oosyyyso++/mdmmm////////   -   /  :o--   -:: :        mmmdmmN           " << endl <<
		"               MMMMMMMNNN+++osyyss++//ddNNm///////d/ `-   //.:..-   `.- `y        dmmhdmmN          " << endl <<
		"               MMMMMMNNN+//oosyss+//d`  /dd/////dm/m`-   /+//..``  mmm  `         `dmmmmmN          " << endl <<
		"               MMMMMMNNN//++osyso// ::/::/o////dmhmm:  //so+.` y mNNmM oo         `ddmNmNM.         " << endl <<
		"               MMMMMNNN///+oosoo//m/y+/  o/// mmmmmN  //hh+-`ohhNy.dNyhsy         ddddNNNM`         " << endl <<
		"               MMMMNNNN///+osooo/oM mhMhyo+ m-NNNMMM/+oddhs.sydMMMdMNNmm          :dddNNNN          " << endl <<
		"               MMNMNNN////+osoo+/.mMyMNMNdsMMNNMMMMNsydmdd+-dMMmmMMMMNNM          ddddNNNN          " << endl <<
		"               MMMMNNN///++oooo++omsMMMMMMMMmMMMMMNNddddddy/s NM.oMMMMMs          ddhdNNNN          " << endl <<
		"               MMMMNNN///++o+++++/dmNNNNNNNMMMMMMMNMNNmmmNdy:/ohmNMMM//- :        ddhdNNN+          " << endl <<
		"               NNMMNNN:o/++o+++++/ddmmNNNNNNNNMMMMMMNNNNNNNmddy.-----.- /         dddmmNN           " << endl <<
		"               mNMMNNm:N/+++++++//dddmNNNNNNNNMMMMMNMNNNNNNNNmmmddmNdmmy         mdddmdNN           " << endl <<
		"               :NMMNNm:N//++//+///:hddNNNNNNNNMMMMNNMNNNNNNNNdNNNNhNNNhy         mmmmhmNN           " << endl <<
		"               `NMMNNN:N//////////:hhdmNNNNNNMMMMMNNMNNNNNNNNNNNsyNNNdh          mmmmdNN            " << endl <<
		"                NMMNNN:N://///////:ohdmNNNNNMMMMMMNNNNNNNNNNNNNNNNmhdh          dmmNmNNN            " << endl <<
		"                NMMNNNNN://////:::::hmmNNNMMMMMMMN  +.mNNNNNNNNNNy/yh`d        .NmmNmNN             " << endl <<
		"                NNMNNNNN::///:::::h:mmmNMMMMMMMMMNd`ddmNNNNNNNNN`/dhyh         dNmNNmN              " << endl <<
		"                NNMMMNNNN:N///::::/h:mNNNMMMMMMMMMNNdmNNNNNNNN ++dddh         ysmmmmNd              " << endl <<
		"`                NMMMNNNN:hm:::::::shdNNMMMMMMMMMMMMmNNNNNN/ -hddddd         yyymmmmm               " << endl <<
		"```              MMMMMNNNN:N:::::::: dmNNMMMMMMMMMNN///+//+ddNNmmmd         dyymmmmm                " << endl <<
		"``````            MMMMMNNNNNN:::::::+ dmNNMMMMMMMMNNNNNNNNNNNNNNmd        dddhmmmmm                 " << endl <<
		"````````````       MMNNMNNNNNm:::::::sm mmNMMMMMMMMNmNNNNNNyNNNN:       .dmmmNNdmm                  " << endl <<
		"`````````````````  yNNNNMNNNNNo::::::shh`mmNMMMMMMMMNNNNNNNNNNN        .dddmNNddd                   " << endl <<
		"````````````````````NNNNNMNNNNNm:::+::yhhh mNMMMMMMMNNNNNNNNN N `s    dddddNNddd d                  " << endl <<
		"````````````````` MNNNNNNNMMMMNNN::/s:hhhdhh NNMMMMMNNNNNNy mmNNh    d dddmNddd ddMM-               " << endl <<
		"`````````````` `:MMMN:`NNNNMMMMNNNo:sy:hddhhhh mMMMMNNNN: NmNmm./   dd.mmNNddd ddMMMMMM             " << endl <<
		"````````````` NMMMMMMN: NNNNMMMNNN myyddmmddhhhhho  N  mmmmmmm h  `hhydmmNmdd ddMMMMMMMMM           " << endl <<
		"```````````` MMMMMMMMMN: NNNNMMMNNN mhdmNNmdydhhddhhsmmmmmmmmyhh yhhh/NNNddd ddMMMMMMMMMMMM         " << endl <<
		"``````````` MNMMMMMMMMMN:NNNNNMMMNNNmmmmNNNmd:dmddddmmmmmmmmm y+yhhh NNNhyd ddNMMMMNMMMmmMMM.```````" << endl <<
		".......... MMM MMMMMMMMMNhN NNNMMMNN mmmmNNmmd dmmmmommmmmmmmyyyhyy.NNmhyd dddMMMMMmMMMmNMMMMM``````" << endl <<
		"..........dMMMN MMMMMMMMMNNN NNNMMNNN NNNNNNNmm`dmmmmmmmmmhh hhyyy:NNmhsd dddMMMMMMMMMMMMMMMMMM`````" << endl <<
		".......... MMMMN MMMMMMMMMMNNNhNNMMNNN NNNNNNNmm-mmmmmmmhhdhhssyh NNNhsd dddMMMMMMMMMmNMMMMMMMNh````" << endl <<
		"..........+MMMMMNhMMMMMMMMMMMNN NNNMNNN NNhNNNNmmmmmddddhdhhsosy NNmmyd dddMMMMMMMMMmNMMMMMMmddd ```" << endl <<
		".......... MMMMMMNmMMMMMMMMMMMNNN NNNNNN mNMMMNNNmmmdddddhhhss NNNdddd dddNMMMMMMMMNMMMMMMM/ddddd```" << endl <<
		".......... MMMMMMNNNNMMMMMMMMMMNNNNNNNNNN-mNMMNNNNNNddddhhhhh NNNmddd dddNMMMhNMMMMMMMNNNmddddddh```" << endl <<
		".......... MMMMMMMNNNNMMMMMMMMMMNNNNmNNNNNo mNNNNNNNdddhhhd.dNNNdddd/ddNMMMNhdNMMMMMMMNNsddddddd ```" << endl <<
		".......... MMMMMMMNNNNNNMMMMMMMMMMNNNNyNNNNNM NmNNNNddhhhhdNNNdddddsdmNMhdMmmNMMMMMMMM`ddddddddd````" << endl <<
		".......... MMMMMMMMNNNNMMMMMMMMMMMMNNNNNNNsNNMN`+mNNhhh NNNdddddddoNNMMyydNNNMMMMMMM-dddddmmmdd ````" << endl <<
		"..--------`MMMMMMMMMNNNNMMNMMMMMMMMMMNNNNN  NMMMNN`mh+hNNddd . ddNMMMMMMMMNNNMMMMMmmmNdddNmmdd ````." << endl <<
		"-----------:MMMMMMMMNNNNNNMMMMMMMMMMMMNNNNNdNNMMMMMMNNNdddddm .NMMMMMMMMMNNNNMMNNmmmNdddNmmmd ......" << endl <<
		"----------- MMMMMMMMNNNNNNNNMMMMMMMMMMMNNNNN NNMMMMMNNmmdNNN NNNdmMMMMMNNNNNMMNmmmmNmmmNNmmd  ......" << endl <<
		"----------- MMMMMMMMMNNNNNNNNMMMMMMMMMMMNNNNN.NNMMMMNNNNNNNNsNmNddhdMMddNNNMMNmmmNNmmNNNmmmd ......." << endl <<
		"----------- MMMMMMMMMNNNNNMNNNNMMMMMMMMMMNNNN`NNMMMMNNNNNhNNNNNNdMhMmmdmdhdNNmmmNNmmNNNNddm ........" << endl <<
		"----------- MMMMMMMMMNNNNNNMMNNNNMMMMMMMMMNNNN NNMMMMMNmmdNddhdMNMNmmNyddhdNmmmNNNNMMNNddd ........." << endl <<
		"----------- MMMMMMMMMNNNNNNNMMMNNNNMMMMMMMNNNN`NNNMMosMhdNNNysymNmhhdhdmdhdmmmNNNNMMMNNddm`........." << endl;
}
