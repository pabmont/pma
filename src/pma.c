#include "pma.h"

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/* Returns the 1-based index of the last (i.e., most significant) bit set in x.
 * Returns 0 if no bit is set.
 */
inline uint64_t last_bit_set (uint64_t x) {
  assert (x > 0);
  return (sizeof (uint64_t) * 8 - __builtin_clzll (x));
}

inline uint64_t floor_lg (uint64_t x) {
  return (last_bit_set (x) - 1);
}

inline uint64_t ceil_lg (uint64_t x) {
  return (last_bit_set (x - 1));
}

/* Returns the largest power of 2 not greater than x
 * (i.e., $2^{\lfloor \lg x \rfloor}$).
 */
inline uint64_t hyperfloor (uint64_t x) {
  return (1 << floor_lg (x));
}

/* Returns the smallest power of 2 not less than x
 * (i.e., $2^{\lceil \lg x \rceil}$).
 */
inline uint64_t hyperceil (uint64_t x) {
  return (1 << ceil_lg (x));
}

inline bool is_power_of_2 (uint64_t x) {
  return (x && !(x & (x - 1)));
}

/* Returns the smallest power of 2 strictly greater than x. */
inline uint64_t next_power_of_2 (uint64_t x) {
  return (1 << last_bit_set (x));
}

inline int min(int a, int b) {
  return ((a < b) ? a : b);
}

inline int max(int a, int b) {
  return ((a > b) ? a : b);
}

typedef uint64_t key_t;
typedef uint64_t val_t;

typedef struct {
  key_t key;
  val_t val;
} keyval_t;

/* Returns true if keyval is empty and false otherwise. */
bool keyval_empty (const keyval_t *keyval) {
  return (keyval->key == 0ULL);
}

/* Sets keyval to be empty. */
void keyval_clear (keyval_t *keyval) {
  keyval->key = 0ULL;
  keyval->val = 0ULL;
}

/* Reserve 8 bits to allow for fixed point arithmetic. */
#define MAX_SIZE ((1ULL << 56) - 1ULL)

/* Depth-based thresholds. */
/* Upper density thresholds. */
static const double t_0 = 0.75;  /* root. */
static const double t_h = 1.00;  /* leaves. */
/* Lower density thresholds. */
static const double p_0 = 0.50;  /* root. */
static const double p_h = 0.25;  /* leaves. */

typedef struct {
  uint64_t n;  /* Number of elements. */
  uint64_t m;  /* Size of the array. */
  uint8_t s;  /* Size of the segments. */
  uint64_t num_segments;
  uint8_t h;  /* Height of the tree. */
  double delta_t;  /* Delta for upper density threshold. */
  double delta_p;  /* Delta for lower density threshold. */
  keyval_t *array;
} pma_t, *PMA;

PMA pma_create (keyval_t *array, uint64_t n) {
  PMA pma = (PMA)malloc (sizeof (pma_t));
  pma->n = n;
  pma->m = hyperceil (2 * n);
  pma->s = floor_log_2 (pma->m);  // Or 2 * floor_log_2 (pma->m)
  pma->num_segments = hyperfloor (pma->m / pma->s);
  pma->m = pma->num_segments * pma->s;
  assert (pma->m <= MAX_SIZE);
  assert (pma->m > pma->n);
  pma->h = floor_log_2 (num_segments) + 1;
  pma->delta_t = (t_h - t_0) / pma->h;
  pma->delta_p = (p_0 - p_h) / pma->h;
  pma->array = (keyval_t *)malloc (sizeof (keyval_t) * pma->m);
  for (uint64_t i = 0; i < m; i++) {
    if (i < pma->n) {
      pma->array [i].key = array [i].key;
      pma->array [i].val = array [i].val;
    } else {
      keyval_clear (&(pma->array [i]));
    }
  }
  pma_spread (pma, 0, m, pma->n);
  return (pma);
}

void pma_destroy (PMA *pma) {
  free ((*pma)->array);
  (*pma)->array = NULL;
  free (*pma);
  *pma = NULL;
}

