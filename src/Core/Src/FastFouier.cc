/*
 * FastFouier.cc
 *
 *  Created on: 2021年5月20日
 *      Author: wzq
 */

#include "FastFouier.h"

const complex<float> I(0, 1);
const complex<uint16_t> ZERO(0, 0);
const float PI = 3.14159265358979323846;

int reverseBits(unsigned short digitsCount, int value)
{
	if (value >> digitsCount > 0) return -1;

	int res = 0;
	for (int d = 0; d < digitsCount; d++)
	{
		res = (res * 2 + (value % 2));
		value /= 2;
	}

	return res;
}

//input sample:f;  number of sample i_max;  output:F; FFT or IFFT:ftd (IFFT may not be used in this project)
//需要补零，目前暂定补零16倍(15/16的内容为0）

#ifdef ZEROPAD_VIR

void discreteFourierFast(const complex<uint16_t>* f, int i_max, complex<uint16_t>* F, fourier_transform_direction ftd)
{
	//if (i_max <= 0 || ((i_max & (i_max - 1)) != 0)) throw INCORRECT_SPECTRUM_SIZE_FOR_FFT;


	float norm, exp_dir;

	switch (ftd)
	{
	case ftdFunctionToSpectrum:
		norm = 1;
		exp_dir = -1;
		break;
//	case ftdSpectrumToFunction:
//		norm = 1.0 / i_max;
//		exp_dir = 1;
//		break;
	//default:
		//throw UNSUPPORTED_FTD;
	}

	int zeroAddi=16*i_max;

	int NN = zeroAddi, digitsCount = 0;
	while (NN >>= 1) digitsCount++;



	// Allocating 2 buffers with n complex values
	complex<float> buf[2][i_max];
//	complex<float>** buf = new complex<float>* [2];
//	for (int i = 0; i < 2; i++)
//	{
//		buf[i] = new complex<float>[i_max];
//	}



	// Grouping function values according to the binary-reversed index order
	int cur_buf = 0;
	//由于内存空间不够,可能出现两种情况
	//reversBit大于i_max,则在对应位置补0即可
	//i已经大于i_max,无论reversbit是多少都无法存储
	//
	for (int i = 0; i < i_max; i++)
	{
		int reverseBit=reverseBits(digitsCount, i);
		//大于等于i_max的部分全部为0
		if(reverseBit>=i_max)
		{
			buf[cur_buf][i] = ZERO;
		}
		else
		{
			buf[cur_buf][i] = f[reverseBit];
		}
	}



	uint16_t exp_divider = 1;
	uint16_t different_exps = 2;
	//注意改变了values_in_row，原本是i_max/2
	uint16_t values_in_row = zeroAddi / 2;

	int next_buf = 1;

	for (uint16_t step = 0; step < digitsCount; step++)
	{
		for (uint16_t n = 0; n < different_exps; n++)
		{
			complex<float> xp = exp((exp_dir * PI * n / exp_divider) * I);

			for (uint16_t k = 0; k < values_in_row; k++)
			{
				int index_1=2 * k + (n % (different_exps / 2)) * (values_in_row * 2);
				//超出buff的不计算
				if(index_1>=i_max)
				{
					continue;
				}
				complex<float>* pf = &buf[cur_buf][2 * k + (n % (different_exps / 2)) * (values_in_row * 2)];
				int index_2=n * values_in_row + k;
				//超出buff的不计算
				if(index_2>=i_max)
				{
					break;
				}
				buf[next_buf][index_2] = (*pf) + (*(pf + 1)) * xp;
			}
		}

		exp_divider *= 2;
		different_exps *= 2;
		values_in_row /= 2;
		cur_buf = next_buf;
		next_buf = (cur_buf + 1) % 2;
	}

	// Norming, saving the result
	for (int i = 0; i < i_max; i++)
	{
		F[i] = norm * buf[cur_buf][i];
	}

	// Freeing our temporary buffers
//	for (int i = 0; i < 2; i++)
//	{
//		delete [] buf[i];
//	}
//	delete [] buf;

}

#else


