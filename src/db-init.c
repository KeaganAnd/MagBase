//     Keagan Anderson
//        MagBase
//       01/08/2026

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "db-init.h"
#include "globals.h"

char *getHelpContent(void) {
    FILE *filePtr;

    filePtr = fopen("../docs/help-pages.txt", "r");

    if (!filePtr) {
        return ("Hm... The help file is missing? Why'd you delete that.\nYou can get a new one "
                "from the github "
                "repo:\nhttps://github.com/KeaganAnd/MagBase/blob/main/docs/help-page.txt\n");
    }

    fseek(filePtr, 0, SEEK_END);

    long size = ftell(filePtr);
    if (size < 0) {
        fclose(filePtr);
        return "Failed to read help page\n";
    }

    rewind(filePtr);

    char *fileContents = malloc(size + 1);
    if (!fileContents) {
        fclose(filePtr);
        return "Failed to allocate memory for the help file\n";
    }

    fread(fileContents, 1, size, filePtr);
    fileContents[size] = '\0';

    fclose(filePtr);
    return (fileContents);
}

bool checkIfFileExists(char *path) {
    FILE *filePtr;
    filePtr = fopen(path, "r");
    if (!filePtr) {
        return false;
    }
    fclose(filePtr);
    return true;
}

Header *createHeader(Header *newHeader) {
    memcpy(newHeader->magic, MAGIC, MAGIC_LENGTH);
    newHeader->version = version;
    newHeader->page_count = 2;
    newHeader->page_size = PAGE_SIZE;
    newHeader->schema_root = 1;
    newHeader->free_list_head = 2;

    return (newHeader);
}

int writeHeader(MagBase *magBase) {
    fseek(magBase->file_pointer, 0, SEEK_SET);
    fwrite(magBase->header, sizeof(Header), 1, magBase->file_pointer);
    fflush(magBase->file_pointer);
    return 0;
}

char *appendFileExt(char *path) {
    // DEPRECATED: Use local buffer handling in caller instead
    // This function is kept for backwards compatibility with -p command
    static char buffer[512];  // Large enough for most paths
    static int buffer_index = 0;
    
    // Rotate through multiple buffers to avoid overwriting
    int index = buffer_index;
    buffer_index = (buffer_index + 1) % 4;
    
    char *result = &buffer[index * 128];
    
    strncpy(result, path, 123);
    result[123] = '\0';
    
    if (strlen(result) < 4 || strcmp(&result[strlen(result) - 4], ".mab") != 0) {
        strcat(result, ".mab");
    }
    return result;
}

int createSchemaPage() { return 0; }

int createDatabase(char *path) {
    char createNewDB[8];
    printf("Do you want to create a new database at %s (y/N): ", path);
    fgets(createNewDB, sizeof(createNewDB), stdin);
    printf("\n");

    if (strcmp(createNewDB, "y\n") != 0 && strcmp(createNewDB, "Y\n") != 0) {
        printf("Cancelling database creation.\n");
        exit(0);
    }

    FILE *filePtr;
    filePtr = fopen(path, "w");

    if (!filePtr) {
        printf("Failed to create database file\n");
        return 0;
    }

    printf("Database file created\n");

    fclose(filePtr);
    return 0;
}

MagBase *createMagBase(Header *header, char path[], bool newFile) {
    MagBase *magBase = malloc(sizeof(MagBase));
    magBase->filePath = path;
    if (newFile) {
        magBase->file_pointer = fopen(path, "wb");
    } else {
        magBase->file_pointer = fopen(path, "r+b");
    }
    magBase->buffer_pool = createBufferPool();
    magBase->header = header;
    magBase->page_size = PAGE_SIZE;
    return magBase;
}

int freeDatabase(MagBase *magBase) {
    // Flush all dirty pages before closing
    flushAllDirtyPages(magBase->buffer_pool, magBase);

    free(magBase->header);
    fclose(magBase->file_pointer);
    // free(magBase->filePath); // Not needed unless I decide to heap allocate the filepath
    free(magBase->buffer_pool);

    free(magBase);
    return 0;
}
