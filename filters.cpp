#include <windows.h>
#include <math.h>

#define PI 3.14159265

BYTE* LoadBMP(int* width, int* height, long* size, LPCTSTR bmpfile)
{
	// declare bitmap structures
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfo;
	// value to be used in ReadFile funcs
	DWORD bytesread;
	// open file to read from
	HANDLE file = CreateFile(bmpfile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (NULL == file)
		return NULL; // coudn't open file

	// read file header
	if (ReadFile(file, &bmpheader, sizeof(BITMAPFILEHEADER), &bytesread, NULL) == false)
	{
		CloseHandle(file);
		return NULL;
	}

	//read bitmap info

	if (ReadFile(file, &bmpinfo, sizeof(BITMAPINFOHEADER), &bytesread, NULL) == false)
	{
		CloseHandle(file);
		return NULL;
	}

	// check if file is actually a bmp
	if (bmpheader.bfType != 'MB')
	{
		CloseHandle(file);
		return NULL;
	}

	// get image measurements
	*width = bmpinfo.biWidth;
	*height = abs(bmpinfo.biHeight);

	// check if bmp is uncompressed
	if (bmpinfo.biCompression != BI_RGB)
	{
		CloseHandle(file);
		return NULL;
	}

	// check if we have 24 bit bmp
	if (bmpinfo.biBitCount != 24)
	{
		CloseHandle(file);
		return NULL;
	}


	// create buffer to hold the data
	*size = bmpheader.bfSize - bmpheader.bfOffBits;
	BYTE* Buffer = new BYTE[*size];
	// move file pointer to start of bitmap data
	SetFilePointer(file, bmpheader.bfOffBits, NULL, FILE_BEGIN);
	// read bmp data
	if (ReadFile(file, Buffer, *size, &bytesread, NULL) == false)
	{
		delete[] Buffer;
		CloseHandle(file);
		return NULL;
	}

	// everything successful here: close file and return buffer

	CloseHandle(file);

	return Buffer;
}

bool SaveBMP(BYTE* Buffer, int width, int height, long paddedsize, LPCTSTR bmpfile)
{
	// declare bmp structures 
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER info;

	// andinitialize them to zero
	memset(&bmfh, 0, sizeof(BITMAPFILEHEADER));
	memset(&info, 0, sizeof(BITMAPINFOHEADER));

	// fill the fileheader with data
	bmfh.bfType = 0x4d42;       // 0x4d42 = 'BM'
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + paddedsize;
	bmfh.bfOffBits = 0x36;		// number of bytes to start of bitmap bits

	// fill the infoheader

	info.biSize = sizeof(BITMAPINFOHEADER);
	info.biWidth = width;
	info.biHeight = height;
	info.biPlanes = 1;			// we only have one bitplane
	info.biBitCount = 24;		// RGB mode is 24 bits
	info.biCompression = BI_RGB;
	info.biSizeImage = 0;		// can be 0 for 24 bit images
	info.biXPelsPerMeter = 0x0ec4;     // paint and PSP use this values
	info.biYPelsPerMeter = 0x0ec4;
	info.biClrUsed = 0;			// we are in RGB mode and have no palette
	info.biClrImportant = 0;    // all colors are important

	// now we open the file to write to
	HANDLE file = CreateFile(bmpfile, GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == NULL)
	{
		CloseHandle(file);
		return false;
	}

	// write file header
	unsigned long bwritten;
	if (WriteFile(file, &bmfh, sizeof(BITMAPFILEHEADER), &bwritten, NULL) == false)
	{
		CloseHandle(file);
		return false;
	}
	// write infoheader
	if (WriteFile(file, &info, sizeof(BITMAPINFOHEADER), &bwritten, NULL) == false)
	{
		CloseHandle(file);
		return false;
	}
	// write image data
	if (WriteFile(file, Buffer, paddedsize, &bwritten, NULL) == false)
	{
		CloseHandle(file);
		return false;
	}

	// and close file
	CloseHandle(file);

	return true;
}

BYTE* ConvertBMPToIntensity(const BYTE* Buffer, int width, int height)
{
	// first make sure the parameters are valid
	if ((NULL == Buffer) || (width == 0) || (height == 0))
		return NULL;

	// find the number of padding bytes

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
		padding++;
	// get the padded scanline width
	int psw = scanlinebytes + padding;

	// create new buffer
	BYTE* newbuf = new BYTE[width*height];

	// now we loop trough all bytes of the original buffer, 
	// swap the R and B bytes and the scanlines
	long bufpos = 0;
	long newpos = 0;
	for (int row = 0; row < height; row++)
		for (int column = 0; column < width; column++)
		{
		newpos = row * width + column;
		bufpos = (height - row - 1) * psw + column * 3;
		//newbuf[newpos] = BYTE(0.11*Buffer[bufpos + 2] + 0.59*Buffer[bufpos + 1] + 0.3*Buffer[bufpos]);
		newbuf[newpos] = BYTE(0.33*Buffer[bufpos + 2] + 0.33*Buffer[bufpos + 1] + 0.33*Buffer[bufpos]);
		//Buffer[bufpos + 2] +=5;
		//Buffer[bufpos + 1] +=5;
		//Buffer[bufpos] +=5;
		}

	return newbuf;
}

BYTE* ConvertIntensityToBMP2(const BYTE* Buffer, bool* coloredBuffer, int width, int height, long* newsize)
{
	// first make sure the parameters are valid
	if ((NULL == Buffer) || (width == 0) || (height == 0))
		return NULL;

	// now we have to find with how many bytes
	// we have to pad for the next DWORD boundary	

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
		padding++;
	// get the padded scanline width
	int psw = scanlinebytes + padding;
	// we can already store the size of the new padded buffer
	*newsize = height * psw;

	// and create new buffer
	BYTE* newbuf = new BYTE[*newsize];

	// fill the buffer with zero bytes then we dont have to add
	// extra padding zero bytes later on
	memset(newbuf, 0, *newsize);

	// now we loop trough all bytes of the original buffer, 
	// swap the R and B bytes and the scanlines
	long bufpos = 0;
	long newpos = 0;
	for (int row = 0; row < height; row++)
		for (int column = 0; column < width; column++)
		{
		bufpos = row * width + column;     // position in original buffer
		newpos = (height - row - 1) * psw + column * 3;           // position in padded buffer
		if (coloredBuffer[bufpos] == true){
			newbuf[newpos] = 0;      //  blue
			newbuf[newpos + 1] = 255;   //  green
			newbuf[newpos + 2] = 0;

		}
		else
		{
			newbuf[newpos] = Buffer[bufpos];       //  blue
			newbuf[newpos + 1] = Buffer[bufpos];   //  green
			newbuf[newpos + 2] = Buffer[bufpos];   //  red

		}
		}
	return newbuf;
}

BYTE* ConvertIntensityToBMP(const BYTE* Buffer, int width, int height, long* newsize)
{
	// first make sure the parameters are valid
	if ((NULL == Buffer) || (width == 0) || (height == 0))
		return NULL;

	// now we have to find with how many bytes
	// we have to pad for the next DWORD boundary	

	int padding = 0;
	int scanlinebytes = width * 3;
	while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
		padding++;
	// get the padded scanline width
	int psw = scanlinebytes + padding;
	// we can already store the size of the new padded buffer
	*newsize = height * psw;

	// and create new buffer
	BYTE* newbuf = new BYTE[*newsize];

	// fill the buffer with zero bytes then we dont have to add
	// extra padding zero bytes later on
	memset(newbuf, 0, *newsize);

	// now we loop trough all bytes of the original buffer, 
	// swap the R and B bytes and the scanlines
	long bufpos = 0;
	long newpos = 0;
	for (int row = 0; row < height; row++)
		for (int column = 0; column < width; column++)
		{
		bufpos = row * width + column;     // position in original buffer
		newpos = (height - row - 1) * psw + column * 3;           // position in padded buffer

		newbuf[newpos] = Buffer[bufpos];       //  blue
		newbuf[newpos + 1] = Buffer[bufpos];   //  green
		newbuf[newpos + 2] = Buffer[bufpos];   //  red
		}

	return newbuf;
}

BYTE * EdgeDetection(const BYTE* intensityBuffer, const int width, const int height, float * angles)
{
	int * edge_ver = new int[width*height];
	int * edge_hor = new int[width*height];
	int * sum = new int[width*height];
	int size = width*height;

	BYTE * edgeimage = new BYTE[width*height];
	memset(edge_ver, 0, width*height);
	memset(edge_hor, 0, width*height);
	memset(edgeimage, 0, width*height);
	memset(sum, 0, width*height);
	
	int biggest = 255;
	int virtualsize = size - width;
	for (int i = width + 1; i < virtualsize; i += width)
	{
		for (int j = i; j < i + width - 1; j++)
		{
			edge_hor[j] = intensityBuffer[j + width - 1] + 2 * intensityBuffer[j + width] + intensityBuffer[j + width + 1]
				- intensityBuffer[j - width - 1] - 2 * intensityBuffer[j - width] - intensityBuffer[j - width + 1];
			edge_ver[j] = -intensityBuffer[j + width - 1] + intensityBuffer[j + width + 1] - 2 * intensityBuffer[j - 1] + 2 * intensityBuffer[j + 1]
				- intensityBuffer[j + width - 1] + intensityBuffer[j + width + 1];
			if (edge_ver[j] != 0)
				angles[j] = atan2((float)edge_hor[j], edge_ver[j]) * 180 / PI;
			else
				angles[j] = atan2((float)edge_hor[j], 1) * 180 / PI;
			if (angles[j] < 0)
				angles[j] = (360 + angles[j]);
			//sum[j] = abs(edge_hor[j]) + abs(edge_ver[j]); // Another method
			sum[j] = sqrt(pow((double)edge_hor[j], 2) + pow((double)edge_ver[j], 2));
			if (sum[j]>biggest)  biggest = sum[j];
		}
	}

	delete[] edge_hor;
	delete[] edge_ver;

	for (int j = 0; j < width*height; j++)
		edgeimage[j] = (sum[j] * 255) / biggest;
	
	delete[] sum;
	return edgeimage;
}

BYTE * NonMaximum(const BYTE* edgeBuffer, const float *angles, const int width, const int height){

	BYTE * tempbuffer = new BYTE[width*height];
	memset(tempbuffer, 0, width*height);
	int virtualsize = width*(height - 1);

	for (int i = width + 1; i < virtualsize; i += width)
	{
		for (int j = i; j < i + width - 1; j++)
		{
			if ((angles[j] > 337.5 && angles[j] <= 360.0) || (angles[j] >= 0.0 && angles[j] <= 22.5) || (angles[j]>157.5 && angles[j]<202.5))
			{
				if (edgeBuffer[j - 1] > edgeBuffer[j] || edgeBuffer[j + 1] > edgeBuffer[j])
				{
					tempbuffer[j - 1] = edgeBuffer[j - 1];
					tempbuffer[j] = 0;
					tempbuffer[j + 1] = edgeBuffer[j + 1];
				}
				else
				{
					tempbuffer[j - 1] = 0; tempbuffer[j + 1] = 0; tempbuffer[j] = edgeBuffer[j];
				}

			}
			else if ((angles[j]>22.5 && angles[j]<67.5) || (angles[j] >= 202.5 && angles[j]<247.5))
			{
				if (edgeBuffer[j - width + 1] > edgeBuffer[j] || edgeBuffer[j + width - 1] > edgeBuffer[j])
				{
					tempbuffer[j - width + 1] = edgeBuffer[j - width + 1];
					tempbuffer[j] = 0;
					tempbuffer[j + width - 1] = edgeBuffer[j + width - 1];
				}
				else
				{
					tempbuffer[j - width + 1] = 0; tempbuffer[j + width - 1] = 0; tempbuffer[j] = edgeBuffer[j];
				}
			}
			else if ((angles[j] >= 67.5 && angles[j]<112.5) || (angles[j] >= 247.5 && angles[j]<292.5))
			{
				if (edgeBuffer[j - width] > edgeBuffer[j] || edgeBuffer[j + width] > edgeBuffer[j])
				{
					tempbuffer[j - width] = edgeBuffer[j - width];
					tempbuffer[j] = 0;
					tempbuffer[j + width] = edgeBuffer[j + width];
				}
				else
				{
					tempbuffer[j - width] = 0; tempbuffer[j + width] = 0; tempbuffer[j] = edgeBuffer[j];
				}
			}
			else if ((angles[j] >= 112.5 && angles[j] <= 157.5) || (angles[j] >= 292.5 && angles[j] <= 337.5))
			{
				if (edgeBuffer[j - 1 - width] > edgeBuffer[j] || edgeBuffer[j + width + 1] > edgeBuffer[j])
				{
					tempbuffer[j - 1 - width] = edgeBuffer[j - 1 - width];
					tempbuffer[j] = 0;
					tempbuffer[j + 1 + width] = edgeBuffer[j + 1 + width];
				}
				else
				{
					tempbuffer[j - 1 - width] = 0; tempbuffer[j + 1 + width] = 0; tempbuffer[j] = edgeBuffer[j];
				}
			}
		}
	}

	return tempbuffer;
}

static BYTE decision(BOOL ch1, BOOL ch2, BYTE a, BYTE b){
	if ((ch1 == TRUE) || (ch2 == TRUE)){
		if ((a == 255) || (ch2 == 255)){
			if (a == 255)
				return a;
			else
				return b;
		}
		else
			return 0;
	}
	else
		return 30;
}

void DoubleThresholding(BYTE * Buffer, const int width, const int height, float * angles, int orank, int oranb){

	UINT32 * th = new UINT32[256];
	for (int i = 0; i < 256; i++){
		th[i] = 0;
	}
	for (int k = 0; k < width*height; k++){
		th[Buffer[k]]++;
	}

	double * thresValues = new double[256];
	double wb = 0;
	double mbu = 0;
	double wf = 1.0;
	double mfu = 0;
	for (int i = 0; i < 256; i++){
		mfu += i*th[i];
	}
	for (int i = 0; i < 256; i++){
		double mb = 0;
		double mf = 0;

		wb += (double)th[i] / (width*height);
		mbu += (i*(double)th[i]);
		mb = mbu / wb*width*height;

		wf -= (double)th[i] / (width*height);
		mfu -= i*(double)th[i];
		mf = mfu / wf*width*height;

		thresValues[i] = (double)(wb*wf)*(mb - mf)*(mb - mf);
	}

	int index_b = 0;
	for (int i = 0; i <= 255; i++){
		if (thresValues[i] != 0){
			index_b = i;
		}
	}

	int index_k = 0;
	for (int i = 0; i <= 255; i++){
		if (thresValues[i] != 0){
			index_k = i;
			break;
		}
	}

	int aralýk = index_b - index_k;

	index_k += aralýk * orank / 100;
	index_b -= aralýk * oranb/ 100;

	delete[]thresValues;
	delete []th;
	
	int size = width*height;
	int virtualsize = size - width;
	BOOL *change = new BOOL[width*height];
	for (int i = 0; i < width*height; i++)
	{
		change[i] = FALSE;
	}
	int count = 2 * width + 2 * (height - 2);

	for (int i = 0; i < width*height; i++){
		if (angles[i] >= 270){
			angles[i] -= 270;
		}
		else{
			angles[i] += 90;
		}
	}

	do{
		for (int i = width + 1; i < virtualsize; i += width){
			for (int j = i; j < i + width - 1; j++){
				if (change[j] == FALSE){
					if (Buffer[j] <= index_k){
						count++;
						Buffer[j] = 0;
						change[j] = TRUE;
					}else if (Buffer[j] >= index_b){
						count++;
						Buffer[j] = 255;
						change[j] = TRUE;
					}
					else{
						if ((angles[j] > 337.5 && angles[j] <= 360.0) || (angles[j] >= 0.0 && angles[j] <= 22.5) || (angles[j] > 157.5 && angles[j] < 202.5)){
							BYTE X = decision(change[j - 1], change[j + 1], Buffer[j - 1], Buffer[j + 1]);
							if (X != 30){
								Buffer[j] = X;
								count++;
								change[j] = TRUE;
							}
						}else if ((angles[j] > 22.5 && angles[j] < 67.5) || (angles[j] >= 202.5 && angles[j] < 247.5)){
							BYTE X = decision(change[j - width + 1], change[j + width - 1], Buffer[j - width + 1], Buffer[j + width - 1]);
							if (X != 30){
								Buffer[j] = X;
								count++;
								change[j] = TRUE;
							}
						}
						else if ((angles[j] >= 67.5 && angles[j] < 112.5) || (angles[j] >= 247.5 && angles[j] < 292.5))
						{
							BYTE X = decision(change[j - width], change[j + width], Buffer[j - width], Buffer[j + width]);
							if (X != 30){
								Buffer[j] = X;
								count++;
								change[j] = TRUE;
							}
						}
						else if ((angles[j] >= 112.5 && angles[j] <= 157.5) || (angles[j] >= 292.5 && angles[j] <= 337.5))
						{
							BYTE X = decision(change[j - 1 - width], change[j + width + 1], Buffer[j - 1 - width], Buffer[j + width + 1]);
							if (X != 30){
								Buffer[j] = X;
								count++;
								change[j] = TRUE;
							}

						}
					}
				}
			}
		}
	} while (count < size - 1);
	delete[] change;
};

BYTE * HoughTransformLine(const BYTE *EdgeBinary, const int width, const int height, int peakCount){

	BYTE * temps = new BYTE[width*height];
	memset(temps, 0, width*height);

	float MaxDistance1 = sqrt(pow((float)width, 2) + pow((float)height, 2));
	int MaxDistance = MaxDistance1;

	int **accumulator = new int*[2 * MaxDistance + 1];
	for (int i = 0; i <= (2 * MaxDistance); i++){
		accumulator[i] = new int[181];
	}
	for (int i = 0; i <= 2 * MaxDistance; i++){
		for (int j = 0; j <= 180; j++){
			accumulator[i][j] = 0;
		}
	}

	for (int i = 0; i <width*height; i++){
		if (EdgeBinary[i] == 255){
			for (int j = -90; j <= 90; j++){
				float d = (floor((float)i / width))*cos((float)j*PI / 180) + (i % width)*sin((float)j*PI / 180);
				int tempd;
				d += MaxDistance1;
				if (d - (floor(d)) <= 0, 5)
					tempd = floor(d);
				else
					tempd = ceil(d);
				int tempj = (j)+90;
				accumulator[tempd][tempj]++;
			}
		}
	}
	// TODO: We need "the peak find algorithm" in here
	const int len = peakCount * 2;
	int *max_i=new int[len];
	memset(max_i, 0, len);
	int temp = 0;
	for (int a = 0; a < len; a += 2){
		for (int i = 0; i <= 2 * MaxDistance; i++){
			for (int j = 0; j <= 180; j++){
				if (temp <= accumulator[i][j]){
					temp = accumulator[i][j];
					max_i[a] = i;
					max_i[a + 1] = j;
				}
			}
		}
		accumulator[max_i[a]][max_i[a + 1]] = 0;
		temp = 0;
	}
	//*//////
	BYTE *x = new BYTE[((2 * MaxDistance) + 1) * 181];
	for (int i = 0; i <= 2 * MaxDistance; i++){
		for (int j = 0; j <= 180; j++){
			x[i * 181 + j] = accumulator[i][j] + 30;
		}
	}

	/*long newsize;
	BYTE* y = ConvertIntensityToBMP(x, 181, ((2 * MaxDistance) + 1), &newsize);
	SaveBMP(y, 181, ((2 * MaxDistance) + 1), newsize, L"C:/Users/Enes/Desktop/CannyEdge/Hough3Dgrafik.bmp");*/

	for (int i = 0; i < len; i += 2)
	{
		max_i[i] -= MaxDistance;
		max_i[i + 1] -= 90;
	}

	for (int j = 0; j < len; j += 2){
		for (int i = 0; i < width*height; i++){
			int  a = ((floor((float)i / width))*cos(((float)max_i[j + 1])*PI / 180) + (i%width)*sin(((float)max_i[j + 1])*PI / 180));
			if (max_i[j] == a){
				temps[i] = 255;
			}
		}
	}
	return temps;
}

bool* Colorization(const BYTE* Hough_buffer, const int width, const int height)
{
	bool* colorbuffer = new bool[width*height];
	for (int i = 0; i < width*height; i++)
	{
		if (Hough_buffer[i] == 255)
		{
			colorbuffer[i] = true;
		}
		else{
			colorbuffer[i] = false;
		}
	}

	return colorbuffer;
}