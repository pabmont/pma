#include "pma.h"

struct pma {
    uint64_t n;
    uint64_t m;
    key_t *array;
};

PMA pma_create (key_t *array, uint64_t n) {
    PMA pma = (PMA)malloc (sizeof (pma_t));
    pma->m = n;
    pma->n = n;
    pma->array = (key_t *)malloc (sizeof (key_t) * pma->m);
    memset (pma->array, 0, sizeof (key_t) * pma->m);
    return pma;
}

void pma_destroy (PMA *pma) {
    if (*pma != NULL) {
        if ((*pma)->array != NULL) {
            free ((*pma)->array);
            (*pma)->array = NULL;
        }
        free (*pma);
        *pma = NULL;
    }
}

bool pma_find (PMA pma, key_t key) {
    uint64_t from = 0;
    uint64_t to = pma->m;
    while (from <= to) {
        uint64_t mid = from + (to - from) / 2;
        uint64_t left
        while (pma_empty (pma, mid))
            mid--;
        if (pma->array [mid] == key)
            return mid;
        else if (pma->array [mid] < key)
            from = mid + 1;
        else
            to = mid - 1;
    }
}

bool pma_predecessor (PMA pma, key_t key) {
}

void pma_insert_after (PMA pma, uint64_t i, key_t key) {
}

void pma_insert (PMA pma, key_t key) {
    uint64_t i = pma_predecessor (pma, key);
    pma_insert_after (pma, i, key)
}

// Returns true if the array at position 0 is empty and false otherwise.
// pre 0 <= i < pma_size (p)
bool pma_empty (PMA p, uint64_t i) {
    return p->array [i] == 0;
}

// Returns the number of positions currently in the array.
uint64_t pma_size (PMA p) {
    return p->m;
}

// Returns the number of elements currently in the array.
uint64_t pma_count (PMA p) {
    return p->n;
}
