//     Keagan Anderson
//        MagBase
//       01/08/2026


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

char *getHelpContent(void) {
    FILE *filePtr;

    filePtr = fopen("../docs/help-pages.txt", "r");

    if (!filePtr) {
        return("Hm... The help file is missing? Why'd you delete that.\nYou can get a new one from the github repo:\nhttps://github.com/KeaganAnd/MagBase/blob/main/docs/help-page.txt\n");
    }

    fseek(filePtr, 0, SEEK_END);

    long size = ftell(filePtr);
    if (size < 0) {
        fclose(filePtr);
        return "Failed to read help page\n";
    }

    rewind(filePtr);

    char *fileContents = malloc(size+1);
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
    if(!filePtr){return false;}
    fclose(filePtr);
    return true;
}

int createDatabase(char *path) {

    char lastFour[5];
    int lastFourOffset = 0;

    if (strlen(path) >= 4) {
        for (int i = strlen(path)-4; i < strlen(path); i++) {
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
    
    char createNewDB[8];
    printf("Do you want to create a new database at %s (y/N): ", path);
    fgets(createNewDB, sizeof(createNewDB), stdin);
    printf("\n");

    if (strcmp(createNewDB, "y\n") != 0 && strcmp(createNewDB, "Y\n") != 0) {
        printf("Cancelling database creation.\n");
        return (0);
    }


    FILE *filePtr;
    filePtr = fopen(path, "w");
    
    if (!filePtr) {
        printf("Failed to create database file\n");
        return 0;
    }

    printf("Database file created\n");

    fclose(filePtr);
    return 1;
}


