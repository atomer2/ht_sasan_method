// Add lib we need
#pragma comment(lib,"lib/cximage.lib")
#pragma comment(lib,"lib/jasper.lib")
#pragma comment(lib,"lib/jbig.lib")
#pragma comment(lib,"lib/Jpeg.lib")
#pragma comment(lib,"lib/libdcr.lib")
#pragma comment(lib,"lib/libpsd.lib")
#pragma comment(lib,"lib/mng.lib")
#pragma comment(lib,"lib/png.lib")
#pragma comment(lib,"lib/Tiff.lib")
#pragma comment(lib,"lib/zlib.lib")


#include <iostream>
#include <string>
#include "ximage.h"
#include <math.h>

//we need to define UNICODE to use CxImage libarary
#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif
template<typename T>
void new2d(size_t cx, size_t cy, T*** rev, T initVal)
{
    T** f = new T*[cy];
    for (int i = 0; i < cy; i++) {
        f[i] = new T[cx];
        //为了提高速度不得不这么做，应该有更好的办法
        memset(f[i], initVal, sizeof(T)*cx);
    }
    *rev = f;
}

template<typename T>
void del2d(size_t cx, size_t cy, T** array)
{
    for (size_t i = 0; i < cy; i++) {
        delete[] array[i];
    }
    delete[] array;
}

float** padarray(size_t cx, size_t cy, float** im, size_t px, size_t py)
{
    float** om;
    //0.0f 是为了让函数模板类型推断正确通过
    new2d((cx + 2 * px), (cy + 2 * py), &om, 0.0f);
    for (size_t i = 0; i < cy; i++) {
        for (size_t j = 0; j < cx; j++) {
            om[i + py][j + px] = im[i][j];
        }
    }
    //padding edge
    for (size_t i = 0; i < cy; i++) {
        for (size_t j = 0; j < px; j++) {
            om[py + i][j] = om[py + i][px];
            om[py + i][cx + px + j] = om[py + i][cx + px - 1];
        }
    }
    for (size_t i = 0; i < cx + 2 * px ; i++) {
        for (size_t j = 0; j < py; j++) {
            om[j][i] = om[py][i];
            om[cy + py + j][i] = om[cy + py - 1][i];
        }
    }
    return om;
}

float** imfilter(float** im, size_t ix, size_t iy, float** filter, size_t fx, size_t fy)
{
    float **pim = padarray(ix, iy, im, fx, fy);
    float **om;
    new2d(ix, iy, &om, 0.0f);
    size_t mx = (fx - 1) / 2;
    size_t my = (fy - 1) / 2;
    for (size_t i = 0; i < iy; i++) {
        for (size_t j = 0; j < ix; j++) {
            om[i][j] = 0;
            for (size_t m = 0; m < fy; m++) {
                for (size_t n = 0; n < fx; n++) {
                    om[i][j] += pim[m + i + my + 1][n + j + mx + 1] * filter[m][n];
                }
            }
        }
    }
    del2d(ix + fx, iy + fy, pim);
    return om;
}

float** gaussianFilterCreator(float sigma, int size)
{
    float sum = 0;
    float **f;
    int mid = (size + 1) / 2;
    new2d(size, size, &f, 0.0f);
    //为什么这里用int就可以用size_t就不行？
    //size_t是没有符号的，但是以下运算是有负数出现的
    //所以不能用size_t
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            sum += f[y][x] = exp(-(pow((x + 1 - mid), 2) + pow((y + 1 - mid), 2)) / (2 * pow(sigma, 2)));
        }
    }
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            f[y][x] /= sum;
        }
    }
    return f;
}
// for debug usage
// void dispMatrix(float **im, size_t sx, size_t sy) {
// std::cout.precision(10);
// for (size_t i = 0; i < sy; i++) {
// for (size_t  j = 0; j < sx; j++) {
// std::cout  << im[i][j] << " ";
// }
// std::cout << std::endl;
// }
// }
typedef struct pos {
    size_t x, y;
} position;

position maxIndensity(float **im, size_t imx, size_t imy, size_t fltx, size_t flty)
{
    position p = {0, 0};
    float maxval = 0;
    for(size_t i = flty; i < imy + flty; i++) {
        for (size_t j = fltx; j < imx + fltx; j++) {
            if (im[i][j] > maxval) {
                maxval = im[i][j];
                p.y = i;
                p.x = j;
            }
        }
    }
    return p;
}

float sum(float **im, size_t imx, size_t imy)
{
    float retval = 0;
    for(size_t i = 0; i < imy; i++) {
        for(size_t j = 0; j < imx; j++) {
            retval += im[i][j];
        }
    }
    return retval;
}


