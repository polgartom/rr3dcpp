#ifndef H_ARRAY
#define H_ARRAY

#include <assert.h>

template <typename T>
struct Array {
    T *data = nullptr;
    long allocated = 0;
    long count = 0;
    long iterator_index = 0;

    ~Array() {
        if (this->allocated != 0) {
            free(this->data);
        }
    };

    T& operator[](unsigned long index) {
        // ASSERT(index < this->allocated || index >= this->allocated, "Array index out of bound");  
        ASSERT(index < this->count || index >= this->count, "Array index out of bound");  
        return this->data[index];
    }

    typedef T* iterator;
    typedef const T* const_iterator;
    
    iterator begin() { return data; }
    iterator begin() const { return data; }
    iterator end() { return count ? &data[count-1] : this->begin(); }
    const_iterator end() const { return count ? &data[count-1] : this->begin(); }
    
    iterator& operator++() {
        if (iterator_index+1 < count) {
            iterator_index += 1;
        }
        return data[iterator_index];
    }
    
    bool operator!=(iterator& rhs) {
        return &data[iterator_index] != rhs;
    }
};

template <typename T>
void array_add(Array<T> *arr, T item)
{
    if (arr->count >= arr->allocated) {
        long reserve = 2 * arr->allocated;
        if (reserve < 8) reserve = 8;
        if (!arr->data) {        
            arr->data = static_cast<T *>(malloc(sizeof(T) * reserve));
        } else {
            arr->data = static_cast<T *>(realloc(arr->data, sizeof(T) * reserve));
        }        
        arr->allocated = reserve;
    }

    arr->data[arr->count] = item;
    arr->count += 1;    
}

template <typename T>
T array_last_item(Array<T> *arr)
{
    if (arr->count == 0) return nullptr;

    return arr->data[arr->count-1];
}

template <typename T>
T array_pop(Array<T> *arr)
{
    if (arr->count == 0) return nullptr;

    T *r = arr->data[arr->count-1];
    arr->count -= 1;

    return r;
}

#define For(_arr) for (auto it = _arr.begin(); it != _arr.end(); it++)
#define For_Index(_arr) for (auto it_index = 0; it_index < _arr.count; it_index++)

#endif 