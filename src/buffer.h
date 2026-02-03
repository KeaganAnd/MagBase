//     Keagan Anderson
//        MagBase
//       01/08/2026

#pragma once

#include "db-init.h"
#include <stdio.h>

BufferPool *createBufferPool();
int freeBufferPool(BufferPool *buffer);
int flushPage(BufferPool *buffer, MagBase *db, int pageIndex);
int addToBuffer(BufferPool *buffer, size_t pageId, char *pageData, MagBase *db);

// Read a page from buffer (or disk if not cached)
// Returns pointer to page data in buffer, or NULL on error
char *readPageFromBuffer(BufferPool *buffer, size_t pageId, FILE *file_pointer, size_t page_size);

// Mark a page as dirty (modified) in the buffer
// Returns 0 on success, -1 on error
int markPageDirty(BufferPool *buffer, size_t pageId);

// Flush all dirty pages in the buffer to disk
// Returns 0 on success, -1 on error
int flushAllDirtyPages(BufferPool *buffer, MagBase *db);
