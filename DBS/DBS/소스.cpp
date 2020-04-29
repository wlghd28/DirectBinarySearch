#include <stdio.h>
#include <process.h>
#include <Windows.h>
#include <string.h>
#include <tchar.h>
#include <stdbool.h>
#include <stdlib.h>
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
unsigned char* pix_ms; // ����þ� ���͸��� �� �̹���
unsigned char* pix_hvs; // ������ �̹���
double* err;
int * pixE;

double pi = 3.141592553589792;
double G[5][5];
int fs = 5;	// ����þ� ���� ������
double CPP[9][9];
int halfcppsize = 4;
double* CEP;

char str[100];				// ���ϸ��� ���� ���ڿ�

//void GaussianFilter(float thresh);		// ����þ� ����
void DBS();					// Direct Binary Search ����
void GaussianFilter();		// ����þ� ���� ����
void CONV();	// 2���� ������� ����
void XCORR();	// ��ȣ������� ����

void Dither();				// �ʱ� ����� �۾�
void FwriteCPU(char *);		// ����� �ȼ����� bmp���Ϸ� �����ϴ� �Լ�

int main(void)
{
	FILE * fp;
	fp = fopen("EDIMAGE.bmp", "rb");

	fread(&bfh, sizeof(bfh), 1, fp);
	fread(&bih, sizeof(bih), 1, fp);

	rgb = (RGBQUAD*)malloc(sizeof(RGBQUAD) * 256);
	fread(rgb, sizeof(RGBQUAD), 256, fp);

	// BPL�� �����ֱ� ���ؼ� �ȼ��������� ����� 4�� ����� ����
	bpl = (bih.biWidth + 3) / 4 * 4;
	bph = (bih.biHeight + 3) / 4 * 4;

	pix = (unsigned char *)malloc(sizeof(unsigned char) * bpl * bph);
	memset(pix, 0, sizeof(unsigned char) * bpl * bph);
	fread(pix, sizeof(unsigned char), bpl * bph, fp);

	pix_ms = (unsigned char *)malloc(sizeof(unsigned char) * bpl * bph);
	memset(pix_ms, 0, sizeof(unsigned char) * bpl * bph);
	//memcpy(pix_ms, pix, sizeof(unsigned char) * bpl * bph);

	pix_hvs = (unsigned char *)malloc(sizeof(unsigned char) * bpl * bph);
	memset(pix_hvs, 0, sizeof(unsigned char) * bpl * bph);
	memcpy(pix_hvs, pix, sizeof(unsigned char) * bpl * bph);

	err = (double *)malloc(sizeof(double) * bpl * bph);
	memset(err, 0, sizeof(double) * bpl * bph);

	CEP = (double *)malloc(sizeof(double) * ((bpl + halfcppsize * 2) + 1) * ((bph + halfcppsize * 2) + 1));
	memset(CEP, 0, sizeof(double) * ((bpl + halfcppsize * 2) + 1) * ((bph + halfcppsize * 2) + 1));

	pixE = (int *)malloc(sizeof(int) * bpl * bph);
	memset(pixE, 0, sizeof(int) * bpl * bph);

	QueryPerformanceFrequency(&tot_clockFreq);	// �ð��� �����ϱ����� �غ�
	total_Time_CPU = 0;

	QueryPerformanceCounter(&tot_beginClock); // �ð����� ����
	// Direct Binary Search �����
	DBS();
	QueryPerformanceCounter(&tot_endClock);

	total_Time_CPU = (double)(tot_endClock.QuadPart - tot_beginClock.QuadPart) / tot_clockFreq.QuadPart;
	printf("Total processing Time_CPU_Single : %f ms\n", total_Time_CPU * 1000);
	//system("pause");

	sprintf(str, "DBS_Dither.bmp");
	FwriteCPU(str);

	free(rgb);
	free(pix);
	free(pix_ms);
	free(pix_hvs);
	free(err);
	free(CEP);
	free(pixE);
	fclose(fp);
	
	return 0;
}

void DBS()
{
	int count = 0;			// �ּ��������� ���� 0�� �ƴҶ����� �ݺ����� ���������� ī��Ʈ ����
	double eps = 0;
	double eps_min = 0;
	double a0 = 0;
	double a1 = 0;
	double a0c = 0;
	double a1c = 0;
	int cpx = 0;
	int cpy = 0;

	Dither();				// �ʱ� ����� �۾� (����� Floyd and Steinberg Dithering) 

	GaussianFilter();		// ����þ� ���� ����
	CONV();		// 2���� ������� ���� ��� ���� (CPP)
	for (int y = 0; y < bph; y++)
	{
		for (int x = 0; x < bpl; x++)
		{
			err[y * bpl + x] = pix_hvs[y * bpl + x] / 255 - (double)pix[y * bpl + x] / 255;
		}
	}
	XCORR();	// ��ȣ���迬�� ��� ���� (CEP)

	// DBS ���� ����..
	while(1)
	{
		count = 0;
		a0 = 0;
		a1 = 0;
		a0c = 0;
		a1c = 0;
		cpx = 0;
		cpy = 0;

		for (int i = 1; i < bph; i++)
		{
			for (int j = 1; j < bpl; j++)
			{
				a0c = 0;
				a1c = 0;
				cpx = 0;
				cpy = 0;
				eps_min = 0;
				for (int y = -1; y <= 1; y++)
				{
					if (i + y < 1 || i + y >= bph)
						continue;
					
					for (int x = -1; x <= 1; x++)
					{
						if (j + x < 1 || j + x >= bpl)
							continue;
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
				if (eps_min < 0)
				{
					for (int y = (-1) * halfcppsize; y <= halfcppsize; y++)
					{
						for (int x = (-1) * halfcppsize; x <= halfcppsize; x++)
						{
							CEP[(i + y + halfcppsize) * (bpl + halfcppsize * 2) + (j + x + halfcppsize)] += a0c * CPP[y + halfcppsize][x + halfcppsize];
						}
					}
					for (int y = (-1) * halfcppsize; y <= halfcppsize; y++)
					{
						for (int x = (-1) * halfcppsize; x <= halfcppsize; x++)
						{
							CEP[(i + y + cpy + halfcppsize) * (bpl + halfcppsize * 2) + (j + x + cpx + halfcppsize)] += a1c * CPP[y + halfcppsize][x + halfcppsize];
						}
					}
					pix_hvs[i * bpl + j] += (unsigned char)(a0c) * 255;
					pix_hvs[(cpy + i) * bpl + (j + cpx)] += (unsigned char)(a1c) * 255;
					count++;
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
	int d = 2;
	int gaulen = (fs - 1) / 2;
	double c;
	for (int k = (-1) * gaulen; k <= gaulen; k++)
	{
		for (int l = (-1) * gaulen; l <= gaulen; l++)
		{
			c = (k * k + l * l) / (2 * d * d);
			G[k + gaulen + 1][l + gaulen + 1] = exp((-1) * c) / (2 * pi * d * d);
		}
	}
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
						sum += G[i - y][j - x] * G[i][j];
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
			for (int i = y; i < y + halfcppsize * 2 + 1; i++)
			{
				for (int j = x; j < x + halfcppsize * 2 + 1; j++)
				{
					if ((i >= 0 && j >= 0) && (i < bph && j < bpl))
					{
						sum += CPP[i - y][j - x] * err[i * bpl + j];
					}
				}
			}
			CEP[(y + halfcppsize * 2) * (bpl + halfcppsize * 2) + (x + halfcppsize * 2)] = (double)sum;
		}
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
void Dither()
{
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