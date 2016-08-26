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
#include "math.h"

//we need to define UNICODE to use CxImage libarary
#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif
template<typename T>
void new2d(int32_t cx, int32_t cy, T*** rev, T initVal) {
    T** f = new T*[cy];
    for (int i = 0; i < cy; i++) {
        f[i] = new T[cx];
        //为了提高速度不得不这么做，应该有更好的办法
        memset(f[i], initVal, sizeof(T)*cx);
    }
    *rev = f;
}

template<typename T>
void del2d(int32_t cx, int32_t cy, T** array) {
    for (int32_t i = 0; i < cy; i++) {
        delete[] array[i];
    }
    delete[] array;
}

float** padarray(int32_t cx, int32_t cy, float** im, int32_t px, int32_t py) {
    float** om;
    //0.0f 是为了让函数模板类型推断正确通过
    new2d((cx + 2 * px), (cy + 2 * py), &om, 0.0f);
    for (int32_t i = 0; i < cy; i++) {
        for (int32_t j = 0; j < cx; j++) {
            om[i + py][j + px] = im[i][j];
        }
    }
    //padding edge
    for (int32_t i = 1; i < cy - 1; i++) {
        for (int32_t j = 0; j < px; j++) {
            om[py + i][j] = om[py + i][px];
            om[py + i][cx + px + j] = om[py + i][cx + px - 1];
        }
    }
    for (int32_t i = 1; i < cx - 1; i++) {
        for (int32_t j = 0; j < py; j++) {
            om[j][px + i] = om[py][px + i];
            om[cy + py + j][px + i] = om[cy + py - 1][py + i];
        }
    }
    //padding corner
    for (int32_t i = 0; i <= py; i++) {
        for (int32_t j = 0; j <= px; j++) {
            om[i][j] = om[py][px];
            om[i][j + px + cx - 1] = om[py][cx + px - 1];
            om[i + py + cy - 1][j] = om[py + cy - 1][px];
            om[i + py + cy - 1][j + px + cx - 1] = om[py + cy - 1][px + cx - 1];
        }
    }
    return om;

}

float** imfilter(float** im, int32_t ix, int32_t iy, float** filter, int32_t fx, int32_t fy) {
    float **pim = padarray(ix, iy, im, fx, fy);
    float **om;
    new2d(ix, iy, &om, 0.0f);
    int32_t mx = (fx - 1) / 2;
    int32_t my = (fy - 1) / 2;
    for (int32_t i = 0; i < iy; i++) {
        for (int32_t j = 0; j < ix; j++) {
            om[i][j] = 0;
            for (int32_t m = 0; m < fy; m++) {
                for (int32_t n = 0; n < fx; n++) {
                    om[i][j] += pim[m + i + my + 1][n + j + mx + 1] * filter[m][n];
                }
            }
        }
    }
    del2d(ix + fx, iy + fy, pim);
    return om;
}

