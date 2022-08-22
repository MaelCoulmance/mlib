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


/*
versions update:
    1.1:    - add macros, typedef and print functions. make vector_s struct opaque.
    1.2:    - add MC_VECTOR_NO_IO guard, add error handling and documentation, remove <stdlib.h> from header,
              remove <stdbool.h> and use shorts instead of bools, add swap and fill functions.
    1.3     - makes 'mc_vector_pop' return the deleted element, add small buffer optimization, rewrite 
              'mc_vector_resize' and make 'mc_vector_shrink' just call it, remove 'mc_pop_several' and 'mc_push_several',
              add 'mc_vector_wprint', use 'fprintf_s' and 'sprintf_s' instead of 'fprintf' and 'sprintf' on _WIN32 platform,
              rewrite documentation.
*/


/**
 * By default, this header includes <stdio.h> and declares functions to print a vector into a file or a string. To disable this,
 * define MC_VECTOR_NO_IO [[BEFORE]] including this file.
 * 
 * By default, this header defines some helper macros, as shortcuts to the long-full-name of each functions. To disable this,
 * define MC_VECTOR_NO_MACROS [[BEFORE]] including this file.
 */

#ifndef MC_VECTOR_H
#define MC_VECTOR_H

#include <stddef.h>

#ifndef MC_VECTOR_NO_IO
#   include <stdio.h>
#endif 

#define MC_VECTOR_BUFSIZE 10

#ifndef MC_VECTOR_NO_MACROS

#define vec()                               mc_vector_make(MC_VECTOR_BUFSIZE)
#define vmake(capacity)                     mc_vector_make(capacity)
#define vmakef(capacity, length, value)     mc_vector_make_filled(capacity, length, value)
#define vclone(vec)                         mc_vector_clone(vec)
#define varray(src, len)                    mc_vector_from_array(src, len)
#define vfree(vec)                          mc_vector_free(vec)

#define vtoarr(vec, buffer)                 mc_vector_to_array(vec, buffer)
#define vextract(vec, buffer, index, len)   mc_vector_extract(vec, buffer, index, len)

#define vget(vec, index, out)               mc_vector_get(vec, index, out)
#define vgetu(vec, index)                   mc_vector_get_unchecked(vec, index)
#define vset(vec, index, value)             mc_vector_set(vec, index, value)
#define vstack(vec)                         mc_vector_is_stack(vec)

#define vsize(vec)                          mc_vector_size(vec)
#define vcapacity(vec)                      mc_vector_capacity(vec)
#define vempty(vec)                         mc_vector_empty(vec)

#define vpush(vec, value)                   mc_vector_push(vec, value)
#define vpop(vec)                           mc_vector_pop(vec)

#define vinsert(vec, index, value)          mc_vector_insert(vec, index, value)
#define vinserts(vec, index, src, len)      mc_vector_inserts(vec, index, src, len)

#define vremove(vec, index)                 mc_vector_remove(vec, index)
#define verase(vec, index, length)          mc_vector_erase(vec, index, length)

#define vswap(v1, v2)                       mc_vector_swap(v1, v2)
#define vfill(vec, value)                   mc_vector_fill(vec, value)
#define vfillr(vec, index, length, value)   mc_vector_fill_range(vec, index, length, value)

#define vshrink(vec)                        mc_vector_shrink(vec)
#define vresize(vec, newSize)               mc_vector_resize(vec, newSize)
#define vclear(vec)                         mc_vector_clear(vec)

#ifndef MC_VECTOR_NO_IO

#define vfprint(vec, stream)                mc_vector_fprint(vec, stream, VDisplay_SingleLine)
#define vsprint(vec, buf, buflen)           mc_vector_sprint(vec, buf, buflen, VDisplay_SingleLine)
#define vwprint(vec, buf, buflen)           mc_vector_wprint(vec, buf, buflen, VDisplay_SingleLine)

#define vfprint2(vec, stream, option)       mc_vector_fprint(vec, stream, option)
#define vsprint2(vec, buf, buflen, option)  mc_vector_sprint(vec, buf, buflen, option)
#define vwprint2(vec, buf, buflen, option)  mc_vector_wprint(vec, buf, buflen, option)

#define vprint(vec)                         mc_vector_fprint(vec, stdout, VDisplay_SingleLine)

#endif /* MC_VECTOR_NO_IO */

#endif /* MC_VECTOR_NO_MACROS */


// Forward declaration of the vector struct
typedef struct vector_s * vector;





