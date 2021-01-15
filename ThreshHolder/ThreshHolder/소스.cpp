#include <stdio.h>
#include <Windows.h>
#include <stdlib.h>

#pragma warning(disable : 4996)

BITMAPFILEHEADER bfh;
BITMAPINFOHEADER bih;

RGBQUAD* rgb;
unsigned char * pix;
unsigned char * pix_bmp;
int * pixE;
int main(void)
{
	FILE * fp;
	FILE * fp2;
	int oldpixel;
	int newpixel;
	int quant_error;
	int bpl;
	//int err;
	
	fp = fopen("EDIMAGE.bmp", "rb");

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
	pix = (unsigned char *)malloc(sizeof(unsigned char) * bpl * bih.biHeight);
	pix_bmp = (unsigned char *)malloc(sizeof(unsigned char) * bpl * bih.biHeight);
	memset(pix_bmp, 0, sizeof(unsigned char) * bpl * bih.biHeight);
	fread(pix, sizeof(unsigned char), bpl * bih.biHeight, fp);

	pixE = (int *)malloc(sizeof(int) * bpl * bih.biHeight);
	memset(pixE, 0, sizeof(int) * bpl * bih.biHeight);

	for (int y = 1; y < bih.biHeight - 1; y++)
	{
		quant_error = 0;
		for (int x = 1; x < bpl - 1; x++)
		{
			// ����ó���� ������ �κ�.
			oldpixel = pix[y * bpl + x] & 0x0000ff;
			pixE[y * bpl + x] += oldpixel;
			pix[y * bpl + x] = pixE[y * bpl + x] / 128 * 255;
	
			quant_error = pixE[y * bpl + x] - pix[y * bpl + x];

			pixE[y * bpl + x + 1] += quant_error * 7 / 16;
			pixE[(y + 1) * bpl + x - 1] += quant_error * 3 / 16;
			pixE[(y + 1) * bpl + x] += quant_error * 5 / 16;
			pixE[(y + 1) * bpl + x + 1] += quant_error * 1 / 16;
			// pix[y * bih.biWidth + x + 1] += pixE[y * bih.biWidth + x + 1];
		}
	}

	// ����� �ڵ�
	/*
	//int bpl4;
	//bpl4 = (bih.biWidth * 4 + 3) / 4 * 4;
	//pixE = (int*)malloc(sizeof(int) * bpl4 * bih.biHeight);
	//memset(pixE, 0, sizeof(int) * bpl4 * bih.biHeight);
	for (int y = 1; y < bih.biHeight - 1; y++)
	{
		quant_error = 0;
		if (1) // y % 2)
		{
			for (int x = 1; x < bih.biWidth - 1; x++)
			{
				oldpixel = pix[y * bpl + x];

				pixE[y * bpl4 + x] += oldpixel;
				pix[y * bpl + x] = newpixel = pixE[y * bpl4 + x] / 128 * 255;

				quant_error = pixE[y * bpl4 + x] - pix[y * bpl + x];

				pixE[y * bpl4 + x + 1] += quant_error * 7 / 16;
				pixE[(y + 1) * bpl4 + x - 1] += quant_error * 3 / 16;
				pixE[(y + 1) * bpl4 + x] += quant_error * 5 / 16;
				pixE[(y + 1) * bpl4 + x + 1] += quant_error * 1 / 16;
			}
		}
		else
		{
			for (int x = bih.biWidth - 2; x >= 1; x--)
			{
				oldpixel = pix[y * bpl + x];

				pixE[y * bpl4 + x] += oldpixel;
				pix[y * bpl + x] = newpixel = pixE[y * bpl4 + x] / 128 * 255;

				quant_error = pixE[y * bpl4 + x] - pix[y * bpl + x];

				pixE[y * bpl4 + x - 1] += quant_error * 7 / 16;
				pixE[(y + 1) * bpl4 + x + 1] += quant_error * 3 / 16;
				pixE[(y + 1) * bpl4 + x] += quant_error * 5 / 16;
				pixE[(y + 1) * bpl4 + x - 1] += quant_error * 1 / 16;
			}
		}
	}
	*/


	/*
	err = 0;
	for (int i = 0; i < bih.biWidth * bih.biHeight; i++)
	{	
		
		// error diffusion �ڵ�
		err = err + pix[i];
		if (err >= 255)
		{
			pix[i] = 255;
			err = err - 255;

		}
		else
		{
			pix[i] = 0;
		}
		
		// TreshHolder
		// pix_bmp[i] = pix[i] / 128 * 255;
		
	}
	*/


	// ������ �ȼ����� bmp���Ϸ� ����.
	fp2 = fopen("new_EDIMAGE.bmp", "wb");
	fwrite(&bfh, sizeof(bfh), 1, fp2);
	fwrite(&bih, sizeof(bih), 1, fp2);
	fwrite(rgb, sizeof(RGBQUAD), 256, fp2);

	fwrite(pix, sizeof(unsigned char), bpl * bih.biHeight, fp2);
	fclose(fp2);


	// ������ �ȼ����� ȭ�鿡 ���
	/*
	HDC hdc;
	hdc = GetDC(NULL);

	
	SetDIBitsToDevice(hdc, 0, 0, bih.biWidth, bih.biHeight, 0, 0, 0, bih.biHeight,
		(BYTE*)pix, (const BITMAPINFO *)&bih, DIB_PAL_COLORS);

	ReleaseDC(NULL, hdc);
	*/

	free(rgb);
	free(pix);
	free(pix_bmp);
	free(pixE);
	fclose(fp);

	return 0;
}