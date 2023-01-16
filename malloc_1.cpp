//
// Created by edenl on 16/01/2023.
//

#include <cstdio>
#include <cmath>
#include <unistd.h>

#define ERROR -1
#define MAX pow(10, 8)

int sbrk(size_t size);

void *smalloc(size_t size) {
    int result;
    if (size == 0 || (double) size > MAX) return nullptr;
    if ((result = sbrk(size)) == ERROR) return nullptr;
    return (void *) result; // + sizeof(size_t) was here
    // if success, return a pointer to the first allocate byte within the allocated block
}

int sbrk(size_t size) {
    return 0;
}