/**
 * @brief Creates a new vector, with given capacity
 * 
 * @param[in] capacity  The capacity of the resulting vector
 *  
 * @return A pointer to a new vector if operation succeeded, NULL otherwise.
 * 
 * @note   If given capacity is not valid (i.e capacity == 0), [errno] will be set to @c EINVAL
 *         If allocation fails, [errno] will be set to @c ENOBUFS 
 */
vector mc_vector_make(size_t capacity);

/**
 * @brief Creates a new vector, filled with a given value
 * 
 * @param[in] capacity  The internal capacity of the resulting vector
 * @param[in] length    The size of the resulting vector
 * @param[in] value     An integer
 * 
 * @return A pointer to a new vector if operation succeded, NULL otherwise
 * 
 * @note   If capacity or length are invalid (i.e capacity == 0 or capacity < length), [errno] will be set to @c EINVAL
 *         If allocation fails, [errno] will be set to @c ENOBUFS   
 */
vector mc_vector_make_filled(size_t capacity, size_t length, long value);

/**
 * @brief Creates a copy of a given vector
 * 
 * @param[in] vec  The vector to be cloned
 * 
 * @return A pointer to a new vector if operation succeeded, NULL otherwise.
 * 
 * @note   If given pointer is NULL, [errno] will be set to @c EFAULT
 *         If allocation fails, [errno]  will be set to @c ENOBUFS  
 */
vector mc_vector_clone(vector vec);

/**
 * @brief Creates a vector representation of a given array
 * 
 * @param[in] src       A pointer to an array of long integers
 * @param[in] length    The size of the array
 * 
 * @return A pointer to a new vector if operation succeeded, NULL otherwise.
 * 
 * @note   If given pointer is NULL, [errno] will be set to @c EFAULT
 *         If given length is invalid (i.e. length == 0), [errno] will be set to @c EINVAL
 *         If allocation fails, [errno] will be set to @c ENOBUFS  
 */
vector mc_vector_from_array(long *src, size_t length);

/**
 * @brief Destroys a given vector. Note that if given pointer is NULL, this function has no effect
 * 
 * @param[inout] vec    The vector to be destroyed 
 */
void   mc_vector_free(vector vec);




/**
 * @brief Stores the content of the vector into a given array
 * 
 * @param[in]  vec      The vector to be copied 
 * @param[out] buffer   The output array
 * 
 * @return The number of elements that has been copied into the array
 * 
 * @note   If either vector or array is NULL, [errno] will be set to @c EFAULT  
 */
int mc_vector_to_array(vector vec, long *buffer);

/**
 * @brief Extracts a range of elements on this vector and stores it into a given array
 * 
 * @param[in]  vec          The vector to be copied
 * @param[out] buffer       The output array
 * @param[in]  startIndex   The position of the first element to be copied
 * @param[in]  length       The number of elements to copy
 * 
 * @return The number of elements that has been copied into the output array
 * 
 * @note   If either vector or array is NULL, [errno] will be set to @c EFAULT
 *         If index is out of range or if length is invalid (i.e. length == 0), [errno] will be set to @c EINVAL  
 */
int mc_vector_extract(vector vec, long *buffer, size_t startIndex, size_t length);





/**
 * @brief Gets an element from the vector
 * 
 * @param[in]  vec      The vector to be read
 * @param[in]  index    The position of the element on the vector
 * @param[out] out      A pointer to an integer where the extracted value will be stored
 * 
 * @return 1 if operation succeeded, 0 otherwise
 * 
 * @note   If given pointer (vec or out) is NULL, [errno] will be set to @c EFAULT
 *         If given index is out of range, [errno] will be set to @c EINVAL 
 */
short mc_vector_get(vector vec, size_t index, long *out);

/**
 * @brief Gets an element from the vector, without checking the result
 * 
 * @param[in] vec       The vector to be read
 * @param[in] index     The position of the element on the vector
 * 
 * @return The element located at index if operation succeeded, undefined otherwise
 */
long  mc_vector_get_unchecked(vector vec, size_t index);

/**
 * @brief Sets the value of an element on the vector
 * 
 * @param[inout] vec     The vector to be modified
 * @param[in]    index   The position of the element
 * @param[in]    value   The new value of the element
 * 
 * @return 1 if operation succeeded, 0 otherwise
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT
 *         If given index is out of range, [errno] will be set to @c EINVAL  
 */
short mc_vector_set(vector vec, size_t index, long value);





/**
 * @brief The size of the vector
 * 
 * @param[in] vec A vector
 * 
 * @return The number of elements currently stored on the vector
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT and 0 will be returned  
 */
size_t mc_vector_size(vector vec);

/**
 * @brief The capacity of the vector
 * 
 * @param[in] vec A vector
 * 
 * @return The internal capacity of the vector
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT and 0 will be returned 
 */