void applyFilterInPosition(float **im, position p, float **filter, size_t fltx, size_t flty)
{
    size_t hfx = (fltx - 1) / 2;
    size_t hfy = (flty - 1) / 2;
    size_t upleftx = p.x - hfx;
    size_t uplefty = p.y - hfy;
    for(size_t i = 0; i < flty; i++) {
        for(size_t j = 0; j < fltx; j++) {
            im[uplefty + i][upleftx + j] -= filter[i][j];
        }
    }
}

float** htSasanMethod(float **im, size_t imx, size_t imy, float **filter, size_t fltx, size_t flty, uint8_t mask)
{
    float **blurImg = im;
    float **paddingBlur;
    size_t halfx = (fltx - 1) / 2;
    size_t halfy = (flty - 1) / 2;
    new2d(imx + 2 * fltx, imy + 2 * flty, &paddingBlur, 0.0f);
    for (size_t i = 0; i < imy; i++) {
        for (size_t j = 0; j < imx; j++) {
            paddingBlur[i + flty][j + fltx] = blurImg[i][j];
        }
    }
    //返回的二值图像 biImg
    float **biImg;
    new2d(imx, imy, &biImg, 0.0f);
    //直接把浮点数传递给整型会有问题吗？
    size_t dots = sum(im, imx, imy);
    for(size_t i = 0; i < dots; i++) {
        position maxPos = maxIndensity(paddingBlur, imx, imy, fltx, flty);
        //这一句是有用的
        paddingBlur[maxPos.y][maxPos.x] = -1;
        biImg[maxPos.y - flty][maxPos.x - fltx] = 1.0f;
        applyFilterInPosition(paddingBlur, maxPos, filter, fltx, flty);
        //dealing with edge
        if (mask & 0x01 && maxPos.y == flty) {
            for (size_t i = 1; i <= halfy; i++) {
                position p = { maxPos.x, maxPos.y - i };
                applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
            }
        }
        if (mask & 0x02 && maxPos.y == flty + imy - 1) {
            for (size_t i = 1; i <= halfy; i++) {
                position p = { maxPos.x, maxPos.y + i };
                applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
            }

        }
        if (mask & 0x04 && maxPos.x == fltx) {
            for (size_t i = 1; i <= halfx; i++) {
                position p = { maxPos.x - i, maxPos.y };
                applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
            }
        }
        if (mask & 0x08 && maxPos.x == fltx + imx - 1) {
            for (size_t i = 1; i <= halfy; i++) {
                position p = { maxPos.x + i, maxPos.y };
                applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
            }
        }
        //dealing with corner
        if (mask & 0x10 && maxPos.x == fltx && maxPos.y == flty) {
            for (size_t i = 1; i <= halfy; i++) {
                for (size_t j = 1; j <= halfx; j++) {
                    position p = { maxPos.x - j, maxPos.y - i };
                    applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
                }
            }
        }
        if (mask & 0x20 && maxPos.x == fltx && maxPos.y == flty + imy - 1) {
            for (size_t i = 1; i <= halfy; i++) {
                for (size_t j = 1; j <= halfx; j++) {
                    position p = { maxPos.x - j, maxPos.y + i };
                    applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
                }
            }
        }
        if (mask & 0x40 && maxPos.x == fltx + imx - 1 && maxPos.y == flty) {
            for (size_t i = 1; i <= halfy; i++) {
                for (size_t j = 1; j <= halfx; j++) {
                    position p = { maxPos.x + j, maxPos.y - i };
                    applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
                }
            }
        }
        if (mask & 0x80 && maxPos.x == fltx + imx - 1 && maxPos.y == flty + imy - 1) {
            for (size_t i = 1; i <= halfy; i++) {
                for (size_t j = 1; j <= halfx; j++) {
                    position p = { maxPos.x + j, maxPos.y + i };
                    applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
                }
            }
        }

    }
    del2d(imx + 2 * fltx, imy + 2 * flty, paddingBlur);
    return biImg;
}


