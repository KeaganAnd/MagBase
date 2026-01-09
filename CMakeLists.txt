cmake_minimum_required(VERSION 3.10)
project(magbase C)

set(CMAKE_C_STANDARD 11)


set(SOURCES
    src/main.c
    src/db-init.c
    src/db-actions.c
)

set(HEADERS
    src/main.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