size_t mc_vector_capacity(vector vec);

/**
 * @brief Whether the vector is empty
 * 
 * @param[in] vec A vector
 * 
 * @return 1 if the vector currently contains no elements, 0 otherwise
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT and 0 will be returned 
 */
short  mc_vector_empty(vector vec);

/**
 * @brief Whether the internal buffer is allocated on the stack
 * 
 * @param[in] vec  The vector to be checked
 *  
 * @return 1 if elements are currently stored on a stack buffer, 0 if they are stored
 *         on a heap buffer 
 * 
 * @note   If vector is NULL, [errno] will be set to @c EFAULT and 0 will be returned  
 */
short mc_vector_is_stack(vector vec);



/**
 * @brief Inserts an element at the end of the vector
 * 
 * @param[inout] vec    The vector to be modified 
 * @param[in]    value  The value to be inserted
 * 
 * @return 1 if operation succeeded, 0 otherwise
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT
 *         If internal buffer is full, and reallocation failed, [errno] will be set to @c ENOMEM and
 *           the vector will still be usable (except value has not been inserted)   
 */
short mc_vector_push(vector vec, long value);

/**
 * @brief Removes the last element of the vector
 * 
 * @param[inout] vec  The vector to be modified
 * 
 * @return The element that has just been removed from the vector, undefined if vector is NULL or empty.
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT
 *         If given vector is empty, [errno] will be set to @c ENOMEM  
 */
long  mc_vector_pop(vector vec);





/**
 * @brief Inserts an element in the vector
 * 
 * @param[inout] vec    The vector to be modified
 * @param[in]    index  The position where the element will be inserted
 * @param[in]    value  The value to be inserted
 * 
 * @return 1 if operation succeeded, 0 otherwise 
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT
 *         If given index is out of range, [errno] will be set to @c EINVAL 
 *         If vector is full and reallocation failed, [errno] will be set to @c ENOMEM
 *           and vector will still be usable (except value won't be inserted)  
 */
short mc_vector_insert(vector vec, size_t index, long value);

/**
 * @brief Inserts an array of elements in the vector
 * 
 * @param[inout] vec     The vector to be modified
 * @param[in]    index   The position where the first element will be inserted
 * @param[in]    src     A pointer to an array of long integers
 * @param[in]    length  The size of the array (aka the number of elements to be inserted)
 * 
 * @return 1 if operation succeded, 0 otherwise
 * 
 * @note   If given vector or array is NULL, [errno] will be set to @c EFAULT
 *         If given index is out of range, or length is invalid (i.e length == 0), [errno] will be set to @c EINVAL
 *         If vector is full and reallocation failed, [errno] will be set to @c ENOMEM
 *           and vector will still be usable (except array won't be inserted)   
 */
short mc_vector_inserts(vector vec, size_t index, long *src, size_t length);





/**
 * @brief Removes an element from the vector
 * 
 * @param[inout] vec    The vector to be modified
 * @param[in]    index  The position of the element to be removed
 * 
 * @return 1 if operation succeeded, 0 otherwise
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT
 *         If given index is out of range, or if length is invalid (i.e length == 0), [errno] will be set to @c EINVAL   
 */
short mc_vector_remove(vector vec, size_t index);

/**
 * @brief Removes a range of element from the vector
 * 
 * @param[inout] vec            The vector to be modified
 * @param[in]    startIndex     The position of the first element to be removed
 * @param[in]    length         The number of element to remove
 * 
 * @return The number of element that has been removed from the vector 
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT
 *         If given index is out of range or if length is invalid (i.e length == 0), [errno] will be set to @c EINVAL 
 */
short mc_vector_erase(vector vec, size_t startIndex, size_t length);





/**
 * @brief Swaps the content of two vectors
 * 
 * @param[inout] vec1  A vector
 * @param[inout] vec2  Another vector
 * 
 * @return 1 if operation succeeded, 0 otherwise
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT  
 */
short mc_vector_swap(vector vec1, vector vec2);

/**
 * @brief Fills the vector with a given value
 * 
 * @param[inout] vec     The vector to be modified
 * @param[in]    value   An integer
 * 
 * @return 1 if operation succeeded, 0 otherwise
 * 
 * @note   If vector is NULL, [errno] will be set to @c EFAULT 
 */
short mc_vector_fill(vector vec, long value);

