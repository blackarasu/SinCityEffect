// mainProgram.cpp : Ten plik zawiera funkcję „main”. W nim rozpoczyna się i kończy wykonywanie programu.
//

#include "pch.h"
#include <windows.h>
#include <chrono>
#include <iostream>
#include <string>
#include <stdio.h>
#include <thread>
#include <vector>
#include "Constatnts.h"
#include "../DLL_CPP/DLL_CPP.h"

extern "C" void _fastcall ProcedeS(int* hues, float* saturations, int H, int ammountToCalculate);//asm function
//RGB & HSL
float Min(float a, float b) {
	return a <= b ? a : b;
}

float Max(float a, float b) {
	return a >= b ? a : b;
}

void RgbToHsl(const unsigned char* BGRImage, const int imageSize, const int width, int* hues, float* saturations, float* lights)//todo zamiana rgb na hsl
{
	int padding = width % 4;
	for (int i = 0, pixel = 0; i < imageSize; i += PIXEL_SIZE, ++pixel)
	{
		if (i>0 && pixel % width == 0)//omijanie paddingu
		{
			if (i + padding >= imageSize)
				break;
			i += padding;
		}
		float b = BGRImage[i] / MAX_BYTE_VALUE;
		float g = BGRImage[i + 1] / MAX_BYTE_VALUE;
		float r = BGRImage[i + 2] / MAX_BYTE_VALUE;
		float min = Min(Min(r, g), b);
		float max = Max(Max(r, g), b);
		float delta = max - min;
		lights[pixel] = (max + min) / 2;
		if (delta == 0)
		{
			hues[pixel] = 0;
			saturations[pixel] = 0.0f;
		}
		else
		{
			saturations[pixel]=(lights[pixel] <= 0.5) ? (delta / (max + min)) : (delta / (2 - max - min));
			float hue;
			if (r == max)
			{
				hue = ((g - b) / 6) / delta;
			}
			else if (g == max)
			{
				hue = (1.0f / 3) + ((b - r) / 6) / delta;
			}
			else
			{
				hue = (2.0f / 3) + ((r - g) / 6) / delta;
			}
			if (hue < 0)
			{
				hue += 1;
			}
			if (hue > 1)
			{
				hue -= 1;
			}	
			hues[pixel] = (int)(hue * TWO_PI);
		}
	}
}

float HueToRGB(float v1, float v2, float H)
{
	if (H < 0)
		H += 1;

	if (H > 1)
		H -= 1;

	if ((6 * H) < 1)
		return (v1 + (v2 - v1) * 6 * H);

	if ((2 * H) < 1)
		return v2;

	if ((3 * H) < 2)
		return (v1 + (v2 - v1) * ((2.0f / 3) - H) * 6);

	return v1;
}

void HslToRgb(unsigned char* BGRImage, const int imageSize, const int width, int* hues, float* saturations, float* lights) //zamiana rgb na hsl
{
	int padding = width % 4;
	for (int i = 0, pixel = 0; i < imageSize; i += PIXEL_SIZE, ++pixel)
	{
		if (i>0 && pixel % width == 0)//omijanie paddingu
		{
			if (i + padding >= imageSize)
				break;
			i += padding;
		}
		if (saturations[pixel] > 1.0f)
		{
			saturations[pixel] = MAX_S;
		}
		else if (saturations[pixel] == 0.0f)
		{
			BGRImage[i] = BGRImage[i+1] = BGRImage[i+2] = (unsigned char)(lights[pixel] * MAX_BYTE_VALUE);
		}
		else
		{
			float v1, v2;
			float hue = (float)hues[pixel] / TWO_PI;

			v2 = (lights[pixel] < 0.5f) ? (lights[pixel] * (1 + saturations[pixel])) : ((lights[pixel] + saturations[pixel]) - (lights[pixel] * saturations[pixel]));
			v1 = 2 * lights[pixel] - v2;

			BGRImage[i+2] = (unsigned char)(MAX_BYTE_VALUE * HueToRGB(v1, v2, hue + (1.0f / 3)));
			BGRImage[i+1] = (unsigned char)(MAX_BYTE_VALUE * HueToRGB(v1, v2, hue));
			BGRImage[i] = (unsigned char)(MAX_BYTE_VALUE * HueToRGB(v1, v2, hue - (1.0f / 3)));
		}
	}
}

//FILE
unsigned char* ReadBMP(const char* filename, unsigned char info[INFO_SIZE], long int* imageSize)
{
	FILE* f = fopen(filename, "rb");

	if (f == NULL)
	{
		std::cout << "No file detected" << std::endl;
		return NULL;
	}

	fread(info, sizeof(unsigned char), INFO_SIZE, f);

	int width = *(int*)&info[WIDTH];
	int height = *(int*)&info[HEIGHT];

	std::cout << std::endl;
	std::cout << "Name: " << filename << std::endl;
	std::cout << "Width: " << width << std::endl;
	std::cout << "Height: " << height << std::endl;

	*imageSize = *(int*)&info[SIZE_IMAGE];
	unsigned char* data = new unsigned char[(*imageSize)];
	fread(data, sizeof(unsigned char), (*imageSize), f);
	fclose(f);
	return data;

}

void WriteBMP(const char* filename, const unsigned char info[INFO_SIZE], const unsigned char* image,long int imageSize)
{
	FILE* f = fopen(filename, "wb");
	if (f == NULL)
	{
		std::cout << "Couldn't open file to save data!" << std::endl;
		return;
	}
	fwrite(info, sizeof(unsigned char), INFO_SIZE, f);
	fwrite(image, sizeof(unsigned char), imageSize, f);
	fclose(f);
}

void Swap(unsigned char* v1, unsigned char* v2)
{
	unsigned char tmp = *v1;
	*v1 = *v2;
	*v2 = tmp;
}

