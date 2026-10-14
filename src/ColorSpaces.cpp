#include "ColorSpaces.h"
#include <QDebug>

/********************************************************************************************************************************/
/* RGB processing */
/********************************************************************************************************************************/
void processing_RGB(const uchar rgbInputImg[], int x, int y, uchar rgbOutputImg[], double R, double G, double B)
{
	for(int i = 0; i< x; i++)
	{
		for(int j=0; j<y; j++)
		{
			rgbOutputImg[j*x*3+i*3] = R*rgbInputImg[j*x*3+i*3];
			rgbOutputImg[j*x*3+i*3+1] = G*rgbInputImg[j*x*3+i*3+1];
			rgbOutputImg[j*x*3+i*3+2] = B*rgbInputImg[j*x*3+i*3+2];
		}
	}
}

/********************************************************************************************************************************/
/* YUV444 processing */
/********************************************************************************************************************************/
void RGBtoYUV444(const uchar rgbImg[], int x, int y, uchar Y_buff[], char U_buff[], char V_buff[]) 
{
	uchar R, G, B;
	for(int i = 0; i<x; i++)
	{
		for(int j = 0; j<y; j+=1)
		{
			R = rgbImg[j*3*x+i*3];
			G = rgbImg[j*3*x+i*3 + 1];
			B = rgbImg[j*3*x+i*3 + 2];

			Y_buff[j*x+i] =  0.299*R + 0.587*G + 0.114*B;
			U_buff[j*x+i] =  - 0.14713*R - 0.28886*G + 0.436*B;
			V_buff[j*x+i] =  R*0.615 - 0.51499*G - 0.10001*B;
		}
	}
}

void YUV444toRGB(const uchar Y_buff[], const char U_buff[], const char V_buff[], int x, int y, uchar rgbImg[]) 
{
	double R,G,B;
	double Y, U, V;
	for(int i = 0; i<x; i++)
	{
		for(int j = 0; j<y; j+=1)
		{
			Y = Y_buff[j*x+i];
			U = U_buff[j*x+i];
			V = V_buff[j*x+i];

			R = Y + 1.13983*V;
			G = Y - 0.39465*U - 0.58060*V;
			B = Y + 2.03211*U;
			
			if (R < 0)
				R = 0;
			else if (R > 255)
				R = 255;
			if (G< 0)
				G = 0;
			else if (G> 255)
				G = 255;
			if (B < 0)
				B = 0;
			else if (B > 255)
				B = 255;


			rgbImg[j*3*x+i*3] =  R;
			rgbImg[j*3*x+i*3 + 1] = G;
			rgbImg[j*3*x+i*3 + 2] =  B;
	
		}
	}
}

void procesing_YUV444(uchar Y_buff[], char U_buff[], char V_buff[], int x, int y, double Y, double U, double V)
{
	for(int i = 0; i<x; i++)
	{
		for(int j = 0; j<y; j++)
		{
			Y_buff[j*x+i] *= Y; 
			U_buff[j*x+i] *= U; 
			V_buff[j*x+i] *= V; 
		}
	}

}

/*******************************************************************************************************************************/
/* YUV422 processing */
/********************************************************************************************************************************/
void RGBtoYUV422(const uchar rgbImg[], int x, int y, uchar Y_buff[], char U_buff[], char V_buff[]) 
{
	uchar R, G, B;
	for(int i = 0; i<x; i+=2)
	{
		for(int j = 0; j<y; j++)
		{
			R = rgbImg[j*3*x+i*3];
			G = rgbImg[j*3*x+i*3 + 1];
			B = rgbImg[j*3*x+i*3 + 2];

			Y_buff[j*x+i] =  0.299*R + 0.587*G + 0.114*B;
			U_buff[j*x/2+i/2] =  (- 0.14713*R - 0.28886*G + 0.436*B)/2;
			V_buff[j*x/2+i/2] =  (R*0.615 - 0.51499*G - 0.10001*B)/2;

			R = rgbImg[j*3*x+(i+1)*3];
			G = rgbImg[j*3*x+(i+1)*3 + 1];
			B = rgbImg[j*3*x+(i+1)*3 + 2];

			Y_buff[j*x+(i+1)] =  0.299*R + 0.587*G + 0.114*B;
			U_buff[j*x/2+i/2] +=  (- 0.14713*R - 0.28886*G + 0.436*B)/2;
			V_buff[j*x/2+i/2] +=  (R*0.615 - 0.51499*G - 0.10001*B)/2;
		}
	}
}

