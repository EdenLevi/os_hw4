//
// Created by edenl on 16/01/2023.
//

#include <cstdio>

void* smalloc(size_t size) {

}


/*● Tries to allocate ‘size’ bytes.
● Return value:
i. Success –a pointer to the first allocated byte within the allocated block.
ii. Failure –
a. If ‘size’ is 0 returns NULL.
b. If ‘size’ is more than 108, return NULL.
c. If sbrk fails, return NULL. */