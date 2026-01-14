//     Keagan Anderson
//        MagBase
//       01/08/2026

#include "buffer.h"
#include "db-init.h"
#include "globals.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BufferPool *createBufferPool() {
    BufferPool *buffer = malloc(sizeof(BufferPool));
    buffer->num_pages = 0;
    buffer->pages = malloc(sizeof(char *) * BUFFER_SIZE);
    buffer->page_ids = malloc(sizeof(size_t) * BUFFER_SIZE);
    buffer->dirty_flags = malloc(sizeof(int) * BUFFER_SIZE);

    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer->pages[i] = malloc(PAGE_SIZE);
        buffer->dirty_flags[i] = 0;
    }

    return (buffer);
}

int freeBufferPool(BufferPool *buffer) {

    if (!buffer) {
        return 0;
    }

    free(buffer->dirty_flags);
    free(buffer->page_ids);

    for (int i = 0; i < buffer->num_pages; i++) {
        free(buffer->pages[i]);
    }

    free(buffer->pages);
    free(buffer);

    return 0;
}

int flushPage(BufferPool *buffer, MagBase *db, int pageIndex) {

    if (!buffer || !db || pageIndex < 0 ||
        pageIndex >= buffer->num_pages) // Make sure all parameters are valid
        return -1;

    if (fseek(db->file_pointer, buffer->page_ids[pageIndex] * PAGE_SIZE, SEEK_SET) != 0) {
        fprintf(stderr, "Failed to seek while flushing file");
    }
    if (fwrite(buffer->pages[pageIndex], PAGE_SIZE, 1, db->file_pointer) != 0) {
        fprintf(stderr, "Failed to write while flushing file");
    }

    fflush(db->file_pointer); // Ensure OS wrote file
    buffer->dirty_flags[pageIndex] = 0;
    return 0;
}

int addToBuffer(BufferPool *buffer, size_t pageId, char *pageData) {
    if (!buffer) {
        return 0;
    }

    size_t positionInBuffer = -1; // The loop below checks the buffer to see if the page id
                                  // already exists, if it doesn't the value stays -1
    for (int i = 0; i < buffer->num_pages; i++) {
        if (buffer->page_ids[i] == pageId) { // If the page is in the buffer, move it to the front
            positionInBuffer = pageId;
        }
    }

    if (positionInBuffer != -1) { // If in buffer move to front
        char *temp = buffer->pages[positionInBuffer];
        memmove(&buffer->pages[1], &buffer->pages[0], sizeof(char *) * positionInBuffer);
        buffer->pages[0] = temp;
        buffer->dirty_flags[0] = 1;
        return 0;
    }

    if (buffer->num_pages == BUFFER_SIZE) { // If the buffer is full, then flush oldest
        if (buffer->dirty_flags[BUFFER_SIZE - 1]) {
            // TODO Write the page and flush it
        }
        buffer->num_pages--;
    }

    memmove(&buffer->pages[1], &buffer->pages[0],
            sizeof(char *) * buffer->num_pages); // Shifts down all the pointers in memory the
                                                 // size of a char pointer
    memcpy(buffer->pages[0], pageData,
           PAGE_SIZE); // Copies the data of the pageData to the first slot in the array
    buffer->page_ids[0] = pageId;
    buffer->dirty_flags[0] = 1;
    buffer->num_pages++;

    return 0;
}
