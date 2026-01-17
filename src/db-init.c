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
    return 0;
}

char *appendFileExt(char *path) {
    char lastFour[5];
    int lastFourOffset = 0;

    if (strlen(path) >= 4) {
        for (int i = strlen(path) - 4; i < strlen(path); i++) {
            lastFour[lastFourOffset] = path[i];
            lastFourOffset++;
        }
        lastFour[4] = '\0';
        // Last four chars are found to looks for the file extension
        if (strcmp(lastFour, ".mab") != 0) {
            // Path does not already have the file extension, need to append.
            strcat(path, ".mab");
        }
    } else {
        strcat(path, ".mab");
    }
    return path;
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
    free(magBase->header);
    fclose(magBase->file_pointer);
    // free(magBase->filePath); // Not needed unless I decide to heap allocate the filepath
    free(magBase->buffer_pool);

    free(magBase);
    return 0;
}
