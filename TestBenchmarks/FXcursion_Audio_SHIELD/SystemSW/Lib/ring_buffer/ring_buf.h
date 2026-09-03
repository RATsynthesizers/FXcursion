
#ifndef RINGBUF_H
#define RINGBUF_H



/* Module includes */

// Get EHAL general types
#include "general.h"



/* Module data types */

// Type of ring buffer size
typedef uint16_t ringbuf_size_t;

// Type of the structure of a ring buffer
typedef struct ringbuf
{
    // Pointer to data of the buffer
    const void* p_data;
    // Item size in bytes
    uint8_t item_size;
    // Size of the buffer in items
    ringbuf_size_t size;
    // Head of the buffer, where data is prepended
    ringbuf_size_t head;
    // Tail of the buffer, where data is removed from
    ringbuf_size_t tail;

} ringbuf_t;

// Errors returned by ring buffer API
typedef enum
{
    // No error occurred
    RINGBUF_NO_ERR = 0U,
    // Buffer is empty
    RINGBUF_EMPTY = 1U,
    // Buffer overflow
    RINGBUF_OVERFLOW = 2U,
    // Size of the buffer is too small
    RINGBUF_SIZE_TOO_SMALL = 3U,
    // Buffer does not exist
    RINGBUF_NOT_EXIST = 4U,
    // Data of the buffer do not exist
    RINGBUF_DATA_NOT_EXIST = 5U,
    // Input parameter error
    RINGBUF_INPUT_PARAM_ERROR = 6U

} ringbuf_error_t;



/* Module Interface */

// Initializes a ring buffer
extern ringbuf_error_t ringbuf_init(ringbuf_t* const p_ringbuf,
                                    const void* const p_ring_data_buf,
                                    const ringbuf_size_t data_buf_size);

// Puts one item into a ring buffer
extern ringbuf_error_t ringbuf_put_data(ringbuf_t* const p_ringbuf,
                                        const void* const p_ring_data);

// Gets and removes one item from a ring buffer
extern ringbuf_error_t ringbuf_get_data(ringbuf_t* const p_ringbuf,
                                        const void* const p_ring_data);

// Cleans a ring buffer
extern ringbuf_error_t ringbuf_purge(ringbuf_t* const p_ringbuf);

// Calculates the number of items in a ring buffer
extern ringbuf_size_t ringbuf_get_number_of_items(const ringbuf_t* const p_ringbuf);

// Calculates the free number of elements in a ring buffer
extern ringbuf_size_t ringbuf_get_free_size(const ringbuf_t* const p_ringbuf);



#endif // #ifndef RINGBUF_H