float** gaussianFilterCreator(float sigma, int size) {
    float sum = 0;
    float **f;
    int mid = (size + 1) / 2;
    new2d(size, size, &f, 0.0f);
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
void dispMatrix(float **im, int32_t sx, int32_t sy) {
    std::cout.precision(10);
    for (int32_t i = 0; i < sy; i++) {
        for (int32_t  j = 0; j < sx; j++) {
            std::cout  << im[i][j] << " ";
        }
        std::cout << std::endl;
    }
}
typedef struct pos {
    int32_t x, y;
} position;

position maxIndensity(float **im, int32_t imx, int32_t imy, int32_t fltx, int32_t flty) {
    position p = {0, 0};
    float maxval = 0;
    for(int32_t i = flty; i < imy + flty; i++) {
        for (int32_t j = fltx; j < imx + fltx; j++) {
            if (im[i][j] > maxval) {
                maxval = im[i][j];
                p.y = i;
                p.x = j;
            }
        }
    }
    return p;
}

float sum(float **im, int32_t imx, int32_t imy) {
    float retval = 0;
    for(int32_t i = 0; i < imy; i++) {
        for(int32_t j = 0; j < imx; j++) {
            retval += im[i][j];
        }
    }
    return retval;
}
//这个函数可以优化，不要每次都重新分配内存
//void matrixBiop(float **im1, float **im2, float **im3, int32_t imx, int32_t imy, int32_t co) {
//    for(int32_t i = 0; i < imy; i++) {
//        for(int32_t j = 0; j < imx; j++) {
//            im3[i][j] = im1[i][j] + co * im2[i][j];
//        }
//    }
//}


void applyFilterInPosition(float **im, position p, float **filter, int32_t fltx, int32_t flty) {
    int32_t hfx = (fltx - 1) / 2;
    int32_t hfy = (flty - 1) / 2;
    int32_t upleftx = p.x - hfx;
    int32_t uplefty = p.y - hfy;
    for(int32_t i = 0; i < flty; i++) {
        for(int32_t j = 0; j < fltx; j++) {
            im[uplefty + i][upleftx + j] -= filter[i][j];
        }
    }
}

uint8_t** htSasanMethod(float **im, int32_t imx, int32_t imy, float **filter, int32_t fltx, int32_t flty) {
    float **blurImg = imfilter(im, imx, imy, filter, fltx, flty);
    float **paddingBlur;
    int32_t halfx = (fltx - 1) / 2;
    int32_t halfy = (flty - 1) / 2;
    new2d(imx + 2 * fltx, imy + 2 * flty, &paddingBlur, 0.0f);
    for (int32_t i = 0; i < imy; i++) {
        for (int32_t j = 0; j < imx; j++) {
            paddingBlur[i + flty][j + fltx] = blurImg[i][j];
        }
    }
    //返回的二值图像 biImg
    uint8_t **biImg;
    new2d(imx, imy, &biImg, uint8_t(0));
    //直接把浮点数传递给整型会有问题吗？
    int32_t dots = sum(im, imx, imy);
    for(int32_t i = 0; i < dots; i++) {
        //errorImg要释放内存
        //matrixBiop(blurImg, midBlur, errorImg, imx, imy, -1);
        position maxPos = maxIndensity(paddingBlur, imx, imy, fltx, flty);
        //这一句是有用的
        paddingBlur[maxPos.y][maxPos.x] = -1;
        biImg[maxPos.y - flty][maxPos.x - fltx] = 255;
        applyFilterInPosition(paddingBlur, maxPos, filter, fltx, flty);
        //dealing with edge
        if (maxPos.y == flty) {
            for (int32_t i = 1; i <= halfy; i++) {
                position p = { maxPos.x, maxPos.y - i };
                applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
            }
        }
        if (maxPos.y == flty + imy - 1) {
            for (int32_t i = 1; i <= halfy; i++) {
                position p = { maxPos.x, maxPos.y + i };
                applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
            }

        }
        if (maxPos.x == fltx) {
            for (int32_t i = 1; i <= halfx; i++) {
                position p = { maxPos.x - i, maxPos.y };
                applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
            }
        }
        //dealing with corner
        if (maxPos.x == fltx + imx - 1) {
            for (int32_t i = 1; i <= halfy; i++) {
                position p = { maxPos.x + i, maxPos.y };
                applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
            }
        }
        if (maxPos.x == fltx && maxPos.y == flty) {
            for (int32_t i = 1; i <= halfy; i++) {
                for (int32_t j = 1; j <= halfx; j++) {
                    position p = { maxPos.x - j, maxPos.y - i };
                    applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
                }
            }
        }
        if (maxPos.x == fltx && maxPos.y == flty + imy - 1) {
            for (int32_t i = 1; i <= halfy; i++) {
                for (int32_t j = 1; j <= halfx; j++) {
                    position p = { maxPos.x - j, maxPos.y + i };
                    applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
                }
            }
        }
        if (maxPos.x == fltx + imx - 1 && maxPos.y == flty) {
            for (int32_t i = 1; i <= halfy; i++) {
                for (int32_t j = 1; j <= halfx; j++) {
                    position p = { maxPos.x + j, maxPos.y - i };
                    applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
                }
            }
        }
        if (maxPos.x == fltx + imx - 1 && maxPos.y == flty + imy - 1) {
            for (int32_t i = 1; i <= halfy; i++) {
                for (int32_t j = 1; j <= halfx; j++) {
                    position p = { maxPos.x + j, maxPos.y + i };
                    applyFilterInPosition(paddingBlur, p, filter, fltx, flty);
                }
            }
        }

    }
    //释放内存
    del2d(imx, imy, blurImg);
    del2d(imx + 2 * fltx, imy + 2 * flty, paddingBlur);
    return biImg;
}

int main() {
    CxImage image;
    float **gausFilter = gaussianFilterCreator(1.3f, 11);
    if (image.Load(_T("lena.jpg") , CXIMAGE_SUPPORT_JPG)) { // _T() is necessary if we need UNICODE support
        image.GrayScale();                                 // To simplify the problem ,convert the image to 8 bits gray scale
        int width = image.GetWidth();
        int height = image.GetHeight();
        float** imaData;
        new2d(width, height, &imaData, 0.0f);
        for (int32_t y = 0; y < height; y++) {
            uint8_t *iSrc = image.GetBits(y);
            for (int32_t x = 0; x < width; x++) {
                imaData[y][x] = iSrc[x] / 255.0;
            }
        }
        //dispMatrix(imaData, width, height);
        //blurred image
        uint8_t **u8Img;
        // new2d(height, width, &u8Img, uint8_t(0));
        //计时函数
        DWORD dwStart;
        DWORD dwEnd;
        DWORD dwTimes;
        dwStart = GetTickCount();
        u8Img = htSasanMethod(imaData, width, height, gausFilter, 11, 11);
        dwEnd = GetTickCount();
        dwTimes = dwEnd - dwStart;
        printf("加网lena图用时 %ld ms\n", dwTimes);
        CxImage outputImg;
        outputImg.CreateFromMatrix(u8Img, width, height, 8, width, NULL);

        outputImg.Save(_T("ht_lena.bmp"), CXIMAGE_SUPPORT_BMP);
        del2d(width, height, imaData);
        del2d(11, 11, gausFilter);
        del2d(width, height, u8Img);
        std::cout << "done\n";
        getchar();
    }
}












