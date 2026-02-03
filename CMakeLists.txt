cmake_minimum_required(VERSION 3.10)
project(magbase C)

set(CMAKE_C_STANDARD 11)


set(SOURCES
    src/main.c
    src/db-init.c
    src/db-actions.c
    src/buffer.c
    src/schema.c
    src/records.c
)

set(HEADERS
    src/main.h
    src/buffer.h
    src/db-init.h
    src/db-actions.h
    src/schema.h
    src/records.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

