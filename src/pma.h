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

/*
 * Packed-Memory Array as described in [1, 2, 3, 4].
 *
 * [1] Michael A. Bender, Erik D. Demaine, and Martin Farach-Colton.
 *     Cache-oblivious B-trees. In Proceedings of the 41st Annual Symposium on
 *     Foundations of Computer Science (FOCS '00), pages 399--409, Redondo
 *     Beach, California, USA, November 2000. IEEE Computer Society.
 * [2] Michael A. Bender, Erik D. Demaine, and Martin Farach-Colton.
 *     Cache-oblivious B-trees. SIAM Journal on Computing, 35(2):341--358, 2005.
 * [3] Michael A. Bender and Haodong Hu. An adaptive packed-memory array. In
 *     Proceedings of the 25th ACM SIGACT-SIGMOD-SIGART Symposium on Principles
 *     of Database Systems (PODS '06), pages 20--29, Chicago, Illinois, USA,
 *     June 2006. ACM.
 * [4] Michael A. Bender and Haodong Hu. An adaptive packed-memory array. ACM
 *     Trans. Database Syst., 32(4), November 2007.
 */

#ifndef __PMA_H_
#define __PMA_H_

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "keyval.h"

/* Reserve 8 bits to allow for fixed point arithmetic. */
#define MAX_SIZE ((1ULL << 56) - 1ULL)

/* Depth-based thresholds. */
/* Upper density thresholds. */
static const double t_0 = 0.75;  /* root. */
static const double t_h = 1.00;  /* leaves. */
/* Lower density thresholds. */
static const double p_0 = 0.50;  /* root. */
static const double p_h = 0.25;  /* leaves. */

struct _pma;
typedef struct _pma pma_t, *PMA;

PMA pma_create (keyval_t *array, uint64_t n);
void pma_destroy (PMA *pma);
bool pma_find (PMA pma, key_t key, uint64_t *index);
void pma_insert_after (PMA pma, uint64_t i, key_t key, val_t val);
void pma_insert (PMA pma, key_t key, val_t val);
void pma_get (PMA pma, uint64_t i, keyval_t *keyval);
uint64_t pma_capacity (PMA p);
uint64_t pma_count (PMA p);

#endif  /* __PMA_H_ */