void Calibrate(int* hslHue)
{
	int modulo = *hslHue%(RANGE*2);
	*hslHue = modulo < RANGE ? *hslHue  + RANGE - modulo : *hslHue - RANGE + modulo;
}

int main()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	DWORD maxNumberOfThreads = sysinfo.dwNumberOfProcessors;
 //logical processors
	std::vector<std::thread> threads;
	DWORD threadCounter = 0;
	int width, height;//width and height of image
	unsigned char info[INFO_SIZE];
	std::cout << "Ilosc procesorow logicznych " << maxNumberOfThreads << std::endl;
	std::string imageName;
	std::cout << "Podaj sciezke do obrazu BMP: ";
	std::cin >> imageName;
	std::string colorS[3];
	unsigned char color[3];
	int hslHue;
	float hslSaturation, hslLight;
	std::cout << "Podaj kolor w palecie barw RGB (R,G,B): ";
	std::cin.clear();
	std::cin.sync();
	std::cin >> colorS[B] >> colorS[G] >> colorS[R];
	std::cout << "Na ilu watkach zostanie wykonany algorytm?[0 && >64-optymalna ilosc]: ";
	std::cin >> threadCounter;
	if (threadCounter <= 0 || threadCounter > 64)
	{
		threadCounter = maxNumberOfThreads;
	}
	std::cout << "Ilosc uzytych watkow: " << threadCounter<<std::endl;
	color[B] = atoi(colorS[B].c_str());
	color[G] = atoi(colorS[G].c_str());
	color[R] = atoi(colorS[R].c_str());
	Swap(&(color[R]), &(color[B]));
	RgbToHsl(color, 3, 1, &hslHue, &hslSaturation, &hslLight);
	//Calibrate(&hslHue); think about it
//wczytaj plik obrazu
	long int imageSize; // size of image in bytes
	unsigned char* image = ReadBMP(imageName.c_str(), info, &imageSize);
	if (image != NULL)
	{
		width = *(int*)&info[WIDTH];
		height = *(int*)&info[HEIGHT];
		int resolution = width * height;
		int hslSizePerThread = int((resolution)/threadCounter);
		int pixelsToFix = resolution % threadCounter;
		int* imageHues = new int[resolution];
		float* imageSaturations = new float[resolution];
		float* imageLights = new float[resolution];
		RgbToHsl(image, imageSize, width, imageHues, imageSaturations, imageLights);//zamiana RGB na HSL wartości
//Start from DLL
		std::chrono::high_resolution_clock::time_point startThreadTime, endThreadTime;
		long long sumTime;
		int* tmpHues = new int[resolution];
		float* tmpSats = new float[resolution];
		//CPP
		std::cout << "Good old C++" << std::endl;
		for (int i = 0; i < resolution; ++i)
		{
			tmpHues[i] = imageHues[i];
			tmpSats[i] = imageSaturations[i];
		}
		startThreadTime = std::chrono::high_resolution_clock::now();
		for (DWORD i = 0; i < threadCounter; ++i)
		{
			threads.push_back(std::thread(std::ref(ProcedeSCPP), &(tmpHues[i*hslSizePerThread]), &(tmpSats[i*hslSizePerThread]), hslHue, hslSizePerThread));
		}
		if (pixelsToFix != 0)
		{
			threads.push_back(std::thread(std::ref(ProcedeSCPP), &(tmpHues[threadCounter*hslSizePerThread]), &(tmpSats[threadCounter*hslSizePerThread]), hslHue, pixelsToFix));
		}
		for (auto i = 0; i < threads.size(); ++i)
		{
			if (threads[i].joinable())
			{
				threads[i].join();
			}
		}
		endThreadTime = std::chrono::high_resolution_clock::now();
		sumTime = std::chrono::duration_cast<std::chrono::microseconds>(endThreadTime - startThreadTime).count();
		std::cout << "Czas wykonania algorytmu CPP[us] " << sumTime << std::endl<<std::endl;
		//ASM
		std::cout << "ASM: Be ready for domination" << std::endl;
		for (int i = 0; i < resolution; ++i)
		{
			tmpHues[i] = imageHues[i];
			tmpSats[i] = imageSaturations[i];
		}
		startThreadTime = std::chrono::high_resolution_clock::now();
		for (DWORD i = 0; i < threadCounter; ++i)
		{
			threads.push_back(std::thread(std::ref(ProcedeS), &(imageHues[i*hslSizePerThread]), &(imageSaturations[i*hslSizePerThread]), hslHue, hslSizePerThread));
		}
		if (pixelsToFix > 0)//wątek fixujący
		{
			threads.push_back(std::thread(std::ref(ProcedeS), &(imageHues[threadCounter*hslSizePerThread]), &(imageSaturations[threadCounter*hslSizePerThread]), hslHue, pixelsToFix));
		}
		for (auto i = 0; i < threads.size(); ++i)
		{
			if (threads[i].joinable())
			{
				threads[i].join();
			}
		}	
		endThreadTime = std::chrono::high_resolution_clock::now();
		sumTime = std::chrono::duration_cast<std::chrono::microseconds>(endThreadTime - startThreadTime).count();
		std::cout << "Czas wykonania algorytmu ASM[us] " << sumTime << std::endl<<std::endl;
		HslToRgb(image, imageSize, width, imageHues, imageSaturations, imageLights);
		imageName.insert(imageName.size() - 4, "-SCE");
		WriteBMP(imageName.c_str(), info, image, imageSize);
		std::cout << "Image changed and saved to \"" << imageName << "\"" << std::endl;
		delete[] image;
		delete[] imageHues;
		delete[] imageLights;
		delete[] imageSaturations;
	}
	system("pause >nul");
	return 0;
}
