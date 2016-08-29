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
#include "matrix.h"
//we need to define UNICODE to use CxImage libarary
#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

template<typename T>
matrix<T>* padarray(matrix<T>& im, size_t px, size_t py)
{
    size_t cx = im.get_width();
    size_t cy = im.get_height();
    matrix<T> *om = new matrix<T>(cx + 2 * px, cy + 2 * py);
    om->put_sub_matrix(py, px, im);
    //padding edge and corner
    for (size_t i = 0; i < cy; i++)
        {
            for (size_t j = 0; j < px; j++)
                {
                    (*om)(py + i, j) = (*om)(py + i, px);
                    (*om)(py + i, cx + px + j) = (*om)(py + i, cx + px - 1);
                }
        }
    for (size_t i = 0; i < cx + 2 * px; i++)
        {
            for (size_t j = 0; j < py; j++)
                {
                    (*om)(j, i) = (*om)(py, i);
                    (*om)(cy + py + j, i) = (*om)(cy + py - 1, i);
                }
        }
    return om;
}

template<typename T>
matrix<float>* imfilter(matrix<T> &im, matrix<float> &filter)
{
    size_t ix = im.get_width();
    size_t iy = im.get_height();
    size_t fx = filter.get_width();
    size_t fy = filter.get_height();
    matrix<T> *pim = padarray<T>(im, fx, fy);
    matrix<float> *om = new matrix<float>(ix, iy);
    size_t mx = (fx - 1) / 2;
    size_t my = (fy - 1) / 2;
    for (size_t i = 0; i < iy; i++)
        {
            for (size_t j = 0; j < ix; j++)
                {
                    for (size_t m = 0; m < fy; m++)
                        {
                            for (size_t n = 0; n < fx; n++)
                                {
                                    (*om)(i, j) += (*pim)(m + i + my + 1, n + j + mx + 1) * filter(m, n);
                                }
                        }
                }
        }
    delete pim;
    return om;
}

matrix<float>* gaussianFilterCreator(float sigma, size_t size)
{
    float sum = 0;
    matrix<float> *f = new matrix<float>(size, size);
    int mid = (size + 1) / 2;
    //为什么这里用int就可以用size_t就不行？
    //size_t是没有符号的，但是以下运算是有负数出现的
    //所以不能用size_t
    for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
                {
                    sum += (*f)(y, x) = exp(-(pow((x + 1 - mid), 2) + pow((y + 1 - mid), 2)) / (2 * pow(sigma, 2)));

                }
        }
    for (size_t y = 0; y < size; y++)
        {
            for (size_t x = 0; x < size; x++)
                {
                    (*f)(y, x) /= sum;
                }
        }
    return f;
}

void applyFilterInPosition(matrix<float> &im, size_t x, size_t y, matrix<float> &filter)
{
    size_t fw = filter.get_width();
    size_t fh = filter.get_height();
    size_t hfw = (fw - 1) / 2;
    size_t hfh = (fh - 1) / 2;
    size_t upleftx = x - hfw;
    size_t uplefty = y - hfh;
    for(size_t i = 0; i < fh; i++)
        {
            for(size_t j = 0; j < fw; j++)
                {
                    im(uplefty + i, upleftx + j) -= filter(i, j);
                }
        }
}

