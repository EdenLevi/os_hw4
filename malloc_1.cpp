//
// Created by edenl on 16/01/2023.
//

#include <cstring>
#include <cmath>
#include <unistd.h>

#define ERROR (void*)-1
#define MAX pow(10, 8)

void *smalloc(size_t size) {
    void* result;
    if (size == 0 || (double) size > MAX) return nullptr;
    if ((result = sbrk(size)) == ERROR) return nullptr;
    return result; // + sizeof(size_t) was here
    // if success, return a pointer to the first allocate byte within the allocated block
}
