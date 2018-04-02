// stdafx.h: dołącz plik do standardowych systemowych plików dołączanych,
// lub specyficzne dla projektu pliki dołączane, które są często wykorzystywane, ale
// są rzadko zmieniane
//

#pragma once

#ifdef _WIN32
#include "targetver.h"
#include <tchar.h>
#include <stdio.h>
#elif __linux__
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <experimental/filesystem>
#include <chrono>
#include <string>
#include <math.h>
#include <array>
#include <vector>
#include "fftw3.h"



// TODO: W tym miejscu odwołaj się do dodatkowych nagłówków wymaganych przez program