/**
 * @brief Fills the content of a given range on the vector with a given value
 * 
 * @param[inout] vec            The vector to be modified
 * @param[in]    startIndex     The position of the first element of the range
 * @param[in]    length         The number of elements on the range
 * @param[in]    value          An integer
 * 
 * @return 1 if operation succeded, 0 otherwise
 * 
 * @note   If vector is NULL, [errno] will be set to @c EFAULT
 *         If given index out of range or if length is invalid (i.e. length == 0), [errno] will be set to @c EINVAL  
 */
short mc_vector_fill_range(vector vec, size_t startIndex, size_t length, long value);




/**
 * @brief Shrinks the internal buffer capacity to fit its size. Every unused memory "blocks" will be removed from the vector,
 *        which will cause its capacity to be equal to its size.
 * 
 * @param[inout] vec  The vector to be modified
 *  
 * @return 1 if operation succeeded, 0 otherwise
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT
 *         If reallocation failed, [errno] will be set to @c ENOMEM 
 *            and vector will still be usable (except capacity won't be modified)  
 */
short mc_vector_shrink(vector vec);

/**
 * @brief Resizes the internal array. If given capacity is bigger than current capacity, new element won't be initialized
 *        and [size] won't be modified. If given capacity is smaller than current capacity, every element located outside
 *        of the new buffer will be lost. Else, nothing happends.
 * 
 * @param[inout] vec      The vector to be modified
 * @param[in]    newSize  The new capacity of the vector
 *  
 * @return 1 if operation succeeded, 0 otherwise
 * 
 * @note   If given vector is NULL, [errno] will be set to @c EFAULT
 *         If given size is invalid (i.e. newSize == 0), [errno] will be set to @c EINVAL
 *         If reallocation failed, [errno] will be set to @c ENOMEM
 *           and vector will still be usable (except capacity won't be modified)   
 */
short mc_vector_resize(vector vec, size_t newSize);

/**
 * @brief Clears the content of the vector
 * 
 * @param[inout] vec  The vector to be cleaned
 *  
 * @return 1 if operation succeeded, 0 otherwise
 * 
 * @note   If vector is NULL, [errno] will be set to @c EFAULT
 */
short mc_vector_clear(vector vec);



#ifndef MC_VECTOR_NO_IO

typedef enum {
    VDisplay_SingleLine     = 0x0000,   // Displays element on one line, adding brackets before first and after last element, line feed at the end
    VDisplay_OnePerLine     = 0x0001,   // Displays one element per line, adding line feed at the end
    VDisplay_Raw            = 0x0002    // Display every element on the same line, without adding anything
} vector_display;


/**
 * @brief Prints a string representation of the vector into a given file, using the standard fprintf function.
 * 
 * @param[in] vec      The vector to be printed
 * @param[in] stream   A pointer to a file struct
 * @param[in] option   The display option (see vector_display for details) 
 * 
 * @return The number of character that has been printed into the file.
 * 
 * @note   If vector is NULL, [errno] will be set to @c EFAULT
 *         If printing fails, see the documentation of your C Library implementation for details   
 */
int mc_vector_fprint(vector vec, FILE *stream, vector_display option);

/**
 * @brief Prints a string representation of the vector into a given string, using the standard sprintf function
 * 
 * @param[in] vec           The vector to be printed
 * @param[in] buffer        A character string
 * @param[in] bufferLen     The size of the string
 * @param[in] option        The display option (see vector_display for details)
 * 
 * @return The number of characters that has been printed into the string
 * 
 * @note   If vector is NULL, [errno] will be set to @c EFAULT  
 *         If buflen is invalid (i.e buflen < 0), [errno] will be set to @c EINVAL
 *         If printing fails, see the documentation of your C Library implementation for details
 * 
 *         Note that if you compile this code on Windows, we will use the 'sprint_f' function, but on Linux
 *         we use the classic 'sprintf' function. Although we check for memory overflow before writing any data,
 *         this makes this function less safe on Linux. Be careful when using it.  
 */
int mc_vector_sprint(vector vec, char *buffer, int bufferLen, vector_display option);

/**
 * @brief Prints a string representation of the vector into a given multibyte string, using the standard swprintf function
 * 
 * @param[in] vec           The vector to be printed
 * @param[in] buffer        A multibyte character string
 * @param[in] bufferLen     The size of the string
 * @param[in] option        The display option (see vector_display for details)
 * 
 * @return The number of characters that has been printed into the string
 * 
 * @note   If vector is NULL, [errno] will be set to @c EFAULT
 *         If buflen is invalid (i.e buflen < 0), [errno] will be set to @c EINVAL
 *         If printing fails, see the documentation of your C Library implementation for details   
 */
int mc_vector_wprint(vector vec, wchar_t *buffer, int bufferLen, vector_display option);


#endif /* MC_VECTOR_NO_IO */

#endif /* Header Guard */