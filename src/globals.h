//     Keagan Anderson
//        MagBase
//       01/08/2026

#pragma once

#include "db-init.h"
#define DB_VERSION_MAJOR 1
#define DB_VERSION_MINOR 0
#define DB_VERSION_PATCH 0

#define PAGE_SIZE 4096
#define MAGIC "MAGDB.\0\0"
#define BUFFER_SIZE 10

extern Version version;
