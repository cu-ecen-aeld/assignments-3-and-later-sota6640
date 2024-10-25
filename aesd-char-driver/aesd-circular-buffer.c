/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    size_t acc, count;
    size_t offset_curr = buffer->out_offs;
    acc = 0;
    count = 0;

    if (buffer == NULL) return NULL;

    while(count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        if(char_offset < acc + buffer->entry[offset_curr].size) 
        {
            *entry_offset_byte_rtn = char_offset-acc;
            return &(buffer->entry[offset_curr]);
        }

        acc += buffer->entry[offset_curr].size;
        offset_curr = (offset_curr + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        count++;

        if(offset_curr == buffer->in_offs){
            break;
        }
    }

    

    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
* @return NULL or, if an existing entry at out_offs was replaced, the value of buffptr for the entry which
* was replaced (for dynamic allocation / free for Assignment 8)
*/
const char *aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    char *retbuffptr = NULL;
    if(buffer->full) //oldest element is about to be replaced
    {
        //retbuffptr = malloc(buffer->entry[buffer->out_offs].size);
        retbuffptr = buffer->entry[buffer->out_offs].buffptr;
        //no need to malloc here, it's done in main
        //strcpy(retbuffptr, buffer->entry[buffer->out_offs].buffptr);
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
    buffer->entry[buffer->in_offs].size = add_entry->size;
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    buffer->full = (buffer->out_offs == buffer->in_offs);
    return retbuffptr;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
