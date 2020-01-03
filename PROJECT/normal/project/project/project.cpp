#include "pch.h"
#include <windows.h>
#include <ctime>
#include <vector>
using namespace std;

#define BITMAP_FILE_SIGNATURE 'MB'

#pragma pack(push)
#pragma pack(1)
typedef struct _BMP_HEADER
{
	WORD Signature;
	DWORD FileSize;
	WORD Reserved1;
	WORD Reserved2;
	DWORD PixelArrayRVA;
} BMP_HEADER, *PBMP_HEADER;

typedef struct _DIB_HEADER
{
	DWORD HeaderSize;
	DWORD Width;
	DWORD Height;
	WORD NoColorPanes;
	WORD BitsPerPixel;
	DWORD CompressionMethod;
	DWORD BitmapSize;
	DWORD HorizontalResolution;
	DWORD VerticalResolution;
	DWORD NumberOfColorsInPalette;
	DWORD NumberOfImportantColors;
} DIB_HEADER, *PDIB_HEADER;

typedef struct _PIXEL
{
	BYTE red;
	BYTE green;
	BYTE blue;
} PIXEL, *PPIXEL;

typedef struct _THREAD_ARG
{
	DWORD start;
	DWORD finish;

} THREAD_ARG, *PTHREAD_ARG;


#pragma pack(pop)


vector<PIXEL> pixels;
PBYTE result;

DWORD gBitmapStart, gWidth, gFileSize, gHeight;

//
// PpdReadAndProcessBmpFile
//
BOOLEAN
ReadAndProcessBmpFile(
	PCHAR FileName
)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hMapping = INVALID_HANDLE_VALUE;
	PVOID pFileMapping = NULL;
	DWORD width, height;


	hFile = CreateFileA(FileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("[ERROR] CreateFileA returned: %d\n", GetLastError());
		return 0;
	}

	hMapping = CreateFileMappingA(hFile,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL);

	if (hMapping == INVALID_HANDLE_VALUE)
	{
		printf("[ERROR] CreateFileMappingA returned: %d\n", GetLastError());
		return 0;
	}

	pFileMapping = MapViewOfFile(hMapping,
		FILE_MAP_READ,
		0,
		0,
		0);

	if (pFileMapping == NULL)
	{
		printf("[ERROR] MapViewOfFile returned: %d\n", GetLastError());
		return 0;
	}

	PBMP_HEADER pBmp = (PBMP_HEADER)pFileMapping;
	if (pBmp->Signature != BITMAP_FILE_SIGNATURE)
	{
		printf("[ERROR] The input image has different signature than normal signature!\n");
		return 0;
	}

	PDIB_HEADER pDib = (PDIB_HEADER)((PBYTE)pFileMapping + sizeof(BMP_HEADER));

	if (pDib->BitsPerPixel != 24)
	{
		printf("[ERROR] Only images with 24 bits per pixels are supported\n");
		return 1;
	}

	if (pDib->CompressionMethod != BI_RGB)
	{
		printf("[ERROR] Only not compressed images are currently supported\n");
		return 0;
	}

	PBYTE pBitmapStart = (PBYTE)pFileMapping + pBmp->PixelArrayRVA;

	// init the global for the threads to know where to write
	gBitmapStart = pBmp->PixelArrayRVA;
	gWidth = pDib->Width;
	gHeight = pDib->Height;
	gFileSize = pBmp->FileSize;

	width = pDib->Width;
	height = pDib->Height;

	printf("[INFO] width = %d, height = %d, start of bitmap is located at %p\n", width, height, pBitmapStart);

	printf("[INFO] First two bytes of the bitmap are: %x %x\n", pBitmapStart[0], pBitmapStart[1]);

	int i, j;
	int currentIndexInArray = 0;
	for (i = 0; i < height; i++)
	{
		// quick mafs
		int nrBytesRemaining = (4 - (3 * width) % 4) % 4;

		for (j = 0; j < width; j++)
		{
			PIXEL current;
			current.blue = pBitmapStart[currentIndexInArray];
			current.green = pBitmapStart[currentIndexInArray + 1];
			current.red = pBitmapStart[currentIndexInArray + 2];

			pixels.push_back(current);
			currentIndexInArray += 3;
		}

		// padding
		currentIndexInArray += nrBytesRemaining;
	}

	PBYTE pInitialFile = (PBYTE)pFileMapping;

	result = (PBYTE)malloc(pBmp->FileSize);
	if (result == NULL)
	{
		printf("[ERROR] Cannot alloc result bitmap, will bail out\n");
		return 1;
	}

	// append to result the headers
	for (i = 0; i < 14 + pDib->HeaderSize; i++)
	{
		result[i] = pInitialFile[i];
	}

	return 1;
}

#define FILTER_SIZE 3