matrix<uint8_t>* htSasanMethod(matrix<float> &im, matrix<float> &filter, size_t ndots)
{
    size_t imw = im.get_width();
    size_t imh = im.get_height();
    size_t fltw = filter.get_width();
    size_t flth = filter.get_height();
    matrix<float> *padding_blur = new matrix<float>(imw + fltw * 2, imh + flth * 2);
    size_t halfx = (fltw - 1) / 2;
    size_t halfy = (flth - 1) / 2;
    padding_blur->put_sub_matrix(fltw, flth, im);
    //返回的二值图像
    matrix<uint8_t> *halftone_image = new matrix<uint8_t>(imw, imh);
    //直接把浮点数传递给整型会有问题吗？
    for(size_t d = 0; d < ndots; d++)
        {
            pos_t p = padding_blur->max_value_position(fltw, flth, imw, imh);
            //这一句是有用的
            (*padding_blur)(p.y, p.x) = -1;
            (*halftone_image)(p.y - flth, p.x - fltw) = 1;
            applyFilterInPosition(*padding_blur, p.x, p.y, filter);
            //dealing with edge
            if (p.y == flth)
                {
                    for (size_t i = 1; i <= halfy; i++)
                        {
                            pos_t pt = { p.x, p.y - i };
                            applyFilterInPosition(*padding_blur, pt.x, pt.y, filter);
                        }
                }
            if (p.y == flth + imh - 1)
                {
                    for (size_t i = 1; i <= halfy; i++)
                        {
                            pos_t pt = { p.x, p.y + i };
                            applyFilterInPosition(*padding_blur, pt.x, pt.y, filter);
                        }
                }
            if (p.x == fltw)
                {
                    for (size_t i = 1; i <= halfx; i++)
                        {
                            pos_t pt = { p.x - i, p.y };
                            applyFilterInPosition(*padding_blur, pt.x, pt.y, filter);
                        }
                }
            //dealing with corner
            if (p.x == fltw + imw - 1)
                {
                    for (size_t i = 1; i <= halfy; i++)
                        {
                            pos_t pt = { p.x + i, p.y };
                            applyFilterInPosition(*padding_blur, pt.x, pt.y, filter);
                        }
                }
            if (p.x == fltw && p.y == flth)
                {
                    for (size_t i = 1; i <= halfy; i++)
                        {
                            for (size_t j = 1; j <= halfx; j++)
                                {
                                    pos_t pt = { p.x - j, p.y - i };
                                    applyFilterInPosition(*padding_blur, pt.x, pt.y, filter);
                                }
                        }
                }
            if (p.x == fltw && p.y == flth + imh - 1)
                {
                    for (size_t i = 1; i <= halfy; i++)
                        {
                            for (size_t j = 1; j <= halfx; j++)
                                {
                                    pos_t pt = { p.x - j, p.y + i };
                                    applyFilterInPosition(*padding_blur, pt.x, pt.y, filter);
                                }
                        }
                }
            if (p.x == fltw + imw - 1 && p.y == flth)
                {
                    for (size_t i = 1; i <= halfy; i++)
                        {
                            for (size_t j = 1; j <= halfx; j++)
                                {
                                    pos_t pt = { p.x + j, p.y - i };
                                    applyFilterInPosition(*padding_blur, pt.x, pt.y, filter);
                                }
                        }
                }
            if (p.x == fltw + imw - 1 && p.y == flth + imh - 1)
                {
                    for (size_t i = 1; i <= halfy; i++)
                        {
                            for (size_t j = 1; j <= halfx; j++)
                                {
                                    pos_t pt = { p.x + j, p.y + i };
                                    applyFilterInPosition(*padding_blur, pt.x, pt.y, filter);
                                }
                        }
                }
        }
    //释放内存
    delete padding_blur;
    return halftone_image;
}

