#include "pch.h"
#include <iostream>
#include <ctime>
#include <windows.h>
#include <CL/cl.hpp>

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


vector<BYTE> pixels;
PBYTE result;

DWORD gBitmapStart, gWidth, gFileSize, gHeight;

//
// PpdReadAndProcessBmpFile
//
BOOLEAN
PpdReadAndProcessBmpFile(
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

			pixels.push_back(current.red);
			pixels.push_back(current.green);
			pixels.push_back(current.blue);
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

double gFilter[FILTER_SIZE * FILTER_SIZE] =
{ 0, 0.6, 0,
 0, 0.5, 0,
 0, -1, 0 };



int main(int argc, char* argv[])
{
	HANDLE hFile;
	PCHAR input = NULL, output = NULL;
	int me, size;

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
	}

	//if (input == NULL)
	//{
	//	printf("[WARNING] No input given, will default to input.bmp\n");
	//	//input = "input.bmp";
	//	strcpy(input, "input.bmp");
	//}
	//if (output == NULL)
	//{
	//	printf("[WARNING] No output given, will default to output.bmp\n");
	//	//output = "output.bmp";
	//	strcpy(output, "output.bmp");
	//}

	if (!PpdReadAndProcessBmpFile(input))
	{
		printf("[ERROR] Failed to process %s...\n", input);
		return 1;
	}


	// get all platforms (drivers), e.g. NVIDIA
	std::vector<cl::Platform> all_platforms;
	cl::Platform::get(&all_platforms);

	if (all_platforms.size() == 0) {
		std::cout << " No platforms found. Check OpenCL installation!\n";
		exit(1);
	}
	cl::Platform default_platform = all_platforms[1];
	std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

	// get default device (CPUs, GPUs) of the default platform
	std::vector<cl::Device> all_devices;
	default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
	if (all_devices.size() == 0) {
		std::cout << " No devices found. Check OpenCL installation!\n";
		exit(1);
	}

	cout << "nr of devices: " << all_devices.size() << "\n";
	cl::Device default_device = all_devices[0];
	std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << "\n";

	cl::Context context({ default_device });

	// create the program that we want to execute on the device
	cl::Program::Sources sources;

	// calculates for each element; C = A + B
	std::string kernel_code =
		"   void kernel make_filter(global const unsigned char* Image,"
		"                           global const int* filterSize, global const double* Filter, global const int* width, global const int* height, global unsigned char* result, global const int* bitmapStart) {"
		"int gWidth = width[0], gHeight = height[0], fSize = filterSize[0] / 2, i, j, start, finish, k, l;"
		"int nrBytesRemaining = (4 - (3 * gWidth) % 4) % 4;"
		"int ID = get_global_id(0);"
		"int nrThreads = get_global_size(0);"
		"int ratio = gHeight/nrThreads;"
		"start = ratio*ID;"
		"finish = ratio*(ID+1);"
		"for (int i = start; i < finish; i++)"
		"{"
		"    for (int j = 0; j < gWidth * 3; j += 3)"
		"    {"
		"        int red = 0;"
		"        int green = 0;"
		"        int blue = 0;"
		"        "
		"        for (k = -1; k <= 1; k++)"
		"        {"
		"            if (i + k < 0 || i + k >= gHeight)"
		"                continue;"
		"            for (l = -1; l <= 1; l++)"
		"            {"
		"                if (j + l * 3 < 0 || j + l * 3 >= gWidth * 3)"
		"                    continue;"
		"                "
		"                int cred = Image[(i + k)*(gWidth*3)+j + l * 3];"
		"                int cgreen = Image[(i + k)*(gWidth*3)+j + 1 + l * 3];"
		"                int cblue = Image[(i + k)*(gWidth*3)+j + 2 + l * 3];"
		"                red += Filter[(k + fSize)*3+l + fSize] * cred;"
		"                green += Filter[(k + fSize)*3+l + fSize] * cgreen;"
		"                blue += Filter[(k + fSize)*3+l + fSize] * cblue;"
		"                red %= 256;"
		"                green %= 256;"
		"                blue %= 256;"
		"            }"
		"        }"
		"        result[bitmapStart[0] + i*(gWidth * 3 + nrBytesRemaining) + j] = blue;"
		"        result[bitmapStart[0] + i*(gWidth * 3 + nrBytesRemaining) + j + 1] = green;"
		"        result[bitmapStart[0] + i*(gWidth * 3 + nrBytesRemaining) + j + 2] = red;"
		"   "
		"    }"
		"}"

		"}";
	sources.push_back({ kernel_code.c_str(), kernel_code.length() });

	cl::Program program(context, sources);
	if (program.build({ default_device }) != CL_SUCCESS) {
		std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
		exit(1);
	}


	// create buffers on device (allocate space on GPU)
	cl::Buffer buffer_IMG(context, CL_MEM_READ_ONLY, sizeof(BYTE)*pixels.size());
	cl::Buffer buffer_fsize(context, CL_MEM_READ_ONLY, sizeof(int));
	cl::Buffer buffer_filter(context, CL_MEM_READ_ONLY, sizeof(double) * FILTER_SIZE * FILTER_SIZE);
	cl::Buffer buffer_width(context, CL_MEM_READ_ONLY, sizeof(int));
	cl::Buffer buffer_height(context, CL_MEM_READ_ONLY, sizeof(int));
	cl::Buffer buffer_result(context, CL_MEM_READ_WRITE, sizeof(BYTE)*gFileSize);
	cl::Buffer buffer_bitmapstart(context, CL_MEM_READ_ONLY, sizeof(int));

	// create a queue (a queue of commands that the GPU will execute)
	cl::CommandQueue queue(context, default_device);

	int fSize = FILTER_SIZE;

	// push write commands to queue
	queue.enqueueWriteBuffer(buffer_IMG, CL_TRUE, 0, sizeof(BYTE)*pixels.size(), pixels.data());
	queue.enqueueWriteBuffer(buffer_fsize, CL_TRUE, 0, sizeof(int), &fSize);
	queue.enqueueWriteBuffer(buffer_filter, CL_TRUE, 0, sizeof(double) * FILTER_SIZE*FILTER_SIZE, gFilter);
	queue.enqueueWriteBuffer(buffer_width, CL_TRUE, 0, sizeof(int), &gWidth);
	queue.enqueueWriteBuffer(buffer_height, CL_TRUE, 0, sizeof(int), &gHeight);
	queue.enqueueWriteBuffer(buffer_result, CL_TRUE, 0, sizeof(BYTE)*gFileSize, result);
	queue.enqueueWriteBuffer(buffer_bitmapstart, CL_TRUE, 0, sizeof(int), &gBitmapStart);

	clock_t start = clock();

	cl::Kernel make_filter(program, "make_filter");
	make_filter.setArg(0, buffer_IMG);
	make_filter.setArg(1, buffer_fsize);
	make_filter.setArg(2, buffer_filter);
	make_filter.setArg(3, buffer_width);
	make_filter.setArg(4, buffer_height);
	make_filter.setArg(5, buffer_result);
	make_filter.setArg(6, buffer_bitmapstart);

	queue.enqueueNDRangeKernel(make_filter, cl::NullRange, cl::NDRange(580), cl::NullRange);

	// read result from GPU to here
	queue.enqueueReadBuffer(buffer_result, CL_TRUE, 0, sizeof(BYTE)*gFileSize, result);

	hFile = CreateFileA(output, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("[ERROR] Cannot open output file %s.\n", output);
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