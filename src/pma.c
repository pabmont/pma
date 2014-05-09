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

static void pack (PMA pma, uint64_t from, uint64_t to, uint64_t n);
static void spread (PMA pma, uint64_t from, uint64_t to, uint64_t n);
static void rebalance (PMA pma, uint64_t from, uint64_t to, uint64_t n);
static bool insert_in_segment_after (PMA pma, uint64_t i, key_t key, val_t val);
static bool find_rebalance_window (PMA pma, uint64_t i, uint64_t *window_start,
                                   uint64_t *window_end, uint64_t *occupancy);
static void grow (PMA pma);
static void shrink (PMA pma);

PMA pma_create (keyval_t *array, uint64_t n) {
  assert (n > 0);
  PMA pma = (PMA)malloc (sizeof (pma_t));
  pma->n = n;
  pma->m = hyperceil (2 * n);
  pma->s = floor_lg (pma->m);  // Or 2 * floor_lg (pma->m) for level-based.
  pma->num_segments = hyperfloor (pma->m / pma->s);
  pma->m = pma->num_segments * pma->s;
  assert (pma->m <= MAX_SIZE);
  assert (pma->m > pma->n);
  pma->h = floor_lg (pma->num_segments) + 1;
  pma->delta_t = (t_h - t_0) / pma->h;
  pma->delta_p = (p_0 - p_h) / pma->h;
  pma->array = (keyval_t *)malloc (sizeof (keyval_t) * pma->m);
  for (uint64_t i = 0; i < pma->m; i++) {
    if (i < pma->n) {
      pma->array [i].key = array [i].key;
      pma->array [i].val = array [i].val;
    } else {
      keyval_clear (&(pma->array [i]));
    }
  }
  spread (pma, 0, pma->m, pma->n);
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
           keyval_empty (&(pma->array [i])))
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
        to = i - 1;
      }
    }
  }
  /* from == to */
  if (keyval_empty (&(pma->array [from])) ||
      pma->array [from].key > key) {  /* Find predecessor. */
    *index = from - 1;
    while (*index >= 0 && keyval_empty (&(pma->array [*index])))
      (*index)--;
  } else {  /* This is the element we are looking for or its predecessor. */
    *index = from;
    if (pma->array [from].key == key)
      return (true);
  }
  return (false);
}

/* from is inclusive, to is exclusive. */
static void pack (PMA pma, uint64_t from, uint64_t to, uint64_t n) {
  assert (from < to);
  uint64_t read_index = from;
  uint64_t write_index = from;
  while (read_index < to) {
    if (!keyval_empty (&(pma->array [read_index]))) {
      if (read_index > write_index) {
        pma->array [write_index].key = pma->array [read_index].key;
        pma->array [write_index].val = pma->array [read_index].val;
        keyval_clear (&(pma->array [read_index]));
      }
      write_index++;
    }
    read_index++;
  }
  assert (n == write_index - 1);
}

/* from is inclusive, to is exclusive. */
static void spread (PMA pma, uint64_t from, uint64_t to, uint64_t n) {
  assert (from < to);
  uint64_t capacity = to - from;
  uint64_t frequency = (capacity << 8) / n;  /* 8-bit fixed point arithmetic. */
  uint64_t read_index = from + n - 1;
  uint64_t write_index = (to << 8) - frequency;
  while ((write_index >> 8) > read_index) {
    pma->array [write_index >> 8].key = pma->array [read_index].key;
    pma->array [write_index >> 8].val = pma->array [read_index].val;
    keyval_clear (&(pma->array [read_index]));
    read_index--;
    write_index -= frequency;
  }
}

/* from is inclusive, to is exclusive. */
static void rebalance (PMA pma, uint64_t from, uint64_t to, uint64_t n) {
  pack (pma, from, to, n);
  spread (pma, from, to, n);
}

/* Returns false if there is no space available in the segment and true
 * otherwise. */
