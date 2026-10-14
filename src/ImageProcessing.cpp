
#include "ImageProcessing.h"
#include "ColorSpaces.h"
#include "JPEG.h"

#include <cmath>

#include <QDebug>
#include <QString>
#include <QImage>

void imageProcessingFun(const QString& progName, QImage& outImgs, const QImage& inImgs, const QVector<double>& params)
{
    int width = inImgs.width();
    int height = inImgs.height();
    // Find next larger size that's divisible by 16.
    // This is done because we're writing 4 8x8 block rows to the
    // output file, so we always increment by 16 pixels.
    if(width % 16 != 0)  { width = 16 * (width / 16) + 16; }
    if(height % 16 != 0) { height = 16 * (height / 16) + 16; }

    int bufSize = width*height;

    /* Create buffers for YUV image */
    uchar* Y_buff = new uchar[bufSize];
    char* U_buff = new char[bufSize / 4];
    char* V_buff = new char[bufSize / 4];

    /*
     * QImage::copy(x, y, w, h) will return a new QImage
     * with the defined size. If the provided size is larger
     * than the original, the new QImage will be padded out
     * with black pixels. This is much easier than having to
     * pad out individual buffer sizes.
     *
     * This technically doesn't align with JPEG standards,
     * which recommend extending the image borders with the
     * last column/row of the image (i.e default UV sampling
     * behavior in OpenGL), but this seems to work just fine.
     */
    QImage input = inImgs.copy(0, 0, width, height);

    /* Create empty output image */
    //outImgs = QImage(inImgs.width(), inImgs.height(), input.format());

    /* Convert input image to YUV420 image */
    RGBtoYUV420(input.bits(), width, height, Y_buff, U_buff, V_buff);

    if(progName == QString("JPEG Encoder"))
    {
        /* Perform NxN DCT */
        performJPEGEncoding(Y_buff, U_buff, V_buff, inImgs.width(), inImgs.height(), params[0]);
    }

    /* Convert YUV image back to RGB */
    // Doesn't seem to be doing anything anyway
    //YUV420toRGB(Y_buff, U_buff, V_buff, inImgs.width(), inImgs.height(), outImgs.bits());

    outImgs = QImage("example.jpg");

    /* Delete used memory buffers */
    delete[] Y_buff;
    delete[] U_buff;
    delete[] V_buff;

}

