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

            size_t length = fread(header, sizeof(Header), 1,
                                  tempFileP); // If the file has no data write the boilerplate
            if (length != 1) {
                if (feof(tempFileP)) {
                    printf("Database is empty, initializing...\n");

                    header = createHeader(header);
                    magBase = createMagBase(header, path, true);

                    int result = writeHeader(magBase);
                    if (result == 0) {
                        printf("Database initialized.\n");
                        fclose(tempFileP);
                    } else {
                        fprintf(stderr, "Database failed to initializing. ABORT\n");
                        fclose(tempFileP);
                        exit(1);
                    }
                }
            } else { // If has data check validity and read data

                if (strcmp(header->magic, MAGIC) != 0) {
                    fprintf(stderr, "This is not a valid MagDB, ensure you are opening the correct "
                                    "file or it may be corrupted.");
                    fclose(tempFileP);
                    exit(1);
                }
                fclose(tempFileP); // Runtime pointer gets made for the struct below

                magBase = createMagBase(header, path, false);
                printf("%d", magBase->header->version.major);
                if (magBase->header->version.major != version.major) {

                    if (magBase->header->version.major >= version.major) {
                        printf("WARNING: Database file is ahead of app\n");
                    } else {
                        printf("WARNING: Database file is behind app\n");
                    }
                    printf("%d.%d.%d vs %d.%d.%d\n", magBase->header->version.major,
                           magBase->header->version.minor, magBase->header->version.patch,
                           version.major, version.minor, version.patch);
                }
            }

            freeDatabase(magBase);
            exit(0);
        }
    }
}
