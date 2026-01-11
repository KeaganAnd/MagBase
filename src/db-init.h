#include <stdio.h>
#include <stdbool.h>

char *getHelpContent(void);
int createDatabase(char *path);
bool checkIfFileExists(char *path);

typedef struct {
    size_t num_tables;
    size_t next_free_page;
    // TODO Expand to include table schema when decided !!
} Header;

typedef struct {
    char **pages;
    size_t *page_ids;
    int num_pages;
    int *dirty_flags;
} BufferPool;

typedef struct{
    char *filePath;
    FILE *file_pointer;
    Header header;
    BufferPool *buffer_pool;
    size_t page_size;
} MagBase;
