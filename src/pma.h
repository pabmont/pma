#ifndef __PMA_H_
#define __PMA_H_

// Questions for Michael:
// How to compute the size of the array and the size of the segments?
// How to compact things to the left
// How to insert? Do we preprocess every element to find the position of the predecessor, or do we
// do it on the fly?

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t key_t;

struct pma;
typedef struct pma pma_t, *PMA;

static const double t_0 = 0.75;
static const double t_h = 1;
static const double p_0 = 0.5;
static const double p_h = 0.25;

PMA pma_create (key_t *array, uint64_t n);
void pma_destroy (PMA *pma);
bool pma_find (PMA pma, key_t key);
bool pma_predecessor (PMA pma, key_t key);
void pma_insert_after (PMA pma, uint64_t i, key_t key);
void pma_insert (PMA pma, key_t key);
bool pma_empty (PMA p, uint64_t i);
uint64_t pma_size (PMA p);
uint64_t pma_count (PMA p);

#endif  /* __PMA_H_ */
