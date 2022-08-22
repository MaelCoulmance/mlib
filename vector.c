/**
 * @file   vector.h
 * 
 * @author Maël Coulmance 
 * 
 * @brief  A small library for manipulating dynamically allocated arrays (vector).
 *         Vector are constructed using small buffer optimization. An initial buffer is allocated on the stack, its size is defined
 *         by the macro 'MC_VECTOR_BUFSIZE'. If we need more memory than we got on the stack, then a new buffer will be allocated
 *         on the heap. This means that the vector's capacity can be increased or decreased at runtime. Note however than a vector
 *         capacity cannot be smaller than 'MC_VECTOR_BUFSIZE', since we will go back to the stack buffer if capacity is decreased
 *         such that it can fit on the stack.
 *         Memory management is handled automatically by the API, but some functions can be used to manually increase or decrease
 *         the vector's capacity (such as 'resize' or 'shrink').
 *         The vector type is a typedef for a pointer to an opaque struct. This means user cannot access the content of the vector
 *         without using API's function, and that vector are passed by reference by default (this behavior can however be bypassed
 *         by using 'struct vector_s' instead of 'vector', but it is not recommended).  
 *         The api provides several function to create and destroy a vector, and to manipulate its content (inserting / removing /
 *         accessing elements). 
 * 
 *         !!!!!!!!!!!!! NOTE THAT SINCE 'VECTOR' IS A POINTER, YOU [[HAVE TO]] DEALLOCATE THEM TO AVOID MEMORY LEAKS !!!!!!!!!!!!!      
 *    
 * @version 1.3
 * 
 * @date 2022-08-17
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "vector.h"


#if !defined(MC_VECTOR_NO_IO) && defined(_WIN32)
#   define fprintf(...)     fprintf_s(__VA_ARGS__)
#   define sprintf(...)     sprintf_s(__VA_ARGS__)
#elif !defined(MC_VECTOR_NO_IO)
#   define sprintf(buf, size, ...) sprintf(buf, __VA_ARGS__)
#endif 

#if defined(__GNUC__) 
#   pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif 

#ifndef MC_VECTOR_NO_IO
#   include <wchar.h>
#endif

#define min(x, y) (((x) < (y)) ? (x) : (y))



struct vector_s {
    size_t count;                           // The number of element currently stored on the vector
    size_t capacity;                        // The total capacity of the vector

    long  stack_buf[MC_VECTOR_BUFSIZE];     // A buffer allocated on the stack
    long *heap_buf;                         // A pointer to a buffer allocated on the heap, if needed

    long  *data;                            // A pointer to the currently used buffer (stack_buf or heap_buf)
};



vector mc_vector_make(size_t capacity) {
    if (capacity == 0) {
        errno = EINVAL;
        return NULL;
    }

    vector res = (vector)(malloc(sizeof (struct vector_s)));

    if (!res) {
        errno = ENOBUFS;
        return NULL;
    }

    if (capacity > MC_VECTOR_BUFSIZE) {
        res->heap_buf = (long*)(malloc(capacity * sizeof (long)));

        if (!res->heap_buf) {
            free(res);
            errno = ENOBUFS;
            return NULL;
        }

        res->data = res->heap_buf;
        res->capacity = capacity;
    }
    else {
        res->data = res->stack_buf;
        res->heap_buf = NULL;
        res->capacity = MC_VECTOR_BUFSIZE;
    }

    res->count = 0;

    return res;
}

vector mc_vector_make_filled(size_t capacity, size_t length, long value) {
    if (capacity == 0 || length == 0 || capacity < length) {
        errno = EINVAL;
        return NULL;
    }

    vector res = mc_vector_make(capacity);

    if (!res)
        return NULL;

    
    // cannot use memset here since we use long :(
    for (size_t i = 0; i < length; i++) {
        res->data[i] = value;
    }

    res->count = length;
    return res;
}

vector mc_vector_clone(vector vec) {
    if (!vec) {
        errno = EFAULT;
        return NULL;
    }

    vector res = mc_vector_make(vec->capacity);

    if (!res)
        return NULL;

    memcpy(res->data, vec->data, vec->count * sizeof (long));

    res->capacity = vec->capacity;
    res->count = vec->count;

    return res;
}

vector mc_vector_from_array(long *src, size_t length) {
    if (!src) {
        errno = EFAULT;
        return NULL;
    }

    if (length == 0) {
        errno = EINVAL;
        return NULL;
    }


    vector res = mc_vector_make(length * 2);

    if (!res)
        return NULL;

    memcpy(res->data, src, length * sizeof (long));

    res->capacity = length * 2;
    res->count = length;

    return res;
}

void mc_vector_free(vector vec) {
    if (!vec)
        return;

    if (vec->heap_buf) 
        free(vec->heap_buf);

    free(vec);
}


int mc_vector_to_array(vector vec, long *buffer) {
    return mc_vector_extract(vec, buffer, 0, vec->count);
}

int mc_vector_extract(vector vec, long *buffer, size_t index, size_t length) {
    if (!vec || !buffer) {
        errno = EFAULT;
        return 0;
    }

    if (index >= vec->count || length == 0 || index + length > vec->count) {
        errno = EINVAL;
        return 0;
    }

    for (size_t i = index, pos = 0; i < index + length; i++, pos++) {
        buffer[pos] = vec->data[i];
    }

    return (int)length;
}




short mc_vector_get(vector vec, size_t index, long *out) {
    if (!vec || !out) {
        errno = EFAULT;
        return 0;
    }

    if (index >= vec->capacity) {
        errno = EINVAL;
        return 0;
    }

    *out = vec->data[index];
    return 1;
}

long mc_vector_get_unchecked(vector vec, size_t index) {
    if (!vec || index >= vec->capacity) {
        return LONG_MAX;
    }

    return vec->data[index];
}

short mc_vector_set(vector vec, size_t index, long value) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    if (index >= vec->count) {
        errno = EINVAL;
        return 0;
    }

    vec->data[index] = value;
    return 1;
}



size_t mc_vector_size(vector vec) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    return vec->count;
}

size_t mc_vector_capacity(vector vec) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    return vec->capacity;
}

short mc_vector_empty(vector vec) {
    if (!vec) {
        errno = EFAULT;
        return 1;
    }

    return vec->count == 0;
}

short mc_vector_is_stack(vector vec) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    return vec->data != vec->heap_buf;
}





 short vec_ensure_capacity(vector vec, size_t len) {
    if (!vec)
        return 0;

    if (vec->count + len >= vec->capacity) {
        // need realloc
        const size_t cap = (vec->capacity * 2) + len;

        long *temp = realloc(vec->heap_buf, cap * sizeof (long));

        if (!temp)
            return 0;

        if (vec->heap_buf == NULL) {
            memcpy(temp, vec->stack_buf, vec->capacity * sizeof (long));
        }

        vec->heap_buf = temp;
        vec->data = vec->heap_buf;
        vec->capacity = cap;
    }

    return 1;
}


short mc_vector_push(vector vec, long value) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    if (!vec_ensure_capacity(vec, 1)) {
        errno = ENOMEM;
        return 0;
    }

    vec->data[vec->count++] = value;
    return 1;
}

long mc_vector_pop(vector vec) {
    if (!vec) {
        errno = EFAULT;
        return LONG_MIN;
    }

    if (vec->count == 0) {
        errno = ENOMEM;
        return LONG_MIN;
    }

    long res = vec->data[--vec->count];
    return res;
}



short mc_vector_insert(vector vec, size_t index, long value) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    if (index >= vec->count) {
        errno = EINVAL;
        return 0;
    }

    if (!vec_ensure_capacity(vec, 1)) {
        errno = ENOMEM;
        return 0;
    }

    if (index == vec->count-1)
        return mc_vector_push(vec, value);

    memmove(vec->data + index + 1, vec->data + index, (vec->count - index) * sizeof (long));

    vec->data[index] = value;
    vec->count++;

    return 1;
}

short mc_vector_inserts(vector vec, size_t index, long *src, size_t length) {
    if (!vec || !src) {
        errno = EFAULT;
        return 0;
    }

    if (index >= vec->capacity || length == 0 || index + length >= vec->capacity) {
        errno = EINVAL;
        return 0;
    }

    if (!vec_ensure_capacity(vec, length)) {
        errno = ENOMEM;
        return 0;
    }


    if (index != vec->count-1) 
        memmove(vec->data + index + length, vec->data + index, (vec->count - index) * sizeof (long));

    memcpy(vec->data + index, src, length * sizeof (long));

    vec->count += length;
    return length;
}




short mc_vector_remove(vector vec, size_t index) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    if (index >= vec->count) {
        errno = EINVAL;
        return 0;
    }

    if (index == vec->count-1)
        return mc_vector_pop(vec) != LONG_MIN;

    memmove(vec->data + index, vec->data + index + 1, (vec->count - index) * sizeof (long));
    vec->count--;

    return 1;
}

short mc_vector_erase(vector vec, size_t index, size_t length) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    if (index >= vec->count || length == 0 || index + length >= vec->count) {
        errno = EINVAL;
        return 0;
    }


    memmove(vec->data + index, vec->data + index + length, (vec->count - index - length) * sizeof (long));

    vec->count -= length;
    return length;
}


short mc_vector_swap(vector vec1, vector vec2) {
    if (!vec1 || !vec2) {
        errno = EFAULT;
        return 0;
    }

    size_t count = vec1->count;
    size_t capacity = vec1->capacity;
    long  *data = vec1->data;

    vec1->count = vec2->count;
    vec1->capacity = vec2->capacity;
    vec1->data = vec2->data;

    vec2->count = count;
    vec2->capacity = capacity;
    vec2->data = data;

    return 1;
}

short mc_vector_fill(vector vec, long value) {
    return mc_vector_fill_range(vec, 0, vec->count, value);
}

short mc_vector_fill_range(vector vec, size_t index, size_t length, long value) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    if (index >= vec->count || length == 0 || index + length > vec->count) {
        errno = EINVAL;
        return 0;
    }

    // cannot use memset here since we use long :(
    for (size_t i = index; i < index + length; i++) {
        vec->data[i] = value;
    }

    return 1;
}



short mc_vector_shrink(vector vec) {
    return mc_vector_resize(vec, vec->count);
}

short mc_vector_resize(vector vec, size_t newSize) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    if (newSize == 0) {
        errno = EINVAL;
        return 0;
    }

    if (newSize <= MC_VECTOR_BUFSIZE) {
        // new size is small enough to fit in the stack buffer
        if (vec->data == vec->heap_buf) {
            // we have to copy the content from the heap buffer and free it
            memcpy(vec->stack_buf, vec->heap_buf, newSize * sizeof (long));
            free(vec->heap_buf);
            vec->heap_buf = NULL;
        }

        vec->data = vec->stack_buf;
        vec->capacity = MC_VECTOR_BUFSIZE;
        vec->count = newSize;
    }
    else {
        // new size is too big for stack buffer
        if (vec->data == vec->heap_buf) {
            // we already have allocated a buffer, just realloc it
            long *temp = (long*)realloc(vec->heap_buf, newSize * sizeof (long));

            if (!temp) {
                errno = ENOMEM;
                return 0;
            }

            vec->heap_buf = temp;
        }
        else {
            // we need to alloc a new buffer, and copy the content from the stack buffer
            long *temp = (long*)(malloc(newSize * sizeof (long)));

            if (!temp) {
                errno = EINVAL;
                return 0;
            }

            memcpy(temp, vec->stack_buf, vec->count * sizeof (long));
            vec->heap_buf = temp;
        }

        // now update attributes
        vec->data = vec->heap_buf;
        vec->capacity = newSize;
    }

    return 1;
}

short mc_vector_clear(vector vec) {
    if (!vec) {
        errno = EFAULT;
        return 0;
    }

    if (vec->data == vec->heap_buf) {
        // free the buffer
        free(vec->heap_buf);
        vec->heap_buf = NULL;
        vec->data = vec->stack_buf;
        vec->capacity = MC_VECTOR_BUFSIZE;
    }

    vec->count = 0;
    return 1;
}




#ifndef MC_VECTOR_NO_IO

int mc_vector_fprint(vector vec, FILE *stream, vector_display option) {
    if (!vec || !stream) {
        errno = EFAULT;
        return 0;
    }

    int res = 0;

    const char *separator = (option == VDisplay_SingleLine) ? ", "
                          : (option == VDisplay_OnePerLine) ? "\n"
                          : (" ");

    if (option == VDisplay_SingleLine)
        res += fprintf(stream, "{", sizeof (char));

    for (size_t i = 0; i < vec->count; i++) {
        res += fprintf(stream, "%lu", vec->data[i], sizeof (long));

        if (i + 1 < vec->count)
            res += fprintf(stream, "%s", separator, sizeof (separator));
    }

    if (option == VDisplay_SingleLine)
        res += fprintf(stream, "}", sizeof (char));

    if (option != VDisplay_Raw)
        res += fprintf(stream, "\n", sizeof (char));

    return res;
}

int mc_vector_sprint(vector vec, char *buffer, int buflen, vector_display option) {
    if (!vec || !buffer) {
        errno = EFAULT;
        return 0;
    }

    if (buflen <= 0) {
        errno = EINVAL;
        return 0;
    }

    int temp, res = 0;
    const char *separator = (option == VDisplay_SingleLine) ? ", "
                          : (option == VDisplay_OnePerLine) ? "\n"
                          : (" ");


    if (option == VDisplay_SingleLine) {
        temp = sprintf(buffer, buflen, "{");
        res += temp;
        buffer += temp;
        buflen -= temp;
    }

    for (size_t i = 0; i < vec->count && buflen > 0; i++) {
        temp = sprintf(buffer, buflen, "%lu", vec->data[i]);

        res += temp;
        buffer += temp;
        buflen -= temp;

        if (buflen > 0 && i + 1 < vec->count) {
            temp = sprintf(buffer, buflen, "%s", separator);
            res += temp;
            buffer += temp;
            buflen -= temp;
        }
    }

    if (buflen >= 0 && option == VDisplay_SingleLine) {
        temp = sprintf(buffer, buflen, "}");
        res += temp;
        buflen += temp;
        buffer += temp;
    }

    if (buflen >= 0 && option != VDisplay_Raw) {
        temp = sprintf(buffer, buflen, "\n");
        res += temp;
    }

    return res;
}


int mc_vector_wprint(vector vec, wchar_t *buffer, int buflen, vector_display option) {
    if (!vec || !buffer) {
        errno = EFAULT;
        return 0;
    }

    if (buflen <= 0) {
        errno = EINVAL;
        return 0;
    }

    int temp, res = 0;
    const wchar_t *separator = (option == VDisplay_SingleLine) ? L", "
                          : (option == VDisplay_OnePerLine) ? L"\n"
                          : (L" ");


    if (option == VDisplay_SingleLine) {
        temp = swprintf(buffer, buflen, L"{");
        res += temp;
        buffer += temp;
        buflen -= temp;
    }

    for (size_t i = 0; i < vec->count && buflen > 0; i++) {
        temp = swprintf(buffer, buflen, L"%lu", vec->data[i]);

        res += temp;
        buffer += temp;
        buflen -= temp;

        if (buflen > 0 && i + 1 < vec->count) {
            temp = swprintf(buffer, buflen, L"%s", separator);
            res += temp;
            buffer += temp;
            buflen -= temp;
        }
    }

    if (buflen >= 0 && option == VDisplay_SingleLine) {
        temp = swprintf(buffer, buflen, L"}");
        res += temp;
        buflen += temp;
        buffer += temp;
    }

    if (buflen >= 0 && option != VDisplay_Raw) {
        temp = swprintf(buffer, buflen, L"\n");
        res += temp;
    }

    return res;
}



#endif 