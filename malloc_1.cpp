//
// Created by edenl on 16/01/2023.
//

#include <cstdio>

void* smalloc(size_t size) {
    if(size == 0 || size > 108) return nullptr;
    // sbrk here, if fails return nullptr
    // if success, return a pointer to the first allocate byte within the allocated block
}