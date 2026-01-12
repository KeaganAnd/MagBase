//     Keagan Anderson
//        MagBase
//       01/08/2026

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

char *getHelpContent(void);
int createDatabase(char *path);
bool checkIfFileExists(char *path);

typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} Version;

typedef struct {
    char magic[8];           // The signature to confirm the file is a magdb file
    Version version;         // the version num, duh
    uint32_t page_size;      // size of a page (currently 4096)
    uint64_t page_count;     // total pages
    uint64_t schema_root;    // page number of the schema table
    uint64_t free_list_head; // first free page
} Header;

typedef struct {
    char **pages;
    size_t *page_ids;
    int num_pages;
    int *dirty_flags;
} BufferPool;

typedef struct {
    char *filePath;
    FILE *file_pointer;
    Header *header;
    BufferPool *buffer_pool;
    size_t page_size;
} MagBase;

int freeDatabase(MagBase *magBase);
MagBase *createMagBase(Header *header, char path[]);
int writeHeader(MagBase *magBase);
Header *createHeader(Header *newHeader);
char *appendFileExt(char *path);
