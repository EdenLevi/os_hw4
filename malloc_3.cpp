//
// Created by edenl on 17/01/2023.
//

#include <cstring>
#include <cmath>
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>

#define ERROR (void*)-1
#define MAX pow(10, 8)
#define BIG_CHUNGUS (128*1024)
#define MIN_SPLIT 128

int randomCookie = rand();

class MallocMetadata {
public:
    int cookie;
    size_t size; // pure block - excluding size of meta data
    bool is_free;
    MallocMetadata *next;
    MallocMetadata *prev;
    // do we need to save the address?

    MallocMetadata() : cookie(randomCookie), size(0), is_free(false), next(nullptr), prev(nullptr) {}
};

void checkCookie(MallocMetadata *block) {
    if (block && block->cookie != randomCookie) {
        exit(0xdeadbeef);
    }
}

MallocMetadata heap;
MallocMetadata mapHeap;


void combineFreebies() {
    MallocMetadata *head = &heap;
    while (head->next) {
        checkCookie(head->next);
        if (head->is_free && head->next->is_free) {
            // found ya haha
            head->size += head->next->size + sizeof(MallocMetadata);

            if (head->next->next) {
                checkCookie(head->next->next);
                head->next->next->prev = head;
            }
            head->next = head->next->next;
            break;
        }
        head = head->next;
    }
}


void *BestFit(size_t size) {
    MallocMetadata *currentBest = nullptr;

    MallocMetadata *head = &heap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
        if (head->is_free && head->size >= size) { /// found one that fits
            if (!currentBest) currentBest = head; /// nothing in hand
            else if (head->size < currentBest->size) currentBest = head; /// found something better
        }
    }
    if (currentBest) {

        if (currentBest->size >= size + sizeof(MallocMetadata) + MIN_SPLIT) { // not *2 because size exclude it
            auto cutMeta = (MallocMetadata *) ((char *) currentBest + sizeof(MallocMetadata) + size);
            cutMeta->is_free = true;
            cutMeta->size = currentBest->size - sizeof(MallocMetadata) - size;
            cutMeta->cookie = randomCookie;

            cutMeta->prev = currentBest;
            cutMeta->next = currentBest->next;

            if (currentBest->next) {
                checkCookie(currentBest->next);
                currentBest->next->prev = cutMeta;
            }
            currentBest->next = cutMeta;
            currentBest->size = size;
            currentBest->cookie = randomCookie;

        }

        currentBest->is_free = false;
        ///currentBest->size = size; /// i think this is bad
        /// size should always hold the actual space allocated
        combineFreebies();
        return ((char *) currentBest + sizeof(MallocMetadata));
    }
    return nullptr;
}

/// block + metadata + metadata + 128 >=

MallocMetadata *GetLast() {
    MallocMetadata *head = &heap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
    }
    return head;
}

MallocMetadata *GetLastMap() {
    MallocMetadata *head = &mapHeap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
    }
    return head;
}

void *big_smalloc(size_t size) {
    //BIG ALLOCATION HAPPENS HERE
    void *address = mmap(NULL, sizeof(MallocMetadata) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
                         0);
    if (address == MAP_FAILED) return nullptr;
    auto block = (MallocMetadata *) address;

    block->size = size;
    block->cookie = randomCookie;
    block->is_free = false;
    block->prev = GetLastMap();
    block->prev->next = block;
    block->next = nullptr;
    return (void *) ((char *) address + sizeof(MallocMetadata));
}

