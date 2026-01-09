//     Keagan Anderson
//        MagBase
//       01/08/2026


#include <stdio.h>
#include <stdlib.h>

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