//一度出现过16倍大小可以允许的情况，但是当时有位置写错了，改动一下之后就无法运行了
//目前依然远远不够满足要求，如果不通过不占内容的假0来表示的话

void discreteFourierFast(const complex<uint16_t>* f, int i_max, complex<uint16_t>* F, fourier_transform_direction ftd)
{
	//if (i_max <= 0 || ((i_max & (i_max - 1)) != 0)) throw INCORRECT_SPECTRUM_SIZE_FOR_FFT;


	float norm=1, exp_dir=-1;

//	switch (ftd)
//	{
//	case ftdFunctionToSpectrum:
//		norm = 1;
//		exp_dir = -1;
//		break;
////	case ftdSpectrumToFunction:
////		norm = 1.0 / i_max;
////		exp_dir = 1;
////		break;
//	//default:
//		//throw UNSUPPORTED_FTD;
//	}

	uint16_t zeroAdd=128;

	uint16_t NN = zeroAdd, digitsCount = 0;
	while (NN >>= 1) digitsCount++;



	// Allocating 2 buffers with n complex values
	complex<uint16_t> buf[2][128];



//	complex<float>** buf = new complex<float>* [2];
//	for (int i = 0; i < 2; i++)
//	{
//		buf[i] = new complex<float>[i_max];
//	}



	// Grouping function values according to the binary-reversed index order
	uint8_t cur_buf = 0;

	for (int i = 0; i < zeroAdd; i++)
	{
		uint16_t reverseBit=reverseBits(digitsCount, i);
		if(reverseBit>=i_max)
		{
			buf[cur_buf][i]=complex<uint16_t>(0,0);
		}
		else
		{
			buf[cur_buf][i] = f[reverseBit];
		}
	}



	int exp_divider = 1;
	int different_exps = 2;
	int values_in_row = zeroAdd / 2;
	uint8_t  next_buf = 1;



	for (uint8_t step = 0; step < digitsCount; step++)
	{
		for (uint8_t n = 0; n < different_exps; n++)
		{
			complex<uint16_t> xp = exp((exp_dir * PI * n / exp_divider) * I);

			for (uint16_t k = 0; k < values_in_row; k++)
			{
				complex<uint16_t>* pf = &buf[cur_buf][2 * k + (n % (different_exps / 2)) * (values_in_row * 2)];
				buf[next_buf][n * values_in_row + k] = (*pf) + (*(pf + 1)) * xp;
			}
		}

		exp_divider *= 2;
		different_exps *= 2;
		values_in_row /= 2;
		cur_buf = next_buf;
		next_buf = (cur_buf + 1) % 2;
	}


	// Norming, saving the result
	for (int i = 0; i < i_max; i++)
	{
		F[i] = buf[cur_buf][i];
	}

	// Freeing our temporary buffers
//	for (int i = 0; i < 2; i++)
//	{
//		delete [] buf[i];
//	}
//	delete [] buf;

}



#endif




//void discreteFourierFast2D(const complex<float>* f, int i_max, int j_max, complex<float>* F, fourier_transform_direction ftd)
//{
//	complex<float>* phi = new complex<float>[i_max * j_max];
//	for (int m = 0; m < j_max; m++)
//	{
//		discreteFourierFast(&f[i_max * m], i_max, &phi[i_max * m], ftd);
//	}
//
//	complex<float>* phi_t = new complex<float>[j_max * i_max];
//
//	for (int p = 0; p < i_max; p++)
//	for (int q = 0; q < j_max; q++)
//	{
//		phi_t[p * j_max + q] = phi[q * i_max + p];
//	}
//
//	complex<float>* F_t = phi;
//
//	for (int i = 0; i < i_max; i++)
//	{
//		discreteFourierFast(&phi_t[j_max * i], j_max, &F_t[j_max * i], ftd);
//	}
//
//	for (int q = 0; q < j_max; q++)
//	for (int p = 0; p < i_max; p++)
//	{
//		F[q * i_max + p] = F_t[p * j_max + q];
//	}
//
//	delete [] F_t;
//	delete [] phi_t;
//}
