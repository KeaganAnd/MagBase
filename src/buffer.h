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
