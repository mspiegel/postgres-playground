# Coding assignment: Median aggregate

The task of this coding assignment is to implement an aggregate
function that calculates the *median* over an input set (typically
values from a table). Documentation on how to create aggregates can be
found in the official PostgreSQL documentation under [User-defined
Aggregates](https://www.postgresql.org/docs/current/static/xaggr.html)

## Initial Thoughts

The problem statement is to find the median of a set of values. If the problem was to find an approximate median for the values, that would be much simpler in the sense that there are constant-space approximation algorithms for finding percentile distributions. But from the provided tests (and we need to get the median of non-approximate things such as strings) we need to find the exact median.

I believe there is no sub-linear space complexity algorithm for finding the median of a stream of values. Storing all values in memory will be a concern for large input set sizes. I can think of the following basic strategies. These all have O(n) space complexity.

1. Build an unsorted array of all the values. In the final function, sort the array and return the median. This is O(n log n) time complexity. This is a good baseline solution to measure against the other solutions.
2. Build an unsorted array of all the values. In the final function, use either the median-of-median algorithm or the quickselect algorithm. Median-of-median algorithm has worst-case O(n) time complexity. Quickselect has average-case O(n) and worst-case O(n^2) time complexity.
3. Build an unsorted array of all the values. Use the 2 heaps strategy to find the median. Using binary heaps this algorithm has average-case O(n) and worst-case O(n log n) time complexity. A downside of the heaps is that they require storage in addition to the unsorted array. Doesn't change the O(n) space complexity.

## Implementation

Here are two implementations of the median aggregate function.

The first implementation builds an unsorted array of all the values. In the final function
the array is sorted. This is O(n log n) time complexity.

The second implementation bulids an unsorted array of all the values. In the final function
the quickselect algorithm is applied. This is O(n) average-case time complexity and O(n^2)
worst-case time complexity. We select the middle value to use as a pivot to avoid
O(n^2) runtime on sorted data.

## Future Work

In the second implementation could use 3-way partitioning and select a random pivot value
to reduce the likelihood of the worst-case O(n^2) behavior.

These algorithms use O(n) space complexity. My intuition is that reducing the space
required is more important in order to apply this aggregate on large data sets.
If the space required can be reduced then whatever resulting algorithm we
create can be subsequently optimized to make it time efficient.

If Postgres allows us to define an aggregate that makes two passes through the data,
then we can reduce the space required by a constant factor. Does Postgres allow
macro sql definitions? If yes, then we could define `median(values)` as
`median-pass2(median-pass1(values), values)` as an aggregate that makes two passes.

In the first pass of the data I would use a data structure that stores
approximate order statistics in constant space. A data structure
such as a [q-digest](https://papercruncher.wordpress.com/2011/07/31/q-digest/)
or a [t-digest](https://github.com/tdunning/t-digest).

In the second pass of the data I use the order statistics and
keep track of the following: the count of values smaller than the 45-percentile
approximation, the count of values larger than the 55-percentile
approximation, and store a buffer with all the remaining values.
If each of the counts is less than half the number of elements,
then I haven't lost information and I can use the buffer to determine
the median. On average, this should use 1/10 the space of storing
all the elements.

If too many elements have been thrown away then fallback
to larger percentile distributions. Or fallback to sorting the
entire range of values.

If the sequence has few distinct values (worst-case only a single distinct
value) then this algorithm will fail. A heuristic for this input
is to store a sorted map (B-tree) in the first pass of the algorithm.
The input data are keys and the counts are the values. Select a
constant size for the map. If the entire input stream can fit in
the map then it can be used to determine the median value.
