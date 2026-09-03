
/*  Module includes */

// Native header
#include "ring_buf.h"



/*  Module constants */

// The minimum size of a ring buffer in items
#define RINGBUF_SIZE_MIN  (2U)



/* Global functions implementation */

/**
 * @fn     ringbuf_init(ringbuf_t* const, const void* const, const ringbuf_size_t)
 * 
 * @brief  Initializes a ring buffer
 * 
 * @note   This function is not reentrant.
 * 
 * @param  p_ringbuf - pointer to a ring buffer item
 * @param  p_ring_data_buf - pointer to data buffer for ring buffer
 * @param  data_buf_size - quantity of items in ring buffer
 * 
 * @return RINGBUF_NOT_EXIST - buffer does not exist;
 *         RINGBUF_DATA_NOT_EXIST - data of the buffer do not exist;
 *         RINGBUF_SIZE_TOO_SMALL - size of the buffer is too small;
 *         RINGBUF_NO_ERR - no error occurred.
*/
ringbuf_error_t ringbuf_init(ringbuf_t* const p_ringbuf,
                             const void* const p_ring_data_buf,
                             const ringbuf_size_t data_buf_size)
{
    if (NULL == p_ringbuf)
    {
        return RINGBUF_NOT_EXIST;
    }

    if (NULL == p_ring_data_buf)
    {
        return RINGBUF_DATA_NOT_EXIST;
    }

    if (data_buf_size < RINGBUF_SIZE_MIN)
    {
        return RINGBUF_SIZE_TOO_SMALL;
    }

    p_ringbuf->p_data = p_ring_data_buf;
    p_ringbuf->size = data_buf_size;
    p_ringbuf->head = data_buf_size - 1U;
    p_ringbuf->tail = data_buf_size - 1U;

    return RINGBUF_NO_ERR;
}



/**
 * @fn     ringbuf_error_t ringbuf_put_data(ringbuf_t* const, const void* const)
 * 
 * @brief  Puts one item into a ring buffer.
 * 
 * @note   This function is not reentrant.
 * 
 * @param  p_ringbuf - pointer to a buffer
 * @param  p_ring_data - pointer to data that will be put into the ring buffer
 * 
 * @return RINGBUF_NOT_EXIST - buffer does not exist;
 *         RINGBUF_DATA_NOT_EXIST - data of the buffer do not exist;
 *         RINGBUF_OVERFLOW - buffer overflow;
 *         RINGBUF_NO_ERR - no error occurred.
*/
ringbuf_error_t ringbuf_put_data(ringbuf_t* const p_ringbuf,
                                 const void* const p_ring_data)
{
    ringbuf_size_t headPlus = 0U;
    uint8_t i = 0U;

    // Check if the item is available
    if (NULL == p_ring_data)
    {
        return RINGBUF_DATA_NOT_EXIST;
    }

    // Check if the buffer is available
    if (NULL == p_ringbuf)
    {
        return RINGBUF_NOT_EXIST;
    }

    headPlus = 1U + p_ringbuf->head;

    // If the head is reached the size, loopback the buffer
    if (headPlus == p_ringbuf->size)
    {
        headPlus = 0U;
    }

    // Check if free space is present in the buffer
    if (headPlus == p_ringbuf->tail)
    {
        return RINGBUF_OVERFLOW;
    }

    // Put one item into the head of the buffer
    for (i = 0U ; i < p_ringbuf->item_size; i++)
    {
        *(((uint8_t*)p_ringbuf->p_data + (headPlus * p_ringbuf->item_size) + i)) = *((uint8_t*)p_ring_data + i);
    }

    // Set the next value of the head
    p_ringbuf->head = headPlus;

    return RINGBUF_NO_ERR;
}