void *smalloc(size_t size) {

    void *result;
    if (size == 0 || (double) size > MAX) return nullptr;

    if (size >= BIG_CHUNGUS) {
        // here magic will happen
        return big_smalloc(size);
    }

    MallocMetadata *last = GetLast();
    void *address = BestFit(size);
    if (address) return address;
    else if (last->is_free) {
        // 1. We know the wilderness block is free
        // 2. We know it is not big enough. Why? Because address returned null!!
        if (sbrk(size - last->size) == ERROR) return nullptr;
        last->size = size;
        return smalloc(size); /// try again
    }

    if ((result = sbrk(size + sizeof(MallocMetadata))) == ERROR) return nullptr;

    // sbrk successful
    auto newBlock = (MallocMetadata *) result;
    last->next = newBlock;

    newBlock->next = nullptr;
    newBlock->prev = last;
    newBlock->size = size;
    newBlock->is_free = false;
    newBlock->cookie = randomCookie;

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


void big_sfree(MallocMetadata *block) {
    /// pashut mumnap
    checkCookie(block);
    checkCookie(block->prev);
    block->prev->next = block->next;
    if (block->next) {
        checkCookie(block->next);
        block->next->prev = block->prev;
    }
    munmap(block, sizeof(MallocMetadata) + block->size);
}

void sfree(void *p) {

    /*
     * AINT NO SUNSHINE WHEN SHES GONE
     */
    if (!p) return;

    MallocMetadata *block = (MallocMetadata *) ((char *) p - sizeof(MallocMetadata));
    checkCookie(block);
    // did we need to remove the size of meta data? they claim they give good pointers

    if (block->is_free) return;

    /// here block actually frees
    if (block->size >= BIG_CHUNGUS) {
        return big_sfree(block);
    }

    checkCookie(block->prev);
    checkCookie(block->next);
    if ((block->prev && block->prev->is_free) || (block->next && block->next->is_free)) {
        if (block->next && block->next->is_free) {
            block->size += sizeof(MallocMetadata) + block->next->size;
            block->next = block->next->next;
            if (block->next) {
                checkCookie(block->next);
                block->next->prev = block;
            }
        }
        checkCookie(block->prev);
        if (block->prev && block->prev->is_free) {
            block->prev->size += sizeof(MallocMetadata) + block->size;
            block->prev->next = block->next;
            if (block->next) {
                checkCookie(block->next);
                block->next->prev = block->prev;
            }
            block = block->prev;
        }
    }
    block->is_free = true;
}

/*● Releases the usage of the block that starts with the pointer ‘p’.
● If ‘p’ is NULL or already released, simply returns.
● Presume that all pointers ‘p’ truly points to the beginning of an allocated block.*/

void *big_srealloc(void *oldp, MallocMetadata *oldBlock, void *result) {
    memcpy(result, oldp, oldBlock->size);
    sfree(oldp);
    return result;
}

void *srealloc(void *oldp, size_t size) {

    if (!oldp) return smalloc(size);
    if (size == 0 || (double) size > MAX) return nullptr;
    MallocMetadata *oldBlock = (MallocMetadata *) ((char *) oldp - sizeof(MallocMetadata));
    checkCookie(oldBlock);

    if (size >= BIG_CHUNGUS) {
        if (oldBlock->size >= size) {
            return oldp;
        }
        void *result = smalloc(size);
        if (!result) return nullptr;
        return big_srealloc(oldp, oldBlock, result);
    }

    //case a
    if (oldBlock->size >= size) {
        oldBlock->is_free = true;
        BestFit(size);
        return oldp;
    }

    //case b
    checkCookie(oldBlock->prev);
    if (oldBlock->prev->is_free && (oldBlock->size + oldBlock->prev->size + sizeof(MallocMetadata)) >= size) {
        MallocMetadata *result = oldBlock->prev;
        result->is_free = false;
        result->next = oldBlock->next;
        result->size = oldBlock->size + result->size + sizeof(MallocMetadata);
        memmove((char *) result + sizeof(MallocMetadata), oldp, oldBlock->size);

        if (result->next) {
            checkCookie(result->next);
            result->next->prev = result;
        }

        result->is_free = true;
        BestFit(size);
        return ((char *) result + sizeof(MallocMetadata));
    }

    //case b *
    if (oldBlock->prev->is_free && !oldBlock->next) { // old Block is wilderness
        MallocMetadata *result = oldBlock->prev;
        result->is_free = false;
        result->next = oldBlock->next;
        result->size = oldBlock->size + result->size + sizeof(MallocMetadata);
        memmove((char *) result + +sizeof(MallocMetadata), oldp, oldBlock->size);

        if (sbrk(size - result->size) == ERROR) return nullptr;
        result->size = size;

        result->is_free = true;
        BestFit(size); // used to split
        return ((char *) result + sizeof(MallocMetadata));
    }

    //case c
    if (!oldBlock->next) {
        if (sbrk(size - oldBlock->size) == ERROR) return nullptr;
        oldBlock->size = size;

        oldBlock->is_free = true;
        BestFit(size); // used to split

        return ((char *) oldBlock + sizeof(MallocMetadata));
    }

    //case d
    checkCookie(oldBlock->next);
    if (oldBlock->next && oldBlock->next->is_free &&
        (oldBlock->size + oldBlock->next->size + sizeof(MallocMetadata)) >= size) {
        oldBlock->size += oldBlock->next->size + sizeof(MallocMetadata);
        oldBlock->next = oldBlock->next->next;

        if (oldBlock->next) {
            checkCookie(oldBlock->next);
            oldBlock->next->prev = oldBlock;
        }

        oldBlock->is_free = true;
        BestFit(size);

        return ((char *) oldBlock + sizeof(MallocMetadata));
    }

    //case e - three adjacent blocks
    if (oldBlock->next && oldBlock->next->is_free && oldBlock->prev->is_free &&
        (oldBlock->prev->size + sizeof(MallocMetadata) * 2 + oldBlock->next->size + oldBlock->size) >= size) {
        MallocMetadata *result = oldBlock->prev;

        /// add the one before me
        result->is_free = false;
        result->next = oldBlock->next;
        result->size += oldBlock->size + sizeof(MallocMetadata);
        memmove((char *) result + sizeof(MallocMetadata), oldp, oldBlock->size);

        if (result->next) {
            checkCookie(result->next);
            result->next->prev = result;
        }

        /// add the one after me
        result->size += result->next->size + sizeof(MallocMetadata);
        result->next = result->next->next;
        if (result->next) {
            checkCookie(result->next);
            result->next->prev = result;
        }
        result->is_free = true;
        BestFit(size);

        return ((char *) result + sizeof(MallocMetadata));
    }

    //case f(1)
    if (oldBlock->next && !oldBlock->next->next && oldBlock->next->is_free &&
        oldBlock->prev->is_free) {
        MallocMetadata *result = oldBlock->prev;
        size_t temp = oldBlock->size + result->size + sizeof(MallocMetadata) * 2 + oldBlock->next->size;
        result->is_free = false;
        result->next = oldBlock->next;
        result->size = oldBlock->size + result->size + sizeof(MallocMetadata);
        memmove((char *) result + sizeof(MallocMetadata), oldp, oldBlock->size);

        if (sbrk(size - temp) == ERROR) return nullptr;

        result->size = size;
        result->next = nullptr;

        return ((char *) result + sizeof(MallocMetadata));
    }

    //case f(2)
    if (oldBlock->next && !oldBlock->next->next && oldBlock->next->is_free) {
        size_t temp = oldBlock->size + oldBlock->next->size + sizeof(MallocMetadata);
        if (sbrk(size - temp) == ERROR) return nullptr;

        oldBlock->size = size;
        oldBlock->next = nullptr;

        return ((char *) oldBlock + sizeof(MallocMetadata));
    }

    void *result = smalloc(size);

    if (!result) return nullptr;
    if (result != oldp) {
        memmove(result, oldp, oldBlock->size);
        sfree(oldp);
    }

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
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
        if (head->is_free) counter++;
    }
    return counter;
}
// Returns the number of allocated blocks in the heap that are currently free.