static bool insert_in_segment_after (PMA pma, uint64_t i,
                                     key_t key, val_t val) {
  uint64_t segment = i / pma->s;
  uint64_t segment_start = segment * pma->s;
  uint64_t segment_end = segment_start + pma->s;
  /* Find the closest empty space within the segment. */
  /* We should be able to find one because we would have triggered a rebalance
   * earlier otherwise. */
  uint64_t left = i - 1;
  uint64_t right = i + 1;
  while ((left >= segment_start &&
         !keyval_empty (&(pma->array [left]))) ||
         (right < segment_end &&
         !keyval_empty (&(pma->array [right])))) {
    left--;
    right++;
  }
  /* shift elements by one to make space for the new element. */
  if ((left >= segment_start &&
       right < segment_end &&
       i - left < right - i) ||
      left >= segment_start) {  /* Push to the left. */
    for (uint64_t j = left; j < i; j++) {
      pma->array [j].key = pma->array [j + 1].key;
      pma->array [j].val = pma->array [j + 1].val;
    }
    pma->array [i].key = key;
    pma->array [i].val = val;
  } else if ((left >= segment_start &&
              right < segment_end &&
              i - left >= right - i) ||
             right < segment_end) {  /* Push to the right. */
    for (uint64_t j = right; j > i + 1; j++) {
      pma->array [j].key = pma->array [j - 1].key;
      pma->array [j].val = pma->array [j - 1].val;
    }
    pma->array [i + 1].key = key;
    pma->array [i + 1].val = val;
  } else {  /* No space available in the segment. */
    return (false);
  }
  pma->n++;
  return (true);
}

static bool find_rebalance_window (PMA pma, uint64_t i, uint64_t *window_start,
                                   uint64_t *window_end, uint64_t *occupancy) {
  uint8_t height = 0;
  *occupancy = (keyval_empty (&(pma->array [i]))) ? 0 : 1;
  uint64_t left_index = i - 1;
  uint64_t right_index = i + 1;
  double density, t_height, p_height;
  do {
    uint64_t window_size = pma->s * (1 << height);
    uint64_t window = i / window_size;
    *window_start = window * window_size;
    *window_end = *window_start + window_size;
    while (left_index >= *window_start) {
      if (!keyval_empty (&(pma->array [left_index])))
        (*occupancy)++;
      left_index--;
    }
    while (right_index < *window_end) {
      if (!keyval_empty (&(pma->array [right_index])))
        (*occupancy)++;
      right_index++;
    }
    density = (double)(*occupancy) / (double)window_size;
    t_height = t_0 - (height * pma->delta_t);
    p_height = p_0 + (height * pma->delta_p);
    height++;
  } while ((density < p_height || density >= t_height) &&
           height < pma->h);
  return (density >= p_height && density < t_height);
}

/* Returns true if the insertion was successful and false otherwise. */
bool pma_insert_after (PMA pma, uint64_t i, key_t key, val_t val) {
  assert (!keyval_empty (&(pma->array [i])));
  if (!insert_in_segment_after (pma, i, key, val))
    return (false);
  uint64_t occupancy, window_start, window_end;
  if (find_rebalance_window (pma, i, &window_start, &window_end, &occupancy))
    rebalance (pma, window_start, window_end, occupancy);
  else
    grow (pma);
  return (true);
}

bool pma_insert (PMA pma, key_t key, val_t val) {
  uint64_t i;
  if (!pma_find (pma, key, &i))  /* We do not allow duplicates.*/
    return (pma_insert_after (pma, i, key, val));
  return (false);
}

void pma_delete_at (PMA pma, uint64_t i) {
  keyval_clear (&(pma->array [i]));
  uint64_t occupancy, window_start, window_end;
  if (find_rebalance_window (pma, i, &window_start, &window_end, &occupancy))
    rebalance (pma, window_start, window_end, occupancy);
  else
    shrink (pma);
}

bool pma_delete (PMA pma, key_t key) {
  uint64_t i;
  if (pma_find (pma, key, &i)) {
    pma_delete_at (pma, i);
    return (true);
  }
  return (false);  /* key does not exist. */
}

static void grow (PMA pma) {
}

static void shrink (PMA pma) {
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
