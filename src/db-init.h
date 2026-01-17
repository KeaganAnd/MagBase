//     Keagan Anderson
//        MagBase
//       01/08/2026

#pragma once

#include "globals.h"
#include "structs/bufferStruct.h"
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
    char magic[MAGIC_LENGTH]; // The signature to confirm the file is a magdb file
    Version version;          // the version num, duh
    uint32_t page_size;       // size of a page (currently 4096)
    uint64_t page_count;      // total pages
    uint64_t schema_root;     // page number of the schema table
    uint64_t free_list_head;  // first free page
} Header;

typedef struct {
    char *filePath;
    FILE *file_pointer;
    Header *header;
    BufferPool *buffer_pool;
    size_t page_size;
} MagBase;

typedef struct {
    uint16_t slot_count;
    uint16_t free_space_offset;
    uint64_t next_page;
} PageHeader;

int freeDatabase(MagBase *magBase);
MagBase *createMagBase(Header *header, char path[], bool newFile);
int writeHeader(MagBase *magBase);
Header *createHeader(Header *newHeader);
char *appendFileExt(char *path);

extern Version version;