/**
 * @fn     ringbuf_error_t ringbuf_get_data(ringbuf_t* const, const void* const)
 * 
 * @brief  Gets one item from a ring buffer.
 * 
 * @note   This function is not reentrant.
 * 
 * @param  p_ringbuf - pointer to a buffer
 * @param  p_ring_data - pointer to the place where item that extracted from the buffer
 * 
 * @return RINGBUF_NOT_EXIST - buffer does not exist;
 *         RINGBUF_DATA_NOT_EXIST - data of the buffer do not exist;
 *         RINGBUF_EMPTY - buffer is empty;
 *         RINGBUF_NO_ERR - no error occurred.
*/
ringbuf_error_t ringbuf_get_data(ringbuf_t* const p_ringbuf,
                                 const void* const p_ring_data)
{
    ringbuf_size_t tailPlus = 0U;
    uint8_t i = 0U;

    // Check if the item is available
    if (NULL == p_ring_data)
    {
        return RINGBUF_DATA_NOT_EXIST;
    }

    // Check if the buffer is available
    if (NULL == p_ringbuf)
    {
        return RINGBUF_NOT_EXIST;
    }

    tailPlus = 1U + p_ringbuf->tail;

    // Check if data is present in the buffer
    if (p_ringbuf->tail == p_ringbuf->head)
    {
        return RINGBUF_EMPTY;
    }

    // If the tail is reached the size, loopback the buffer
    if (tailPlus == p_ringbuf->size)
    {
        tailPlus = 0U;
    }

    // Get the next item from the tail of the buffer
    for (i = 0U ; i < p_ringbuf->item_size; i++)
    {
        *(((uint8_t*)p_ring_data + i)) = *(((uint8_t*)p_ringbuf->p_data + (tailPlus * p_ringbuf->item_size) + i));
    }
    
    // Set the next value of the tail
    p_ringbuf->tail = tailPlus;

    return RINGBUF_NO_ERR;
}


/**
 * @fn     ringbuf_error_t ringbuf_purge(ringbuf_t* const)
 * 
 * @brief  Cleans a ring buffer.
 * 
 * @note   This function is not reentrant.
 * 
 * @param  p_ringbuf - pointer to a buffer
 * 
 * @return RINGBUF_NOT_EXIST - buffer does not exist;
 *         RINGBUF_NO_ERR - no error occurred. 
*/
ringbuf_error_t ringbuf_purge(ringbuf_t* const p_ringbuf)
{
    // Check if the buffer is available
    if (NULL == p_ringbuf)
    {
        return RINGBUF_NOT_EXIST;
    }

    // Clean the buffer
    p_ringbuf->tail = p_ringbuf->head;

    return RINGBUF_NO_ERR;
}



/**
 * @fn     ringbuf_size_t ringbuf_get_number_of_items(const ringbuf_t* const)
 * 
 * @brief  Calculates the number of items in a ring buffer.
 * 
 * @note   This function is not reentrant.
 * 
 * @param  p_ringbuf - pointer to a buffer
 * 
 * @return 0 - buffer does not exist or empty;
 *         [1..65534] - number of busy elements in a buffer
*/
ringbuf_size_t ringbuf_get_number_of_items(const ringbuf_t* const p_ringbuf)
{
    int32_t busy_elements_qty = 0UL;

    // Check if the buffer is available
    if (NULL == p_ringbuf)
    {
        return 0U;
    }

    busy_elements_qty = (int32_t)((int32_t)p_ringbuf->head - (int32_t)p_ringbuf->tail);

    if (busy_elements_qty < 0)
    {
        busy_elements_qty += p_ringbuf->size;
    }

    return (ringbuf_size_t)busy_elements_qty;
}



/**
 * @fn     ringbuf_size_t ringbuf_get_free_size(const ringbuf_t* const)
 * 
 * @brief  Calculates the free number of elements in a ring buffer.
 * 
 * @note   This function is not reentrant.
 * 
 * @param  p_ringbuf - pointer to a buffer
 * 
 * @return 0 - buffer does not exist or empty;
 *         [1..65534] - number of free elements in a buffer
*/
ringbuf_size_t ringbuf_get_free_size(const ringbuf_t* const p_ringbuf)
{
    int32_t busy_elements_qty = 0UL;

    if (0U == p_ringbuf)
    {
        return 0U;
    }

    busy_elements_qty = (int32_t)((int32_t)p_ringbuf->head - (int32_t)p_ringbuf->tail);

    if (busy_elements_qty < 0)
    {
        busy_elements_qty += p_ringbuf->size;
    }

    return (ringbuf_size_t)((p_ringbuf->size - (ringbuf_size_t)busy_elements_qty) - 1U);
}
