# Concurrent Mergesort
Time analysis and comparision of concurrent mergesort with normal mergesort.

## Details
- There are four types of sorting algorithm I am comparing here.
    1. `qsort` builtin sorting algorithm in C.
    2. `normal` (without forking, just one process)
    3. `concurrent` (forking for every part)
    4. `threaded` (uses multiple threads for sorting)

For each time comes compare while running