void apply_block_sasan_method(size_t blocks_x,
                              size_t blocks_y,
                              matrix<float> &origin_image,
                              matrix<float> &im,
                              matrix<float> &filter,
                              matrix<uint8_t> &halftone_image,
                              size_t block_size)
{

    size_t phase_coef[4][2] = {0, 0, 0, 1, 1, 0, 1, 1};

    for(size_t p = 0; p < 4; p++)
        {
            size_t phase_x;
            size_t phase_y;
            matrix<float> *blur_halftone_image = imfilter<uint8_t>(halftone_image, filter);
            phase_x = (blocks_x + 1 - phase_coef[p][0]) / 2;
            phase_y = (blocks_y + 1 - phase_coef[p][1]) / 2;
            for(size_t i = 0; i < phase_y; i++)
                {
                    for(size_t j = 0; j < phase_x; j++)
                        {
                            size_t xc = block_size * (2 * j + phase_coef[p][0]);
                            size_t yc = block_size * (2 * i + phase_coef[p][1]);
                            matrix<float> *blur_block = blur_halftone_image->get_sub_matrix(yc, xc, block_size, block_size);
                            matrix<float> *block_image = im.get_sub_matrix(yc, xc, block_size, block_size);
                            matrix<float> *subtracted_image = *block_image - *blur_block;
                            matrix<float> *origin_block_image = origin_image.get_sub_matrix(yc, xc, block_size, block_size);
                            size_t ndots = origin_block_image->sum();
                            matrix<uint8_t> *res = htSasanMethod(*subtracted_image, filter, ndots);
                            halftone_image.put_sub_matrix(yc, xc, *res);
                            delete res;
                            delete block_image;
                            delete origin_block_image;
                            delete subtracted_image;
                            delete blur_block;
                        }
                }
        }
}

int main()
{
    CxImage image;
    //divide the image into small blocks so we can process it in parallel by adding OpenMP support
    size_t block_size = 32;
    size_t flt_size = 11;
    matrix<float> *gaus_filter = gaussianFilterCreator(1.3f, flt_size);
    // _T() is necessary if we need UNICODE support
    if (image.Load(_T("lena.jpg") , CXIMAGE_SUPPORT_JPG))
        {
            // To simplify the problem ,convert the image to 8 bits gray scale
            image.GrayScale();
            size_t width = image.GetWidth();
            size_t height = image.GetHeight();
            size_t blocks_w = (width + block_size - 1) / block_size;
            size_t blocks_h = (height + block_size - 1) / block_size;
            size_t padding_image_w = block_size * blocks_w;
            size_t padding_image_h = block_size * blocks_h;
            matrix<float> image_data(padding_image_w, padding_image_h);

            for (size_t y = 0; y < height; y++)
                {
                    uint8_t *iSrc = image.GetBits(y);
                    for (size_t x = 0; x < width; x++)
                        {
                            image_data(y, x) = iSrc[x] / 255.0f;
                        }
                }
            //padding image
            for(size_t x = width; x < padding_image_w; x++)
                {
                    for(size_t y = 0; y < height; y++)
                        {
                            image_data(y, x) = image_data(y, width - 1);
                        }
                }
            for(size_t y = height; y < padding_image_h; y++)
                {
                    for(size_t x = 0; x < padding_image_w; x++)
                        {
                            image_data(y, x) = image_data(height - 1, x);
                        }
                }
            matrix<float>  *blur_image_data = imfilter<float>(image_data, *gaus_filter);

            //halftone image
            matrix<uint8_t> halftone_image(padding_image_w, padding_image_h);
            //计时函数
            DWORD dwStart;
            DWORD dwEnd;
            DWORD dwTimes;
            dwStart = GetTickCount();
            apply_block_sasan_method(blocks_w, blocks_h, image_data, *blur_image_data, *gaus_filter, halftone_image, block_size);
            for (size_t i = 0; i < padding_image_h; i++)
                {
                    for(size_t j = 0; j < padding_image_w; j++)
                        {
                            halftone_image(i, j) = halftone_image(i, j) * 255;
                        }
                }
            dwEnd = GetTickCount();
            dwTimes = dwEnd - dwStart;
            printf("加网lena图用时 %ld ms\n", dwTimes);
            CxImage outputImg;
            outputImg.CreateFromArray(halftone_image.get_elements_pointer(), width, height, 8, padding_image_w, NULL);
            outputImg.Save(_T("ht_lena.bmp"), CXIMAGE_SUPPORT_BMP);

            std::cout << "done\n";
            getchar();
        }
}


