#include "JPEG.h"
#include "NxNDCT.h"
#include <math.h>

#include "JPEGBitStreamWriter.h"


#define DEBUG(x) do{ qDebug() << #x << " = " << x;}while(0)



// quantization tables from JPEG Standard, Annex K
uint8_t QuantLuminance[8*8] =
    { 16, 11, 10, 16, 24, 40, 51, 61,
      12, 12, 14, 19, 26, 58, 60, 55,
      14, 13, 16, 24, 40, 57, 69, 56,
      14, 17, 22, 29, 51, 87, 80, 62,
      18, 22, 37, 56, 68,109,103, 77,
      24, 35, 55, 64, 81,104,113, 92,
      49, 64, 78, 87,103,121,120,101,
      72, 92, 95, 98,112,100,103, 99 };
uint8_t QuantChrominance[8*8] =
    { 17, 18, 24, 47, 99, 99, 99, 99,
      18, 21, 26, 66, 99, 99, 99, 99,
      24, 26, 56, 99, 99, 99, 99, 99,
      47, 66, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99 };

static char quantizationMatrix[64] =
{
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

// Precalculated zig-zag order for the quantization tables
const int zigzagOrder[8][8] =
{
    { 0,  1,  5,  6, 14, 15, 27, 28},
    { 2,  4,  7, 13, 16, 26, 29, 42},
    { 3,  8, 12, 17, 25, 30, 41, 43},
    { 9, 11, 18, 24, 31, 40, 44, 53},
    {10, 19, 23, 32, 39, 45, 52, 54},
    {20, 22, 33, 38, 46, 51, 55, 60},
    {21, 34, 37, 47, 50, 56, 59, 61},
    {35, 36, 48, 49, 57, 58, 62, 63}
};

struct imageProperties{
    int width;
    int height;
    int16_t* coeffs;
};


void DCTUandV(const char input[], int16_t output[], int N, double* DCTKernel)
{
    double* temp = new double[N*N];
    double* DCTCoefficients = new double[N*N];

    double sum;
    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + DCTKernel[i*N+k] * (input[k*N+j]);
            }
            temp[i*N + j] = sum;
        }
    }

    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + temp[i*N+k] * DCTKernel[j*N+k];
            }
            DCTCoefficients[i*N+j] = sum;
        }
    }

    for(int i = 0; i < N*N; i++)
    {
        output[i] = floor(DCTCoefficients[i]+0.5);
    }

    delete[] temp;
    delete[] DCTCoefficients;

    return;
}

uint8_t quantQuality(uint8_t quant, uint8_t quality) {
    // Convert to an internal JPEG quality factor, formula taken from libjpeg
    // Preemptively clamp values as implemented in libjpeg
    // https://github.com/winlibs/libjpeg/blob/1177955a8eecc6cdc61a39e59081e410d9054f66/jcparam.c#L132
    if(quality <= 0) quality = 1;
    if(quality > 100) quality = 100;
    int16_t q = quality < 50 ? 5000 / quality : 200 - quality * 2;
    return clamp((quant * q + 50) / 100, 1, 255);
}

// Use templates to avoid overloading functions and to avoid
// headaches with differences between uint8_t and int16_t.
template<typename T> void doZigZag(T block[], int N)
{
    T* result = new T[N*N];

    for(int j = 0; j < N; j++)
    {
        for(int i = 0; i < N; i++)
        {
            result[zigzagOrder[j][i]] = block[j * N + i];
        }
    }

    for(int i = 0; i < N*N; i++)
    {
        block[i] = result[i];
    }
    delete[] result;
}

/* perform DCT */
imageProperties performDCT(char input[], int xSize, int ySize, int N, uint8_t* quantTable)
{
    double* DCTKernel = new double[N * N];
    GenerateDCTmatrix(DCTKernel, N); // Generate DCT kernel.

    // Output DCT coefficients.
    imageProperties result;
    result.width = xSize;
    result.height = ySize;
    result.coeffs = new int16_t[xSize * ySize];

    // Helper input block.
    char* in = new char[N * N];
    // Helper output block.
    int16_t *blockCoeffs = new int16_t[N*N];

    for(int y = 0; y < ySize / N; y++)
    {
        for(int x = 0; x < xSize / N; x++)
        {
            // Write to input block.
            for(int j = 0; j < N; j++)
            {
                for(int i = 0; i < N; i++)
                {
                    in[j * N + i] = input[(y * N + j) * xSize + (x * N + i)];
                }
            }

            // Perform DCT on the input block.
            DCTUandV(in, blockCoeffs, N, DCTKernel);

            // Quantize the transformed block.
            for(int j = 0; j < N; j++)
            {
                for(int i = 0; i < N; i++)
                {
                    /*
                     * As blockCoeffs and quantTable are whole integer types (int16_t and uint8_t respectively),
                     * the resulting division would always floor the result. The JPEG standard recommends
                     * using a proper rounding function for the result.
                     * https://www.w3.org/Graphics/JPEG/itu-t81.pdf (Page 28)
                     *
                     * Empirically (based on the small sample size of demo images) the difference seems to be
                     * unnoticeable, if it exists at all.
                     */

                    blockCoeffs[j * N + i] = qRound(blockCoeffs[j * N + i] / (double)quantTable[j * N + i]);
                }
            }

            // Perform zigzagging to optimize the compression rate for the lossless part.
            doZigZag(blockCoeffs, N);

            // Write back to the resulting buffer.
            for(int j = 0; j < N; j++)
            {
                for(int i = 0; i < N; i++)
                {
                    result.coeffs[(y * N + j) * xSize + (x * N + i)] = blockCoeffs[j * N + i];
                }
            }
        }
    }
    delete[] DCTKernel;
    delete[] in;
    delete[] blockCoeffs;
    return result;
}

