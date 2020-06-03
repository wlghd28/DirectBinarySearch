#include <stdio.h>
#include <process.h>
#include <Windows.h>
#include <string.h>
#include <tchar.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#pragma warning(disable : 4996)
// �ð��� ����ϱ� ���� ����
double total_Time_GPU, total_Time_CPU, tmp_time;
LARGE_INTEGER beginClock, endClock, clockFreq;
LARGE_INTEGER tot_beginClock, tot_endClock, tot_clockFreq;

// ��Ʈ�� �̹��� ������ �޾ƿ��� ���� ����ü ����
BITMAPFILEHEADER bfh;
BITMAPINFOHEADER bih;
RGBQUAD * rgb;

// �̹��� ������ �ٷ�� ���� ����ϴ� ����
int bpl, bph;
unsigned char* pix; // ���� �̹���
//unsigned char* pix_ms; // ����þ� ���͸��� �� �̹���
unsigned char* pix_hvs; // ������ �̹���
unsigned char* pix_check; // ���� ������ �ȼ����� �ƴ��� �Ǵ�
//double* err;
int * pixE;

double T[128][128];
int ts = 128;	// threshold array size
int D = 7;

double pi = 3.14159265358979323846264338327950288419716939937510;

double G[9][9];
int fs = 9;	// ����þ� ���� ������
double CPP[17][17];
int halfcppsize = 8;

/*
double G[11][11];
int fs = 11;	// ����þ� ���� ������
double CPP[21][21];
int halfcppsize = 10;
*/
double* CEP;

char str[100];				// ���ϸ��� ���� ���ڿ�

//void GaussianFilter(float thresh);		// ����þ� ����
void DBS();					// Direct Binary Search ����
void GaussianFilter();		// ����þ� ���� ����
double GaussianRandom(double stddev, double average);		// ���Ժ��� ���� ����
void CONV();	// 2���� ������� ����
void XCORR();	// ��ȣ������� ����
void Threshold(int n);
void BayerMatrix(int n);		// ���� ����� �� �� �� �Ӱ谪 ��� ����
double Uniformity();			// �Ӱ谪 ����� ���ϼ��� ���ϴ� �Լ�
void Halftone();				// �ʱ� ������ �̹��� ����
void Dither();				// ����� �۾�
void FwriteCPU(char *);		// ����� �ȼ����� bmp���Ϸ� �����ϴ� �Լ�

