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

int addToBuffer(BufferPool *buffer, size_t pageId, char *pageData, MagBase *db) {
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
            flushPage(buffer, db, BUFFER_SIZE - 1);
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

char *readPageFromBuffer(BufferPool *buffer, size_t pageId, FILE *file_pointer, size_t page_size) {
    if (!buffer || !file_pointer) {
        return NULL;
    }

    // Check if page is already in buffer
    for (int i = 0; i < buffer->num_pages; i++) {
        if (buffer->page_ids[i] == pageId) {
            return buffer->pages[i];
        }
    }

    // Page not in buffer, read from disk
    char *page_data = malloc(page_size);
    if (!page_data) {
        return NULL;
    }

    // Seek to page location
    if (fseek(file_pointer, pageId * page_size, SEEK_SET) != 0) {
        free(page_data);
        return NULL;
    }

    // Read page data
    if (fread(page_data, page_size, 1, file_pointer) != 1) {
        // If read fails, initialize with zeros (new page)
        memset(page_data, 0, page_size);
    }

    // Add to buffer (don't pass MagBase, just track in buffer)
    if (buffer->num_pages >= BUFFER_SIZE) {
        // Buffer full, evict oldest
        if (buffer->dirty_flags[BUFFER_SIZE - 1]) {
            // In real implementation, would flush to disk
            // For now just discard
        }
        buffer->num_pages--;
    }

    // Shift existing pages and add new one at front
    memmove(&buffer->pages[1], &buffer->pages[0], sizeof(char *) * buffer->num_pages);
    memmove(&buffer->page_ids[1], &buffer->page_ids[0], sizeof(size_t) * buffer->num_pages);
    memmove(&buffer->dirty_flags[1], &buffer->dirty_flags[0], sizeof(int) * buffer->num_pages);

    buffer->pages[0] = page_data;
    buffer->page_ids[0] = pageId;
    buffer->dirty_flags[0] = 0;
    buffer->num_pages++;

    return buffer->pages[0];
}

int markPageDirty(BufferPool *buffer, size_t pageId) {
    if (!buffer) {
        return -1;
    }

    for (int i = 0; i < buffer->num_pages; i++) {
        if (buffer->page_ids[i] == pageId) {
            buffer->dirty_flags[i] = 1;
            return 0;
        }
    }

    return -1;  // Page not found in buffer
}
