//     Keagan Anderson
//        MagBase
//       01/08/2026

#pragma once

#include <stdio.h>

typedef struct {
    char **pages;     // An array of 4096 char bytes each storing a page
    size_t *page_ids; // An parallel array to pages with the id of the page
    int num_pages;    // The number of pages loaded
    int *dirty_flags; // Dirty means the file is modified in memory and not written to disk
} BufferPool;         // Logic needs implementation
