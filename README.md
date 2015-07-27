#sdr database

###Still work in progress

This is a library for storage and retrieval of sdr. It supports a few methods:

 * similarity (weighted)
 * union similarity (weighted)
 * matching (weighted)
 * closest (weighted, async, weighted_async)

As well as saving, loading, and utility methods.

###Designed to be very fast & idiomatic C++11.

Amount means total number of stored items in database

Width means dimensions per item

Percent means amount enabled


```
bench
    amount: 100000
    width: 2048
    percent: 0.02

generation took: 93ms
insertion took: 1077ms
save to file took: 201ms
load from file (100000 concepts) took: 771ms
finding closest (parallel => 100) took: 55ms
finding weighted closest took: 2ms
finding union similarity took: 0.006ms
finding matching took: 0.11ms
```

Generation is how long it took to create the data for 100k concepts. Ignore.

Insertion is adding those 100k concepts to the database.

We then save the file, clear the system, and load the file we saved.

Closest is the most interesting, I am able to find the closest 10 concepts  to any sdr within a database of 100k, each being 2048 dimensions with 2% enabled in about half a millisecond when run in parallel (we ran this for 100 different concepts at once, it took 55ms to get all results).

Weighted closest is simply the weighted modifier (multiplied by some value corresponding to each of the 2048 bits we pass in) being run only once. As you can see, there is significant value to batching these together.

Union similarity is finding how similar a single concept is to a batch of concepts OR'd with each other

Matching is finding concepts that match each of the traits/bits in a list.


###Requires


 * google sparsehash: https://code.google.com/p/sparsehash/
 * c++11 compiler (gcc or clang will do)