int main()
{
    CxImage image;
    //divide the image into small blocks so we can process it in parallel by adding OpenMP support
    size_t blockSize = 32;
    size_t fltSize = 11;
    float **gausFilter = gaussianFilterCreator(1.3f, 11);
    // _T() is necessary if we need UNICODE support
    if (image.Load(_T("lena.jpg") , CXIMAGE_SUPPORT_JPG)) {
        // To simplify the problem ,convert the image to 8 bits gray scale
        image.GrayScale();
        size_t width = image.GetWidth();
        size_t height = image.GetHeight();
        size_t xBlocks = (width + blockSize - 1) / blockSize;
        size_t yBlocks = (height + blockSize - 1) / blockSize;
        size_t padImg_x = blockSize * xBlocks;
        size_t padImg_y = blockSize * yBlocks;
        float** imageData;
        new2d(padImg_x, padImg_y, &imageData, 0.0f);

        for (size_t y = 0; y < height; y++) {
            uint8_t *iSrc = image.GetBits(y);
            for (size_t x = 0; x < width; x++) {
                imageData[y][x] = iSrc[x] / 255.0;
            }
        }
        //padding image
        for(size_t x = width; x < padImg_x; x++) {
            for(size_t y = 0; y < height; y++) {
                imageData[y][x] = imageData[y][width - 1];
            }
        }
        for(size_t y = height; y < padImg_y; y++) {
            for(size_t x = 0; x < padImg_x; x++) {
                imageData[y][x] = imageData[height - 1][x];
            }
        }
        float **blurImageData = imfilter(imageData, padImg_x, padImg_y, gausFilter, fltSize, fltSize);
        //halftone image
        float **htImage;
        new2d(padImg_x, padImg_y, &htImage, 0.0f);
        //计时函数
        DWORD dwStart;
        DWORD dwEnd;
        DWORD dwTimes;
        dwStart = GetTickCount();
        /////////////////////////////////////////////////////////////////////
        size_t phase_coef[4][2] = {0, 0, 1, 0, 0, 1, 1, 1};
        for(size_t p = 0; p < 4; p++) {
            uint8_t edgeMask = 0x00;
            size_t phase_x;
            size_t phase_y;
            phase_x = (xBlocks + 1 - phase_coef[p][0]) / 2;
            phase_y = (yBlocks + 1 - phase_coef[p][1]) / 2;
            float **fltHtImage = imfilter(htImage, padImg_x, padImg_y, gausFilter, fltSize, fltSize);
            for(size_t i = 0; i < phase_y; i++) {
                for(size_t j = 0; j < phase_x; j++) {
                    size_t blockIdx_x = 2 * j + phase_coef[p][0];
                    size_t blockIdx_y = 2 * i + phase_coef[p][1];
                    size_t xc = blockSize * blockIdx_x;
                    size_t yc = blockSize * blockIdx_y;
                    switch(p) {
                    case 0:
                        edgeMask = 0xff;
                        break;
                    case 1:
                        edgeMask = 0xf3;
                        if(blockIdx_x == xBlocks - 1) {
                            edgeMask |= 0x08;
                        }
                        break;
                    case 2:
                        edgeMask = 0x0c;
                        if(blockIdx_x == xBlocks - 1) {
                            edgeMask |= 0xc0;
                        }
                        if(blockIdx_x == 0) {
                            edgeMask |= 0x30;
                        }
                        if(blockIdx_y == yBlocks - 1) {
                            edgeMask |= 0xa2;
                        }
                        break;
                    case 3:
                        edgeMask = 0x00;
                        if(blockIdx_x == xBlocks - 1) {
                            edgeMask |= 0xc8;
                        }
                        if(blockIdx_y == yBlocks - 1) {
                            edgeMask |= 0xa2;
                        }
                        break;
                    }
                    float** blockImage;
                    new2d(blockSize, blockSize, &blockImage, 0.0f);
                    for(size_t m = 0; m < blockSize; m++) {
                        for(size_t n = 0; n < blockSize; n++) {
                            blockImage[m][n] = blurImageData[yc + m][xc + n] - fltHtImage[yc + m][xc + n];
                        }
                    }
                    float **res = htSasanMethod(blockImage, blockSize, blockSize, gausFilter, 11, 11, edgeMask);
                    for(size_t m = 0; m < blockSize; m++) {
                        for(size_t n = 0; n < blockSize; n++) {
                            htImage[yc + m][xc + n] = res[m][n];
                        }
                    }
                    del2d(blockSize, blockSize, blockImage);
                    del2d(blockSize, blockSize, res);
                }
            }
            del2d(padImg_x, padImg_y, fltHtImage);
        }
        //////////////////////////////////////////////////
        dwEnd = GetTickCount();
        dwTimes = dwEnd - dwStart;
        printf("加网lena图用时 %ld ms\n", dwTimes);
        uint8_t **htImageByte;
        new2d(padImg_x, padImg_y, &htImageByte, uint8_t(0));
        for(size_t i = 0; i < padImg_y; i++) {
            for(size_t j = 0; j < padImg_x; j++) {
                htImageByte[i][j] = htImage[i][j] * 255;
            }
        }
        CxImage outputImg;
        outputImg.CreateFromMatrix(htImageByte, width, height, 8, padImg_x, NULL);
        outputImg.Save(_T("ht_lena.bmp"), CXIMAGE_SUPPORT_BMP);
        del2d(11, 11, gausFilter);
        del2d(padImg_x, padImg_y, htImage);
        del2d(padImg_x, padImg_y, htImageByte);
        del2d(padImg_x, padImg_y, imageData);
        std::cout << "done\n";
        getchar();
    }
}
