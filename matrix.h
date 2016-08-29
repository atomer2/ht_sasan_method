#ifndef __matrix_h
#define __matrix_h
#include <memory.h>
typedef struct position
{
    size_t x;
    size_t y;
} pos_t;

template<typename T>
class matrix
{
private:
    size_t height;
    size_t width;
    T* elements;
    size_t plain_to_2d(size_t x, size_t y);
public:
    matrix(size_t width_p, size_t height_p);
    ~matrix();
    size_t get_height();
    size_t get_width();
    T sum(void);
    T& operator() (size_t y, size_t x);
    matrix<T>* operator - (matrix<T> &subtractor);
    matrix<T>* get_sub_matrix(size_t y, size_t x, size_t sub_matrix_w, size_t sub_matrix_h);
    pos_t max_value_position(size_t left, size_t top, size_t w, size_t h);
    void put_sub_matrix(size_t y, size_t x, matrix<T> &im);
    T* get_elements_pointer();
};


template<typename T>
matrix<T>::matrix(size_t width_p, size_t height_p)
    : width(width_p), height(height_p)
{
    size_t element_count = width * height;
    elements = new T[element_count];
    //initialize matrix elements to zero
    memset(elements, 0, element_count * sizeof(T));
}

template<typename T>
matrix<T>::~matrix()
{
    delete[] elements;
}

template<typename T>
size_t matrix<T>::get_width()
{
    return width;
}

template<typename T>
size_t matrix<T>::get_height()
{
    return height;
}


template<typename T>
size_t matrix<T>::plain_to_2d(size_t x, size_t y)
{
    return y * width + x;
}


template<typename T>
T matrix<T>::sum(void)
{
    T ret_value = 0;
    size_t count = height * width;
    for (size_t i = 0; i < count; i++)
        {
            ret_value += elements[i];
        }
    return ret_value;
}


template<typename T>
T& matrix<T>::operator()(size_t y, size_t x)
{
    return elements[plain_to_2d(x, y)];
}

template<typename T>
matrix<T>* matrix<T>::get_sub_matrix(size_t y, size_t x, size_t sub_matrix_w, size_t sub_matrix_h)
{
    matrix<T> *sub_matrix = new matrix<T>(sub_matrix_w, sub_matrix_h);
    for (size_t i = 0; i < sub_matrix_h; i++)
        {
            for (size_t j = 0; j < sub_matrix_w; j++)
                {
                    (*sub_matrix)(i, j) = (*this)(i + y, j + x);
                }
        }
    return sub_matrix;
}

template<typename T>
void matrix<T>::put_sub_matrix(size_t y, size_t x, matrix<T> &im)
{
    size_t sub_width = im.get_width();
    size_t sub_height = im.get_height();
    for (size_t i = 0; i < sub_height; i++)
        {
            for (size_t j = 0; j < sub_width; j++)
                {
                    (*this)(i + y, j + x) = im(i, j);
                }
        }
}

template<typename T>
pos_t matrix<T>::max_value_position(size_t left, size_t top, size_t w, size_t h)
{
    T itor = T(0);
    pos_t idx = { left, top };
    for (size_t i = 0; i < h; i++)
        {
            for (size_t j = 0; j < w; j++)
                {
                    if ((*this)(i + top, j + left) > itor)
                        {
							itor = (*this)(i + top, j + left);
                            idx.x = j + left;
                            idx.y = i + top;
                        }
                }
        }
    return idx;
}

template<typename T>
T* matrix<T>::get_elements_pointer(void)
{
    return elements;
}


template<typename T>
matrix<T>* matrix<T>::operator -(matrix<T> &subtractor)
{
    matrix<T> *res = new matrix<T>(width, height);
    for (size_t i = 0; i < width * height; i++)
        {
            res->get_elements_pointer()[i] = elements[i] - subtractor.get_elements_pointer()[i];
        }
    return res;
}

#endif