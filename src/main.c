//
//
//
//          MagBase
//       A Maggie Database
//       A toy database
//       Start: 01/08/2026
//
//       ..it's weird to write 2026

#define VERSION "1.0.0"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "db-init.h"

int main(int argc, char* argv[])  {

    if (argc == 1) {
        printf("Welcome page\n");
        return 0;
    }
    
    // Flag parser
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "-version")) { // strcmp returns 0 if they're equal, performs a binary comparison
            printf("Version %s\n", VERSION);

        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
            char *helpContent = getHelpContent();
            printf("%s", helpContent);
            free(helpContent);

        } else if (!strcmp(argv[i], "-p")) {
// Check if files exists if not make it 
            if (checkIfFileExists(argv[++i])) {
                printf("A database already exists at %s\n", argv[i]);
                break;
            }

            createDatabase(argv[i]);

        }
    }
}