double gFilter[FILTER_SIZE][FILTER_SIZE] = {
	{0, 0.6, 0},
	{0, 1, 0},
	{0, -1, 0},
};



//
// PpdWorkerThreadFunction
//
VOID
PpdWorkerThreadFunction(
	LPVOID Parameter
)
{
	int i, j;
	PTHREAD_ARG pArg = (PTHREAD_ARG)Parameter;
	int nrBytesRemaining = (4 - (3 * gWidth) % 4) % 4;
	int lastAddrWritten;

	for (i = pArg->start; i <= pArg->finish; i++)
	{
		for (j = 0; j < gWidth; j++)
		{
			int currentPos = i * (gWidth * 3 + nrBytesRemaining) + j * 3;
			int currentPixelPos = i * gWidth + j;

			PIXEL current = pixels[currentPixelPos];
			PIXEL transformed;

			int fred = 0, fgreen = 0, fblue = 0;

			for (int k = -FILTER_SIZE / 2; k <= FILTER_SIZE / 2; k++)
			{
				if (i + k >= gHeight || i + k < 0)
				{
					continue;
				}
				for (int l = -FILTER_SIZE / 2; l <= FILTER_SIZE / 2; l++)
				{
					if (j + l < 0 || j + l >= gWidth)
					{
						continue;
					}

					PIXEL ppixel = pixels[(i + k)*gWidth + j + l];

					fred += (int)ppixel.red * gFilter[k + 1][l + 1];
					fgreen += (int)ppixel.green * gFilter[k + 1][l + 1];
					fblue += (int)ppixel.blue * gFilter[k + 1][l + 1];

				}
			}

			transformed.red = fred % 256;
			transformed.green = fgreen % 256;
			transformed.blue = fblue % 256;

			result[gBitmapStart + currentPos] = transformed.blue;
			result[gBitmapStart + currentPos + 1] = transformed.green;
			result[gBitmapStart + currentPos + 2] = transformed.red;

			lastAddrWritten = gBitmapStart + currentPos + 2;

		}

	}

}


int main(int argc, char* argv[])
{

	HANDLE hFile;
	PCHAR input = NULL, output = NULL;
	int nThreads = 0;

	clock_t start = clock();

	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0)
		{
			printf("[HELP] normal.exe -i <input> -o <output> -t <nr threads>\n");
			return 0;
		}
		if (strcmp(argv[i], "-i") == 0)
		{
			if (i + 1 < argc)
			{
				input = argv[i + 1];
			}
		}
		if (strcmp(argv[i], "-o") == 0)
		{
			if (i + 1 < argc)
			{
				output = argv[i + 1];
			}
		}
		if (strcmp(argv[i], "-t") == 0)
		{
			if (i + 1 < argc)
			{
				nThreads = atoi(argv[i + 1]);
			}
		}
	}

	if (nThreads == 0)
	{
		printf("[WARNING] No number of threads given, will default to 1\n");
		nThreads = 1;
	}

	if (!ReadAndProcessBmpFile(input))
	{
		printf("[ERROR] Failed to process %s...\n", input);
		return 1;
	}

	int i;
	PHANDLE pTids = (PHANDLE)malloc(sizeof(HANDLE)*nThreads);
	PTHREAD_ARG pArgs = (PTHREAD_ARG)malloc(sizeof(THREAD_ARG)*nThreads);

	for (i = 0; i < nThreads; i++)
	{
		if (i == 0)
		{
			pArgs[i].start = 0;
		}
		else
		{
			pArgs[i].start = pArgs[i - 1].finish + 1;
		}
		pArgs[i].finish = pArgs[i].start + gHeight / nThreads - 1;
		if (gHeight % nThreads > i)
		{
			pArgs[i].finish++;
		}
		pTids[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PpdWorkerThreadFunction, &pArgs[i], 0, NULL);
		if (pTids[i] == INVALID_HANDLE_VALUE)
		{
			printf("[ERROR] Creating thread %d\n", i);
		}
	}

	for (i = 0; i < nThreads; i++)
	{

		if (pTids[i] != INVALID_HANDLE_VALUE)
		{
			if (WaitForSingleObject(pTids[i], INFINITE) != STATUS_WAIT_0)
			{
				printf("[ERROR] Failed wait for thread %d\n", i);
			}
		}
	}

	hFile = CreateFileA(output, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("[ERROR] Cannot oppen output file %s.\n", output);
		return 1;
	}
	if (!WriteFile(hFile, result, gFileSize, NULL, NULL))
	{
		printf("[ERROR] Failed to write the output file %s!\n", output);
		return 1;
	}

	printf("[INFO] Output file %s written!\n", output);
	clock_t end = clock();

	printf("[INFO] Finished in: %.5f\n", (double)(end - start) / (double)CLOCKS_PER_SEC);

	return 0;
}