//JPEGBitStreamWriter streamer("example.jpg");
void performJPEGEncoding(uchar Y_buff[], char U_buff[], char V_buff[], int xSize, int ySize, int quality)
{
	DEBUG(quality);
    const int N = 8;
    const int dN = 2*N;
    /*
     * The function should get the untouched dimensions of the input image
     * so that they can be written in the JPEG header. That way, the final
     * JPEG image will have the intended image size.
     */
    int xSizeNorm = xSize;
    int ySizeNorm = ySize;
    if(xSizeNorm % dN != 0) xSizeNorm = dN * (xSizeNorm / dN) + dN;
    if(ySizeNorm % dN != 0) ySizeNorm = dN * (ySizeNorm / dN) + dN;

    auto s = new JPEGBitStreamWriter("example.jpg");

    // Perform level shifting of the Y channel.
    char* Y_norm = new char[xSizeNorm * ySizeNorm];
    for(int i = 0; i < xSizeNorm * ySizeNorm; i++)
    {
        Y_norm[i] = Y_buff[i] - 128;
    }

    // Prepare quantization tables based on the quality factor.
    uint8_t* chroma = new uint8_t[N * N];
    uint8_t* luma = new uint8_t[N * N];
    for(int i = 0; i < N * N; i++)
    {
        luma[i] = quantQuality(QuantLuminance[i], quality);
        chroma[i] = quantQuality(QuantChrominance[i], quality);
    }

    // Perform DCT for every channel buffer.
    auto y_dct = performDCT(Y_norm, xSizeNorm, ySizeNorm, N, luma);
    auto u_dct = performDCT(U_buff, xSizeNorm/2, ySizeNorm/2, N, chroma);
    auto v_dct = performDCT(V_buff, xSizeNorm/2, ySizeNorm/2, N, chroma);

    // Quantization tables also need to be in zig-zag order for proper decoding.
    doZigZag(luma, N);
    doZigZag(chroma, N);

    // Write header and other metadata to the file.
    s->writeHeader();
    s->writeQuantizationTables(luma, chroma);
    s->writeImageInfo(xSize, ySize); // Write proper, non-expanded image size.
    s->writeHuffmanTables();

    int16_t* in  = new int16_t[dN * dN];
    int16_t* inY = new int16_t[N * N];
    int16_t* inU = new int16_t[N * N];
    int16_t* inV = new int16_t[N * N];

    // Iterate through every block.
    for(int y = 0; y < ySizeNorm / dN; y++)
    {
        for(int x = 0; x < xSizeNorm / dN; x++)
        {
            // Iterate through every pixel in the block, write to input block from the Y channel.
            for (int j = 0; j < dN; j++)
            {
                for (int i = 0; i < dN; i++)
                {
                    int in_index = j * dN + i;
                    int y_ind = y * dN + j;
                    int x_ind = x * dN + i;
                    in[in_index] = y_dct.coeffs[y_ind * xSizeNorm + x_ind];
                }
            }
            // Iterate through the 8x8 blocks.
            for(int by = 0; by < 2; by++)
            {
                for(int bx = 0; bx < 2; bx++)
                {
                    // Write the Y component first.
                    for(int j = 0; j < N; j++)
                    {
                        for(int i = 0; i < N; i++)
                        {
                            int in_index = j * N + i;
                            int y_ind = by * N + j;
                            int x_ind = bx * N + i;
                            inY[in_index] = in[y_ind * dN + x_ind];
                        }
                    }
                    s->writeBlockY(inY);
                }
            }
            // Write the U and V blocks afterwards.
            for(int j = 0; j < N; j++)
            {
                for(int i = 0; i < N; i++)
                {
                    int in_index = j * N + i;
                    int y_ind = y * N + j;
                    int x_ind = x * N + i;
                    inU[in_index] = u_dct.coeffs[y_ind * (xSizeNorm/2) + x_ind];
                    inV[in_index] = v_dct.coeffs[y_ind * (xSizeNorm/2) + x_ind];
                }
            }
            s->writeBlockU(inU);
            s->writeBlockV(inV);
        }
    }
    s->finishStream();
    //qWarning() << xSizeNorm << ySizeNorm;
    //qWarning() << xSizeNorm / 16 << ySizeNorm / 16;

    delete[] luma;
    delete[] chroma;
    delete[] in;
    delete[] inY;
    delete[] inU;
    delete[] inV;
    delete[] y_dct.coeffs;
    delete[] u_dct.coeffs;
    delete[] v_dct.coeffs;
    delete s;
}