void YUV422toRGB(const uchar Y_buff[], const char U_buff[], const char V_buff[], int x, int y, uchar rgbImg[]) 
{
	double R,G,B;
	double Y, U, V;
	for(int i = 0; i<x; i+=2)
	{
		for(int j = 0; j<y; j++)
		{
			U = U_buff[j*x/2+i/2];
			V = V_buff[j*x/2+i/2];

			Y = Y_buff[j*x+i];

			R = Y + 1.13983*V;
			G = Y -0.39465*U - 0.58060*V;
			B = Y + 2.03211*U;

			if (R < 0)
				R = 0;
			else if (R > 255)
				R = 255;
			if (G < 0)
				G = 0;
			else if (G> 255)
				G = 255;
			if (B < 0)
				B = 0;
			else if (B > 255)
				B = 255;

			rgbImg[j*3*x+i*3] =  R;
			rgbImg[j*3*x+i*3 + 1] = G;
			rgbImg[j*3*x+i*3 + 2] = B;

			Y = Y_buff[j*x+(i+1)];

			R = Y + 1.13983*V;
			G = Y -0.39465*U - 0.58060*V;
			B = Y + 2.03211*U;

			if (R < 0)
				R = 0;
			else if (R > 255)
				R = 255;
			if (G< 0)
				G = 0;
			else if (G> 255)
				G = 255;
			if (B < 0)
				B = 0;
			else if (B > 255)
				B = 255;

			rgbImg[j*3*x+(i+1)*3] =  R;
			rgbImg[j*3*x+(i+1)*3 + 1] = G;
			rgbImg[j*3*x+(i+1)*3 + 2] = B;
		}
	}
}

void procesing_YUV422(uchar Y_buff[], char U_buff[], char V_buff[], int x, int y, double Y, double U, double V)
{
	for(int i = 0; i<x; i+=2)
	{
		for(int j = 0; j<y; j++)
		{
			Y_buff[j*x+i] *= Y; 
			Y_buff[j*x+i+1] *= Y; 
			U_buff[j*x/2+i/2] *= U; 
			V_buff[j*x/2+i/2] *= V; 
		}
	}

}

/*******************************************************************************************************************************/
/* YUV420 processing */
/*******************************************************************************************************************************/
void RGBtoYUV420(const uchar rgbImg[], int x, int y, uchar Y_buff[], char U_buff[], char V_buff[]) 
{
	uchar R, G, B;
	double U, V;

    // RGB to YUV conversion matrix
    // https://en.wikipedia.org/wiki/Y%E2%80%B2UV#Conversion_to/from_RGB
    auto getY = [&]() -> uchar {
        return qRound(0.299 * (double)R + 0.587 * (double)G + 0.114 * (double)B);
    };
    auto getU = [&]() -> double {
        return -0.14713 * (double)R - 0.28886 * (double)G + 0.436 * (double)B;
    };
    auto getV = [&]() -> double {
        return 0.615 * (double)R - 0.51499 * (double)G - 0.10001 * (double)B;
    };

    for(int i = 0; i < x; i += 2)
    {
        for(int j = 0; j < y; j += 2)
        {
            /*
             * Perform 4:2:0 chroma subsampling.
             * The human eye is much more sensitive to pure brightness (Y)
             * compared to color information (U, V), so they can be
             * subsampled/averaged over a set of pixels in order to save space.
             */
            U = 0;
            V = 0;
            // Sample through 2x2 pixel blocks.
            for(int n = 0; n < 2; n++)
            {
                for(int m = 0; m < 2; m++)
                {
                    int k = j + m;
                    int l = i + n;
                    int ind = (3 * k) * x + (3 * l);

                    R = rgbImg[ind];
                    G = rgbImg[ind + 1];
                    B = rgbImg[ind + 2];
                    // Keep the luminance for every pixel.
                    Y_buff[k * x + l] = getY();
                    // Add up U and V values which will be averaged
                    // over the entire 2x2 block.
                    U += getU();
                    V += getV();
                }
            }

            // Average out the summed values.
            U_buff[(j / 2) * (x / 2) + (i / 2)] =  qRound(U / 4.0);
            V_buff[(j / 2) * (x / 2) + (i / 2)] =  qRound(V / 4.0);
        }
    }
}

void YUV420toRGB(const uchar Y_buff[], const char U_buff[], const char V_buff[], int x, int y, uchar rgbImg[]) 
{
	double R,G,B;
	double Y, U, V;
    // TO DO

}

void procesing_YUV420(uchar Y_buff[], char U_buff[], char V_buff[], int x, int y, double Y, double U, double V)
{
	// TO DO

}

/*******************************************************************************************************************************/
/* Y decimation */
/*******************************************************************************************************************************/
void decimate_Y(uchar Y_buff[], int x, int y)
{
	uchar YY;
	// TO DO

}
