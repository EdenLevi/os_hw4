//
// Created by edenl on 16/01/2023.
//

#include <cstring>
#include <cmath>
#include <unistd.h>

#define ERROR (void*)-1
#define MAX pow(10, 8)

class MallocMetadata {
public:
    size_t size; // pure block - excluding size of meta data
    bool is_free;
    MallocMetadata *next;
    MallocMetadata *prev;
    // do we need to save the address?

    MallocMetadata() : size(0), is_free(false), next(nullptr), prev(nullptr) {}
};

MallocMetadata heap;

void *IsEnoughSpace(size_t size) {
    MallocMetadata *head = &heap;
    while (head->next) {
        head = head->next;
        if (head->is_free && head->size >= size) {
            // enough space :) :) :)
            head->is_free = false;
            head->size = size;
            return ((char *) head + sizeof(MallocMetadata));
        }
    }
    return nullptr;
}

MallocMetadata *GetLast() {
    MallocMetadata *head = &heap;
    while (head->next) {
        head = head->next;
    }
    return head;
}

void *smalloc(size_t size) {
    void *result;
    if (size == 0 || (double) size > MAX) return nullptr;

    void *address = IsEnoughSpace(size);
    if (address) return address;

    if ((result = sbrk(size + sizeof(MallocMetadata))) == ERROR) return nullptr;

    // sbrk successful
    auto newBlock = (MallocMetadata *) result;
    MallocMetadata *last = GetLast();
    last->next = newBlock;

    newBlock->next = nullptr;
    newBlock->prev = last;
    newBlock->size = size;
    newBlock->is_free = false;

    return (void *) ((char *) result + sizeof(MallocMetadata)); // make sure pointer calculation is good
}

/*● Searches for a free block with at least ‘size’ bytes or allocates (sbrk()) one if none are
found.
● Return value:
i. Success – returns pointer to the first byte in the allocated block (excluding the meta-data of
course)
ii. Failure –
a. If size is 0 returns NULL.
b. If ‘size’ is more than 108, return NULL.
c. If sbrk fails in allocating the needed space, return NULL.*/


void *scalloc(size_t num, size_t size) {
    void *result = smalloc(num * size);
    if (!result) return nullptr;

    // smalloc successful
    memset(result, 0, num * size);
    return result;
}

/*● Searches for a free block of at least ‘num’ elements, each ‘size’ bytes that are all set to 0
or allocates if none are found. In other words, find/allocate size * num bytes and set all
bytes to 0.
● Return value:
i. Success - returns pointer to the first byte in the allocated block.
ii. Failure –
a. If size or num is 0 returns NULL.
b. If ‘size * num’ is more than 108, return NULL.
c. If sbrk fails in allocating the needed space, return NULL.*/


void sfree(void *p) {

    /*
     * AINT NO SUNSHINE WHEN SHES GONE
     */
    if (!p) return;

    MallocMetadata *block = (MallocMetadata *) ((char *) p - sizeof(MallocMetadata));
    // did we need to remove the size of meta data? they claim they give good pointers

    if (!block->is_free) return;

    block->is_free = true;
}

/*● Releases the usage of the block that starts with the pointer ‘p’.
● If ‘p’ is NULL or already released, simply returns.
● Presume that all pointers ‘p’ truly points to the beginning of an allocated block.*/


void *srealloc(void *oldp, size_t size) {
    if (size == 0 || (double) size > MAX) return nullptr;
    MallocMetadata *oldBlock = (MallocMetadata *) ((char *) oldp - sizeof(MallocMetadata));

    if (oldBlock->size >= size) return oldp;
    void *result = smalloc(size);

    if (!result) return nullptr;

    memmove(result, oldp, oldBlock->size);
    sfree(oldp);

    return result;
}

/*● If ‘size’ is smaller than or equal to the current block’s size, reuses the same block.
Otherwise, finds/allocates ‘size’ bytes for a new space, copies content of oldp into the
new allocated space and frees the oldp.
● Return value:
i. Success –
a. Returns pointer to the first byte in the (newly) allocated space.
b. If ‘oldp’ is NULL, allocates space for ‘size’ bytes and returns a pointer to it.
ii. Failure –
a. If size is 0 returns NULL.
b. If ‘size’ if more than 108, return NULL.
c. If sbrk fails in allocating the needed space, return NULL.
d. Do not free ‘oldp’ if srealloc() fails.*/


/*On top of the memory allocation functions that you are defining, you are also required to define the
        following stats methods.*/
size_t _num_free_blocks() {
    size_t counter = 0;
    auto head = &heap;
    while(head->next) {
        head = head->next;
        if(head->is_free) counter++;
    }
    return counter;
}
// Returns the number of allocated blocks in the heap that are currently free.

size_t _num_free_bytes() {
    size_t counter = 0;
    auto head = &heap;
    while(head->next) {
        head = head->next;
        if(head->is_free) counter += head->size;
    }
    return counter;
}

/*● Returns the number of bytes in all allocated blocks in the heap that are currently free,
        excluding the bytes used by the meta-data structs.*/

size_t _num_allocated_blocks() {
    size_t counter = 0;
    auto head = &heap;
    while(head->next) {
        head = head->next;
        counter++;
    }
    return counter;
}
//● Returns the overall (free and used) number of allocated blocks in the heap.

size_t _num_allocated_bytes() {
    size_t counter = 0;
    auto head = &heap;
    while(head->next) {
        head = head->next;
        counter += head->size;
    }
    return counter;
}

/*● Returns the overall number (free and used) of allocated bytes in the heap, excluding
the bytes used by the meta-data structs.*/

size_t _num_meta_data_bytes() {
    size_t counter = 0;
    auto head = &heap;
    while(head->next) {
        head = head->next;
        counter++;
    }
    return counter*sizeof(MallocMetadata);
}
//● Returns the overall number of meta-data bytes currently in the heap.

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
//● Returns the number of bytes of a single meta-data structure in your system.