int main(void)
{
	FILE * fp;
	//fp = fopen("newEDIMAGE.bmp", "rb");
	fp = fopen("newEDIMAGE2.bmp", "rb");
	//fp = fopen("test.bmp", "rb");
	//fp = fopen("bird2_720_C.bmp", "rb");

	if (fp == NULL)
	{
		printf("File not found!!\n");
		return 0;
	}

	fread(&bfh, sizeof(bfh), 1, fp);
	fread(&bih, sizeof(bih), 1, fp);

	rgb = (RGBQUAD*)malloc(sizeof(RGBQUAD) * 256);
	fread(rgb, sizeof(RGBQUAD), 256, fp);

	// BPL�� �����ֱ� ���ؼ� �ȼ��������� ����� 4�� ����� ����
	bpl = (bih.biWidth + 3) / 4 * 4;
	bph = (bih.biHeight + 3) / 4 * 4;
	//printf("%d %d\n", bpl, bph);

	pix = (unsigned char *)calloc(bpl * bph, sizeof(unsigned char));
	fread(pix, sizeof(unsigned char), bpl * bph, fp);

	//pix_ms = (unsigned char *)calloc(bpl * bph, sizeof(unsigned char));
	//memcpy(pix_ms, pix, sizeof(unsigned char) * bpl * bph);

	pix_hvs = (unsigned char *)calloc(bpl * bph, sizeof(unsigned char));
	//memcpy(pix_hvs, pix, sizeof(unsigned char) * bpl * bph);

	//pix_check = (unsigned char *)calloc(bpl * bph, sizeof(unsigned char));

	//err = (double *)calloc(bpl * bph, sizeof(double));

	CEP = (double *)calloc((bpl + halfcppsize * 2) * (bph + halfcppsize * 2), sizeof(double));

	printf("%p %p %p \n", CEP, pix, pix_hvs);

	//pixE = (int *)calloc(bpl * bph, sizeof(int));

	// Memory allocation ���� ó��
	if (CEP == NULL || pix == NULL || pix_hvs == NULL || rgb == NULL)
	{
		printf("Memory allocation fail!\n");
		return 0;
	}

	QueryPerformanceFrequency(&tot_clockFreq);	// �ð��� �����ϱ����� �غ�

	total_Time_CPU = 0;
	QueryPerformanceCounter(&tot_beginClock); // �ð����� ����
	//Dither();
	// Direct Binary Search �����
	DBS();
	QueryPerformanceCounter(&tot_endClock);

	total_Time_CPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_DBS : %f ms\n", total_Time_CPU * 1000);
	//system("pause");

	//sprintf(str, "new_DBS_Dither.bmp");
	sprintf(str, "new_DBS_Dither2.bmp");
	//sprintf(str, "test_DBS_Dither.bmp");
	//sprintf(str, "bird2_DBS_Dither_720_C.bmp");

	FwriteCPU(str);

	free(rgb);
	free(pix);
	//free(pix_ms);
	free(pix_hvs);
	//free(pix_check);
	//free(err);
	free(CEP);
	//free(pixE);
	fclose(fp);

	return 0;
}