/*
 * Perform a modified binary search, with $O (\lg n)$ comparisons, that allows
 * gaps of size $O(1)$ in the array.
 * Returns true if the element is found and false otherwise.
 * If the element is found, index holds the position in the array where the
 * element associated with key is stored.
 * If the element is not found, index holds the position of the predecessor or
 * -1 if no predecessor exist in the array.
 */
bool pma_find (PMA pma, key_t key, uint64_t *index) {
  uint64_t from = 0;
  uint64_t to = pma->m - 1;
  while (from < to) {
    uint64_t mid = from + (to - from) / 2;
    uint64_t i = mid;
    /* Start scanning left until we find a non-empty spot or we reach past the
     * beginning of the subarray. */
    while (i >= from &&
           keyval_empty (&(pma->array [left])))
      i--;
    if (i < from) {  /* Everything between from and mid (inclusive) is empty. */
      from = mid + 1;
    } else {
      if (pma->array [i].key == key) {
        *index = i;
        return (true);
      }
      else if (pma->array [i].key < key) {
        from = mid + 1;
      } else {
        to = left - 1;
      }
    }
  }
  /* from == to */
  if (keyval_empty (&(pma->array [from])) ||
      pma->array [from].key > key) {
    *index = from - 1;
    while (*index >= 0 && keyval_empty (&(pma->array [*index])))
      (*index)--;
  } else {
    *index = from;
    if (pma->array [from].key == key)
      return (true);
  }
  return (false);
}

void pma_range (PMA pma, uint64_t from, uint64_t to,
                uint64_t *k, keyval_t **output) {
  // TODO: NYI.
}

/* TODO: Should to and from be inclusive? */
/* Returns how many elements were packed. */
uint64_t pma_pack (PMA pma, uint64_t from, uint64_t to) {
  assert (from < to);
  uint64_t read_index = from;
  uint64_t write_index = from;
  while (read_index < to) {
    if (!keyval_empty (&(pma->array [read_index]))) {
      if (read_index > write_index) {
        pma->array [write_index].key = pma->array [read_index].key;
        pma->array [write_index].val = pma->array [read_index].val;
        keyval_clean (&(pma->array [read_index]));
      }
      write_index++;
    }
    read_index++;
  }
  return (write_index - 1);  /* TODO: Do I need to return this, or do I know it already? */
}

/* TODO: Should to and from be inclusive? */
void pma_spread (PMA pma, uint64_t from, uint64_t to, uint64_t n) {
  assert (from < to);  // Assuming from is inclusive and to is exclusive.
  uint64_t capacity = to - from;  // Assuming from is inclusive and to is exclusive.
  uint64_t frequency = (capacity << 8) / n;  /* 8-bit fixed point arithmetic. */
  uint64_t read_index = from + n - 1;  // Assuming from is inclusive. */
  uint64_t write_index = (to << 8) - frequency;  // Assuming to is exclusinve
  while ((write_index >> 8) > read_index) {  // TODO: Do we have to check if w and r are >= 0? */
    pma->array [write_index >> 8].key = pma->array [read_index].key;
    pma->array [write_index >> 8].val = pma->array [read_index].val;
    keyval_clean (&(pma->array [read_index]));
    read_index--;
    write_index -= frequency;
  }
}

/* TODO: Should to and from be inclusive? */
void pma_rebalance (PMA pma, uint64_t from, uint64_t to) {
  pma_pack (pma, from, to);
  pma_spread (pma, from, to);
}


void pma_insert_after (PMA pma, uint64_t i, key_t key, val_t val) {
  pma->n++;
}

void pma_insert (PMA pma, key_t key, val_t val) {
  uint64_t i;
  /* Do not allow duplicates.*/
  if (!pma_find (pma, key, &i))
    pma_insert_after (pma, i, key, val);
}

void pma_grow (PMA pma) {
}

void pma_shrink (PMA pma) {
}

void pma_get (PMA pma, uint64_t i, keyval_t *keyval) {
  *keyval = pma->array [i];
}

/* Returns the size of the array. */
uint64_t pma_capacity (PMA p) {
  return p->m;
}

/* Returns the number of elements in the array. */
uint64_t pma_count (PMA p) {
  return p->n;
}
