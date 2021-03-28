#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "mask.h"
#include "tiff.h"
#include "tiffconf.h"
#include "tiffio.h"
#include "tiffvers.h"

void *icc_profile = NULL;
uint32 count = 0;
BYTE* pix_buf;
BYTE* pix_out;
long width, height;
long pix_size;
float xres, yres;
uint32 min_color;
//int pad;
//BYTE trash[3] = { 0 }; // ������ ��

unsigned int bit8_array[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
int main(void)
{
	TIFF *input;
	TIFF *output;
	// tiff ���� �б�
	input = TIFFOpen("input.tif", "rb");

	if (input == NULL)
	{
		printf("File Not Found!!\n");
		exit(0);
	}

	TIFFGetField(input, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(input, TIFFTAG_IMAGELENGTH, &height);
	TIFFGetField(input, TIFFTAG_ICCPROFILE, &count, &icc_profile);
	TIFFGetField(input, TIFFTAG_XRESOLUTION, &xres);
	TIFFGetField(input, TIFFTAG_YRESOLUTION, &yres);
	TIFFGetField(input, TIFFTAG_YRESOLUTION, &yres);
	TIFFGetField(input, TIFFTAG_PHOTOMETRIC, &min_color);
	pix_buf = (BYTE *)calloc(width , sizeof(BYTE));	// line scan memory
	pix_out = (BYTE *)calloc((width + 7) / 8, sizeof(BYTE));	// line output memory
	pix_size = width * height;
	printf("width : %d , height : %d\n", width, height);
	printf("image size : %d X %d\n", width, height);
	printf("resolution : %f X %f\n", xres, yres);
	printf("min_color : %d\n", min_color);

	// ����� ��� tiff ���� ����
	output = TIFFOpen("output_Stochastic.tif", "wb");
	TIFFSetField(output, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(output, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(output, TIFFTAG_ICCPROFILE, icc_profile);
	TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, 1);
	TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, 1);
	TIFFSetField(output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	TIFFSetField(output, TIFFTAG_XRESOLUTION, xres);
	TIFFSetField(output, TIFFTAG_YRESOLUTION, yres);
	TIFFSetField(output, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	//TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(output, TIFFTAG_PHOTOMETRIC, min_color);
	TIFFSetField(output, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);

	for (int y = 0; y < height; y++)
	{
		TIFFReadScanline(input, pix_buf, y, 0);
		memset(pix_out, 0, (width + 7) / 8);
		for (int x = 0; x < width; x++)
		{

			if (pix_buf[x] != 0 && pix_buf[x] >= mask[(y % ms_y)][(x % ms_x)])
			{
				pix_out[x / 8] += bit8_array[x % 8];
			}
		}
		TIFFWriteScanline(output, pix_out, y, 0);
	}

	TIFFClose(input);
	TIFFClose(output);

	free(pix_buf);
	free(pix_out);
	return 0;
}

