#include "filters.h"
#include <iostream>
#include <windows.h>
#include <math.h>

using namespace std;

static void EdgeDetectionWithoutNonmaximum(const LPCTSTR , const LPCTSTR);
static void NonmaximumWithoutDoubleThresholding(const LPCTSTR , const LPCTSTR );
static void CannyEdgeDetection(const LPCTSTR , const LPCTSTR );
static void HoughTransform(const LPCTSTR , const LPCTSTR );

int _tmain(int argc, _TCHAR* argv[])
{
	LPCTSTR input, output;
	input = L"./input.bmp";
	output = L"./output.bmp";
	HoughTransform(input, output);


	system("PAUSE");

	return 0;
}

static void EdgeDetectionWithoutNonmaximum(const LPCTSTR input, const LPCTSTR output){
	int width, height;
	long Size, new_size;
	/* EDGE DETECTION WITHOUT NONMAXIMUM*/
	
	BYTE* buffer = LoadBMP(&width, &height, &Size, input);
	BYTE* raw_intensity = ConvertBMPToIntensity(buffer, width, height);
	delete[] buffer;
	float * angles = new float[width*height];
	for (int i = 0; i < width*height; i++) angles[i] = 0;

	BYTE* edge = EdgeDetection(raw_intensity, width, height, angles);
	delete[] raw_intensity;
	char ch;
	cout << "Sonucu diske kaydetsin mi? E/H:"; cin >> ch;

	if ((ch == 'E') || (ch == 'e')) {
		BYTE* display_imge = ConvertIntensityToBMP(edge, width, height, &new_size);
		if (SaveBMP(display_imge, width, height, Size, output))
			cout << " Output Image was successfully saved" << endl;
		else cout << "Error on saving image" << endl;
		delete[] display_imge;
	}
	delete[] edge;
	delete[] angles;
}

static void NonmaximumWithoutDoubleThresholding(const LPCTSTR input, const LPCTSTR output){
	int width, height;
	long Size, new_size;
	/*NONMAXIMUM WIHOUT THRESHOLDING*/
	
	BYTE* buffer = LoadBMP(&width, &height, &Size, input);
	BYTE* raw_intensity = ConvertBMPToIntensity(buffer, width, height);
	delete[] buffer;
	float * angles = new float[width*height];
	for (int i = 0; i < width*height; i++) angles[i] = 0;
	BYTE* edge = EdgeDetection(raw_intensity, width, height, angles);
	delete[] raw_intensity;
	BYTE * nonmax = NonMaximum(edge, angles, width, height);
	delete[] edge;
	delete[] angles;
	char ch;
	cout << "Sonucu diske kaydetsin mi? E/H:"; cin >> ch;

	if ((ch == 'E') || (ch == 'e')) {
		BYTE* display_imge = ConvertIntensityToBMP(nonmax, width, height, &new_size);
		if (SaveBMP(display_imge, width, height, Size, output))
			cout << " Output Image was successfully saved" << endl;
		else cout << "Error on saving image" << endl;
		delete[] display_imge;
	}
	delete[] nonmax;
	delete[] angles;
}

static void CannyEdgeDetection(const LPCTSTR input, const LPCTSTR output){
	
	int width, height;
	long Size, new_size;

	/*THRESHOLDING WITH CANNY EDGE DETECTION*/
	BYTE* buffer = LoadBMP(&width, &height, &Size, input);
	BYTE* raw_intensity = ConvertBMPToIntensity(buffer, width, height);
	delete[] buffer;
	float * angles = new float[width*height];
	for (int i = 0; i < width*height; i++)
		angles[i] = 0;
	BYTE* edge = EdgeDetection(raw_intensity, width, height, angles);
	delete[] raw_intensity;
	BYTE * nonmax = NonMaximum(edge, angles, width, height);
	delete[] edge;
	DoubleThresholding(nonmax, width, height, angles, 20, 80);
	delete[] angles;
	char ch;
	cout << "Sonucu diske kaydetsin mi? E/H:"; cin >> ch;

	if ((ch == 'E') || (ch == 'e')) {
		BYTE* display_imge = ConvertIntensityToBMP(nonmax, width, height, &new_size);
		if (SaveBMP(display_imge, width, height, Size, output))
			cout << " Output Image was successfully saved" << endl;
		else cout << "Error on saving image" << endl;
		delete[] display_imge;
	}
	delete[] nonmax;
}

static void HoughTransform(const LPCTSTR input, const LPCTSTR output){
	
	int width, height;
	long Size, new_size;
	/*	HOUGH TRANSFORM	*/
	
	BYTE* buffer = LoadBMP(&width, &height, &Size, input);
	BYTE* raw_intensity = ConvertBMPToIntensity(buffer, width, height);
	delete[] buffer;
	float * angles = new float[width*height];
	for (int i = 0; i < width*height; i++)
		angles[i] = 0;
	BYTE* edge = EdgeDetection(raw_intensity, width, height, angles);
	
	BYTE * nonmax = NonMaximum(edge, angles, width, height);
	delete[] edge;
	DoubleThresholding(nonmax, width, height, angles, 20, 80);
	BYTE *hough = HoughTransformLine(nonmax, width, height, 25);
	delete[] nonmax;
	bool* coloredLines = Colorization(hough, width, height);
	delete[] hough;
	char ch;
	cout << "Sonucu diske kaydetsin mi? E/H:"; cin >> ch;

	if ((ch == 'E') || (ch == 'e')) {
		BYTE* result = ConvertIntensityToBMP2(raw_intensity, coloredLines, width, height, &new_size);
		if (SaveBMP(result, width, height, Size, output))
			cout << " Output Image was successfully saved" << endl;
		else cout << "Error on saving image" << endl;
		delete[] result;
	}
	delete[]raw_intensity;
	delete[]coloredLines;
	delete[] angles;

}