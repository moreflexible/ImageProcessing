#include <windows.h>

BYTE*  LoadBMP(int* width, int* height, long* size, LPCTSTR bmpfile);
bool   SaveBMP(BYTE* Buffer, int width, int height, long paddedsize, LPCTSTR bmpfile);
BYTE*  ConvertBMPToIntensity(const BYTE* Buffer, int width, int height);
BYTE*  ConvertIntensityToBMP(const BYTE* Buffer, int width, int height, long* newsize);
BYTE*  EdgeDetection(const BYTE* intensityBuffer, const int width, const int height, float *);
BYTE*  NonMaximum(const BYTE* edgeBuffer, const float *angles, const int width, const int height);
void  DoubleThresholding(BYTE * Buffer, const int width, const int height, float * angles, int, int);
BYTE*  HoughTransformLine(const BYTE *EdgeBinary, const int width, const int height, int peakCount);
BYTE*  ConvertIntensityToBMP2(const BYTE* Buffer, bool* coloredBuffer, int width, int height, long* newsize);
bool*  Colorization(const BYTE* Hough_buffer, const int width, const int height);