//
//
//
//          MagBase
//       A Maggie Database
//       A toy database
//       Start: 01/08/2026
//
//       ..it's weird to write 2026

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db-init.h"
#include "globals.h"

Version version = {DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH};

int main(int argc, char *argv[]) {

    if (argc == 1) {
        printf("Welcome page\n");
        return 0;
    }

    // Flag parser
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-v") ||
            !strcmp(argv[i], "-version")) { // strcmp returns 0 if they're equal,
                                            // performs a binary comparison
            printf("Version %d.%d.%d\n", version.major, version.minor, version.patch);

        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
            char *helpContent = getHelpContent();
            printf("%s", helpContent);
            free(helpContent);

        } else if (!strcmp(argv[i], "-p")) {
            // Check if files exists if not make it

            char *path = appendFileExt(argv[++i]);

            if (!checkIfFileExists(path)) {
                createDatabase(path);
            }
            printf("Opening MagBase at %s\n", path);

            // Open file then read header, if nothing exists write the header if it does read the
            // data
            FILE *tempFileP = fopen(path, "r");
            if (!tempFileP) {
                fprintf(stderr, "File failed to open while reading header.");
                exit(1);
            }

            Header *header = malloc(sizeof(Header));
            MagBase *magBase;

            size_t length = fread(header, sizeof(Header), 1, tempFileP);
            if (length != 1) {
                if (feof(tempFileP)) {
                    printf("Database is empty, initializing...\n");

                    header = createHeader(header);
                    magBase = createMagBase(header, path);

                    int result = writeHeader(magBase);
                    if (result == 0) {
                        printf("Database initialized.\n");
                    } else {
                        fprintf(stderr, "Database failed to initializing. ABORT\n");
                        exit(1);
                    }
                }
            } else {

                if (strcmp(header->magic, MAGIC) != 0) {
                    fprintf(stderr, "This is not a valid MagDB, ensure you are opening the correct "
                                    "file or it may be corrupted.");
                    exit(1);
                }

                magBase = createMagBase(header, path);
            }

            freeDatabase(magBase);
            exit(0);
        }
    }
}
