/*
 * Copyright (c) 2014 Pablo Montes <pabmont@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "pma.h"

struct _pma {
  uint64_t n;  /* Number of elements. */
  uint64_t m;  /* Size of the array. */
  uint8_t s;  /* Size of the segments. */
  uint64_t num_segments;
  uint8_t h;  /* Height of the tree. */
  double delta_t;  /* Delta for upper density threshold. */
  double delta_p;  /* Delta for lower density threshold. */
  keyval_t *array;
};

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
  assert (!keyval_empty (&(pma->array [i])));
  /* Find the closest empty space within the segment. */
  uint64_t left = i - 1;
  uint64_t right = i + 1;
  while (!keyval_empty (&(pma->array [left])) &&
         !keyval_empty (&(pma->array [right]))) {
    left--;
    right++;
  }
  /* Find the smallest window within threshold. */
  while (density < x) {
  }
  /* Rebalance */
  pma_rebalance (pma, window_start, window_end);
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