size_t _num_free_bytes() {
    size_t counter = 0;
    auto head = &heap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
        if (head->is_free) counter += head->size;
    }
    return counter;
}

/*● Returns the number of bytes in all allocated blocks in the heap that are currently free,
        excluding the bytes used by the meta-data structs.*/

size_t _num_allocated_blocks() {
    size_t counter = 0;
    auto head = &heap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
        counter++;
    }
    head = &mapHeap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
        counter++;
    }
    return counter;
}
//● Returns the overall (free and used) number of allocated blocks in the heap.

size_t _num_allocated_bytes() {
    size_t counter = 0;
    auto head = &heap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
        counter += head->size;
    }
    head = &mapHeap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
        counter += head->size;
    }
    return counter;
}

/*● Returns the overall number (free and used) of allocated bytes in the heap, excluding
the bytes used by the meta-data structs.*/

size_t _num_meta_data_bytes() {
    size_t counter = 0;
    auto head = &heap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
        counter++;
    }
    head = &mapHeap;
    checkCookie(head);
    while (head->next) {
        head = head->next;
        checkCookie(head);
        counter++;
    }
    return counter * sizeof(MallocMetadata);
}
//● Returns the overall number of meta-data bytes currently in the heap.

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
//● Returns the number of bytes of a single meta-data structure in your system.