void DBS()
{
	int count = 0;			// �ּ��������� ���� 0�� �ƴҶ����� �ݺ����� ���������� ī��Ʈ ����
	double eps = 0.0;
	double eps_min = 0.0;
	int a0 = 0;
	int a1 = 0;
	int a0c = 0;
	int a1c = 0;
	int cpx = 0;
	int cpy = 0;


	//Dither();
	Halftone();				// �ʱ� �������̹��� ����

	GaussianFilter();		// ����þ� ���� ����

	CONV();		// 2���� ������� ���� ��� ���� (CPP)
	/*
	for (int y = 0; y < bph; y++)
	{
		for (int x = 0; x < bpl; x++)
		{
			err[y * bpl + x] = (double)pix_hvs[y * bpl + x] / 255 - (double)pix[y * bpl + x] / 255;
		}
	}
	*/
	XCORR();	// ��ȣ���迬�� ��� ���� (CEP)

	// DBS ���� ����..
	while (1)
	{
		count = 0;
		a0 = 0;
		a1 = 0;
		a0c = 0;
		a1c = 0;
		cpx = 0;
		cpy = 0;

		for (int i = 1; i < bph - 1; i++)
		{
			for (int j = 1; j < bpl - 1; j++)
			{
				//if (pix_check[i * bpl + j] == 0)
				{
					a0c = 0;
					a1c = 0;
					cpx = 0;
					cpy = 0;
					eps_min = 0;
					for (int y = -1; y <= 1; y++)
					{
						//if (i + y < 0 || i + y >= bph)
							//continue;			
						for (int x = -1; x <= 1; x++)
						{
							//if (j + x < 0 || j + x >= bpl)
								//continue;
							if (y == 0 && x == 0)
							{
								if (pix_hvs[i * bpl + j] == 255)
								{
									a0 = -1;
									a1 = 0;
								}
								else
								{
									a0 = 1;
									a1 = 0;
								}
							}
							else
							{
								if (pix_hvs[(i + y) * bpl + (j + x)] != pix_hvs[i * bpl + j])
								{
									if (pix_hvs[i * bpl + j] == 255)
									{
										a0 = -1;
										a1 = (-1) * a0;
									}
									else
									{
										a0 = 1;
										a1 = (-1) * a0;
									}
								}
								else
								{
									a0 = 0;
									a1 = 0;
								}
							}
							eps = (a0 * a0 + a1 * a1) * CPP[halfcppsize][halfcppsize]
								+ 2 * a0 * a1 * CPP[halfcppsize + y][halfcppsize + x]
								+ 2 * a0 * CEP[(i + halfcppsize) * (bpl + halfcppsize * 2) + (j + halfcppsize)]
								+ 2 * a1 * CEP[(i + y + halfcppsize) * (bpl + halfcppsize * 2) + (j + x + halfcppsize)];
							if (eps_min > eps)
							{
								eps_min = eps;
								a0c = a0;
								a1c = a1;
								cpx = x;
								cpy = y;
							}
						}
					}
					if (eps_min < 0.0)
					{
						for (int y = (-1) * halfcppsize; y <= halfcppsize; y++)
						{
							for (int x = (-1) * halfcppsize; x <= halfcppsize; x++)
							{
								CEP[(i + y + halfcppsize) * (bpl + halfcppsize * 2) + (j + x + halfcppsize)] += (double)a0c * CPP[y + halfcppsize][x + halfcppsize];
								CEP[(i + y + cpy + halfcppsize) * (bpl + halfcppsize * 2) + (j + x + cpx + halfcppsize)] += (double)a1c * CPP[y + halfcppsize][x + halfcppsize];
							}
						}
						//pix_hvs[i * bpl + j] = (unsigned char)((pix_hvs[i * bpl + j] / 255 + a0c) % 2) * 255;
						//pix_hvs[(cpy + i) * bpl + (j + cpx)] = (unsigned char)((pix_hvs[(cpy + i) * bpl + (j + cpx)] / 255 + a1c) % 2) * 255;
						pix_hvs[i * bpl + j] += (unsigned char)a0c * 255;
						pix_hvs[(cpy + i) * bpl + (j + cpx)] += (unsigned char)a1c * 255;
						count++;
					}
				}
			}
		}
		printf("%d\n", count);
		if (count == 0)
			break;
	}
}
// ����þ� ���� ����
void GaussianFilter()
{
	double sum = 0;
	double d = /*(fs - 1) / 6*/ 1.2;		// sigma
	double c;
	int gaulen = (fs - 1) / 2;

	for (int k = (-1) * gaulen; k <= gaulen; k++)
	{
		for (int l = (-1) * gaulen; l <= gaulen; l++)
		{
			// ���� ����þ� ���� ����	

			c = (k * k + l * l) / (2 * d * d);
			G[k + gaulen][l + gaulen] = exp((-1) * c) / (2 * pi * d * d);
			//sum += G[k + gaulen][l + gaulen];		


			// fs = 11 �϶� ����� ���� ������..
			/*
			c = (k * k + l * l) / (2 * d * d) + 1;
			G[k + gaulen][l + gaulen] = 1 / c;
			//sum += G[k + gaulen][l + gaulen];
			*/
		}
	}
	//printf("%lf\n", sum);
}
// 2���� ������� ����
void CONV()
{
	double sum = 0;
	for (int y = (-1) * fs + 1; y < fs; y++)
	{
		for (int x = (-1) * fs + 1; x < fs; x++)
		{
			sum = 0;
			for (int i = y; i < y + fs; i++)
			{
				for (int j = x; j < x + fs; j++)
				{
					if ((i >= 0 && j >= 0) && (i < fs && j < fs))
					{
						sum += (double)G[i - y][j - x] * G[i][j];
					}
				}
			}
			CPP[(y + fs - 1)][(x + fs - 1)] = (double)sum;
		}
	}
}
// ��ȣ������� ����
void XCORR()
{
	double sum = 0;
	for (int y = (-1) * halfcppsize * 2; y < bph; y++)
	{
		for (int x = (-1) * halfcppsize * 2; x < bpl; x++)
		{
			sum = 0;
			for (int i = y; i <= y + halfcppsize * 2; i++)
			{
				for (int j = x; j <= x + halfcppsize * 2; j++)
				{
					if ((i >= 0 && j >= 0) && (i < bph && j < bpl))
					{
						// err * CPP
						sum += (double)CPP[i - y][j - x] * ((double)pix_hvs[i * bpl + j] / 255 - (double)pix[i * bpl + j] / 255);
					}
				}
			}
			CEP[(y + halfcppsize * 2) * (bpl + halfcppsize * 2) + (x + halfcppsize * 2)] = (double)sum;
		}
	}
}
// DBS ������� ������� �Ӱ谪 ��� ������...
void Threshold(int n)
{
	// �ʱ� �Ӱ谪 ��Ŀ� 0 ~ D ���� �Ҵ�
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			//T[i][j] = (i * n + j) % 10;
			T[i][j] = rand() % 10;
		}
	}

	// DBS ��� �������� ���� ���ϼ��� �ִ밡 �ǵ��� �˰��� ����
	int count = 0;			// ��Ŭ���� �ּҰŸ� ���� ���ŵǴ� Ƚ��.
	double uT = 0;
	double uT_max = 0;
	double tmp = 0;
	// Swapping ���� ����..
	while (1)
	{
		count = 0;
		uT_max = Uniformity();
		//uT = Uniformity();
		for (int i = 0; i < ts; i++)
		{
			for (int j = 0; j < ts; j++)
			{
				//printf("%lf\n", uT);
				for (int y = -1; y <= 1; y++)
				{
					if (i + y < 0 || i + y >= ts)
						continue;
					for (int x = -1; x <= 1; x++)
					{
						if (j + x < 0 || j + x >= ts)
							continue;
						if (!(x == 0 && y == 0))
						{
							//swapping...
							tmp = T[i][j];
							T[i][j] = T[i + y][j + x];
							T[i + y][j + x] = tmp;
							uT = Uniformity();
							if (uT_max < uT)
							{
								uT_max = uT;
								count++;
							}
							else
							{
								tmp = T[i][j];
								T[i][j] = T[i + y][j + x];
								T[i + y][j + x] = tmp;
							}
						}
					}
				}
			}
			//printf("test2\n");
		}

		printf("Threshold count : %d\n", count);
		if (count == 0)
			break;
	}

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			printf("%lf ", T[i][j]);
		}
		printf("\n");
	}
}
// ���ϼ� �� ����
double Uniformity()
{
	double distance = 0;
	double sum = 0;
	double min;
	for (int i = 0; i < ts; i++)
	{
		for (int j = 0; j < ts; j++)
		{
			min = ts * ts * 2;
			for (int k = 1; k < ts; k++)
			{
				for (int y = -k; y <= k; y++)
				{
					if (i + y < 0 || i + y >= ts)
						continue;
					for (int x = -k; x <= k; x++)
					{
						if (j + x < 0 || j + x >= ts)
							continue;
						if (y != 0 && x != 0)
						{
							if (T[i + y][j + x] <= T[i][j])
							{
								distance = y * y + x * x;
								if (distance < min)
								{
									min = distance;
								}
							}
						}
					}
				}
				// �Ÿ��� �ּڰ��� ������ ��� �ݺ��� Ż��
				if (min != ts * ts * 2)
					break;
			}
			sum += min;
		}
	}

	return sum;
}
void BayerMatrix(int n)
{
	int count = 2;
	// �Ӱ谪 ��� �ʱⰪ ����
	T[0][0] = 0.0;
	T[0][1] = 2.0;
	T[1][0] = 3.0;
	T[1][1] = 1.0;

	while (count < n)
	{
		for (int i = 0; i < count; i++)
		{
			for (int j = 0; j < count; j++)
			{
				T[i][j] *= 4;
				T[i][j + count] = T[i][j] + 2;
				T[i + count][j] = T[i][j] + 3;
				T[i + count][j + count] = T[i][j] + 1;
			}
		}
		count *= 2;
	}

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			T[i][j] /= n * n;
			//printf("%lf ", (double)T[i][j]);
		}
		//printf("\n");
	}
}
/*
// ����þ� ����
void GaussianFilter(float thresh)
{
	float g = thresh;
	// 5 X 5 ����ũ
	float a = 1 / g;
	float b = 4 / g;
	float c = 6 / g;
	float d = 12 / g;
	float e = 24 / g;
	float f = 36 / g;
	// ����þ� ���� ����ũ �迭
	float G[5][5] =
	{
		a, b, c, b, a,
		b, d, e, d, b,
		c, e, f, e, c,
		b, d, e, d, b,
		a, b, c, b, a
	};
	for (int y = 1; y < bph - 1; y++)
	{
		for (int x = 1; x < bpl - 1; x++)
		{
			for (int k = 0; k < 5; k++)
			{
				for (int l = 0; l < 5; l++)
				{
					pix_ms[y * bpl + x] += pix[(y + k - 2) * bpl + (x + l - 2)] * G[k][l];
				}
			}
		}
	}

	// 3 X 3 ����ũ
	float a = 0 / g;
	float b = 1 / g;
	float c = 2 / g;
	float G[3][3] =
	{
		a, b, a,
		b, c, b,
		a, b, a
	};
	for (int y = 1; y < bph - 1; y++)
	{
		for (int x = 1; x < bpl - 1; x++)
		{
			for (int k = 0; k < 3; k++)
			{
				for (int l = 0; l < 3; l++)
				{
					pix_ms[y * bpl + x] += pix[(y + k - 1) * bpl + (x + l - 1)] * G[k][l];
				}
			}
		}
	}
}
*/
// 
double GaussianRandom(double stddev, double average)
{
	double v1, v2, s, temp;

	do {
		v1 = 2 * ((double)rand() / RAND_MAX) - 1;      // -1.0 ~ 1.0 ������ ��
		v2 = 2 * ((double)rand() / RAND_MAX) - 1;      // -1.0 ~ 1.0 ������ ��
		s = v1 * v1 + v2 * v2;
	} while (s >= 1 || s == 0);

	s = sqrt((-2 * log(s)) / s);

	temp = v1 * s;
	temp = (stddev * temp) + average;


	return temp;
}
void Halftone()
{
	// Thresh Hold
	/*
	for (int y = 1; y < bph - 1; y++)
	{
		for (int x = 1; x < bpl - 1; x++)
		{
			pix_hvs[y * bpl + x] = pix[y * bpl + x] / 128 * 255;
		}
	}
	*/

	// ���Ժ��� ������ �������̹��� ����

	srand(time(NULL));
	double tmp;

	for (int y = 1; y < bph - 1; y++)
	{
		for (int x = 1; x < bpl - 1; x++)
		{
			tmp = (double)GaussianRandom(1.0, 0);
			//printf("%lf\n", tmp);
			if (tmp > 0.5)
			{
				//if(pix_check[y * bpl + x] == 0)
				pix_hvs[y * bpl + x] = 255;
			}
		}
	}

}
void Dither()
{
	// Ordered Dither
	/*
	BayerMatrix(ts); // Threshold array ����

	for (int y = 1; y < bph - 1; y++)
	{
		for (int x = 1; x < bpl - 1; x++)
		{
			if (pix[y * bpl + x] > T[y % ts][x % ts] * 255)
			{
				pix_hvs[y * bpl + x] = 255;
				pix_check[y * bpl + x] = 1;
			}
		}
	}
	*/
	// Ordered Dithering based DBS

	Threshold(ts);	// Threshold array ����

	for (int y = 1; y < bph - 1; y++)
	{
		for (int x = 1; x < bpl - 1; x++)
		{
			if (pix[y * bpl + x] < D)
			{
				//pix_hvs[y * bpl + x] = pix[y * bpl + x];
				if (pix[y * bpl + x] > T[y % ts][x % ts])
				{
					pix_hvs[y * bpl + x] = 255;
					pix_check[y * bpl + x] = 1;
				}
			}
			else if (pix[y * bpl + x] > 255 - D)
			{
				//pix_hvs[y * bpl + x] = pix[y * bpl + x];
				if (pix[y * bpl + x] < 255 - T[y % ts][x % ts])
				{
					pix_hvs[y * bpl + x] = 0;
					pix_check[y * bpl + x] = 1;
				}
				else
				{
					pix_hvs[y * bpl + x] = 255;
				}
			}

		}
	}

	// Floyd Steinberg �����
	/*
	int quant_error = 0;	// �ʱ� ����� �۾��� �Ҷ� ���� ���� ����ġ ����

	for (int y = 1; y < bph - 1; y++)
	{
		if (y % 2 == 1)
		{
			for (int x = 1; x < bpl - 1; x++)
			{
				pixE[y * bpl + x] += pix_hvs[y * bpl + x];
				pix_hvs[y * bpl + x] = pixE[y * bpl + x] / 128 * 255;
				quant_error = pixE[y * bpl + x] - pix_hvs[y * bpl + x];

				pixE[y * bpl + x + 1] += quant_error * 7 / 16;
				pixE[(y + 1) * bpl + x - 1] += quant_error * 3 / 16;
				pixE[(y + 1) * bpl + x] += quant_error * 5 / 16;
				pixE[(y + 1) * bpl + x + 1] += quant_error * 1 / 16;
			}
		}
		else
		{
			for (int x = bpl - 2; x >= 1; x--)
			{
				pixE[y * bpl + x] += pix_hvs[y * bpl + x];
				pix_hvs[y * bpl + x] = pixE[y * bpl + x] / 128 * 255;
				quant_error = pixE[y * bpl + x] - pix_hvs[y * bpl + x];

				pixE[y * bpl + x - 1] += quant_error * 7 / 16;
				pixE[(y + 1) * bpl + x + 1] += quant_error * 3 / 16;
				pixE[(y + 1) * bpl + x] += quant_error * 5 / 16;
				pixE[(y + 1) * bpl + x - 1] += quant_error * 1 / 16;
			}
		}
	}
	*/
	// Floyd Steinberg �ܹ���
	/*
	int quant_error = 0;	// �ʱ� ����� �۾��� �Ҷ� ���� ���� ����ġ ����

	for (int y = 1; y < bph - 1; y++)
	{
		for (int x = 1; x < bpl - 1; x++)
		{

			pixE[y * bpl + x] += pix_hvs[y * bpl + x];
			pix_hvs[y * bpl + x] = pixE[y * bpl + x] / 128 * 255;
			quant_error = pixE[y * bpl + x] - pix_hvs[y * bpl + x];

			pixE[y * bpl + x + 1] += quant_error * 7 / 16;
			pixE[(y + 1) * bpl + x - 1] += quant_error * 3 / 16;
			pixE[(y + 1) * bpl + x] += quant_error * 5 / 16;
			pixE[(y + 1) * bpl + x + 1] += quant_error * 1 / 16;

		}
	}
	*/
}
void FwriteCPU(char * fn)
{
	// ������ �ȼ����� bmp���Ϸ� ����.
	FILE * fp2 = fopen(fn, "wb");
	fwrite(&bfh, sizeof(bfh), 1, fp2);
	fwrite(&bih, sizeof(bih), 1, fp2);
	fwrite(rgb, sizeof(RGBQUAD), 256, fp2);

	fwrite(pix_hvs, sizeof(unsigned char), bpl * bph, fp2);
	//fwrite(pixE, sizeof(int), bpl * bph, fp2);
	fclose(fp2);
}