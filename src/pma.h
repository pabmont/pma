#ifndef __PMA_H_
#define __PMA_H_

// Questions for Michael:
// - How to compute the size of the array and the size of the segments?
// - How to compact things to the left. When packing, are we allowed to use all used levels, or only
// the one we are promoting to?
// - Do we also promote elements that have been insterted in the meantime?
// - How to insert? Do we preprocess every element to find the position of the predecessor, or do we
// do it on the fly?
// - Should we have an implicit tree even for the amortized version?
// - Do we have to ``clean'' as we are packing?
// - Is it true that there can be $O(n)$ cleaning processes active at the same time? Do we trigger a
// step only on those windows that contain a new element being inserted? When do cleaning processes
// stop? Is it something likne: when a cleaning process finishes, it kills all processes underneath
// it? Can we do this in $o(n)$?


#endif  /* __PMA_H_ */
