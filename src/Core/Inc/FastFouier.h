/*
 * FastFouier.h
 *
 *  Created on: 2021年5月20日
 *      Author: wzq
 */

#ifndef SRC_FASTFOUIER_H_
#define SRC_FASTFOUIER_H_


#include <complex>

using namespace std;

enum fourier_transform_direction { ftdFunctionToSpectrum, ftdSpectrumToFunction };

//#define INCORRECT_SPECTRUM_SIZE_FOR_FFT				1
//#define UNSUPPORTED_FTD								2

void discreteFourierFast(const complex<double>* f, int i_max, complex<double>* F, fourier_transform_direction ftd);
void discreteFourierFast2D(const complex<double>* f, int i_max, int j_max, complex<double>* F, fourier_transform_direction ftd);



#endif /* SRC_FASTFOUIER_H_ */
