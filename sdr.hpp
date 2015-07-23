#ifndef SDR_H_
#define SDR_H_

#include <vector>
#include <array>
#include <bitset>
#include <iostream>
#include <utility>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <unordered_set>
#include <future>
#include <thread>


namespace sdr
{


typedef std::size_t position_t;
typedef std::size_t width_t;



// this holds all of the traits for a single concept
// ie - if width is 1024, this could hold up to 1024 values between 0 and 1024
// designed to be fast to create, pass around, and as util to compress sparse data
struct concept
{
    std::vector<position_t> data;

    // for vector of positions we assume the data has already been dealt with
    concept(const std::vector<position_t> & input) : data(input)
    {}

    // otherwise we assume that is hasn't, thus we only add non zero fields
    template <width_t W> concept(const std::bitset<W> & input) : data()
    {
        for(std::size_t i=0; i < input.size(); ++i) {
            if(input[i]) {
                data.push_back(i);
            }
        }
    }

    // store [amount] largest ones from input
    template <typename T, width_t W> concept(const std::array<T, W> & input, const std::size_t amount) : data()
    {
        std::vector<position_t> idx(input.size());
        std::iota(std::begin(idx), std::end(idx), 0);

        std::partial_sort(std::begin(idx), std::begin(idx) + amount, std::end(idx), [&](
            const T a,
            const T b
        ) {
            return input[a] > input[b];
        });

        for(std::size_t i=0; i < amount; ++i) {
            // end if value is 0
            if(! input[idx[i]]) {
                break;
            }

            data.push_back(idx[i]);
        }
    }
};


// represents a concept in storage
// designed to be able to be quickly queried and modified
struct storage_concept
{
    // store the positions of all set bits from 0 -> width
    std::unordered_set<position_t> positions;

    storage_concept(const concept & concept) : positions(std::begin(concept.data), std::end(concept.data))
    {}

    void fill(const concept & concept)
    {
        std::copy(std::begin(concept.data), std::end(concept.data), std::inserter(positions, std::begin(positions)));
    }

    void clear()
    {
        positions.clear();
    }

    // conversion to concept via cast
    operator concept() const
    {
        return concept(std::vector<position_t>(std::begin(positions), std::end(positions)));
    }
};


// this is the memory bank for the sdr memory unit
template <width_t Width> struct bank
{
    // all inputs we have ever received, we store here compressed into storage
    std::vector<storage_concept> storage;

    // this holds our sets of vectors for easy comparison of different objects in storage
    std::array<std::unordered_set<position_t>, Width> bitmap;

    bank() : storage(), bitmap()
    {}

    position_t insert(const concept & concept)
    {
        storage.push_back(storage_concept(concept));

        const position_t last_pos { storage.size() - 1 };

        for(position_t pos : concept.data) {
            bitmap[pos].insert(last_pos);
        }

        return last_pos;
    }

    void update(const position_t pos, const concept & concept)
    {
        storage_concept & storage_concept { storage[pos] };

        for(position_t i : storage_concept.positions) {
            bitmap[i].erase(pos);
        }

        storage_concept.clear();

        storage_concept.fill(concept);

        for(position_t p : storage_concept.positions) {
            bitmap[p].insert(pos);
        }
    }

    // find amount of matching bits between two vectors
    std::size_t similarity(const position_t a, const position_t b) const
    {
        std::size_t result { 0 };

        for(position_t pos : storage[a].positions) {
            result += bitmap[pos].count(b);
        }

        return result;
    }

    // find amount of matching bits between two vectors
    double weighted_similarity(const position_t a, const position_t b, const std::array<double, Width> & weights) const
    {
        double result { 0 };

        for(position_t pos : storage[a].positions) {
            result += bitmap[pos].count(b) * weights[pos];
        }

        return result;
    }

    // find similarity of one object compared to the OR'd result of a list of objects
    std::size_t union_similarity(const position_t pos, const std::vector<position_t> & positions) const
    {
        std::bitset<Width> punions;

        for(const position_t ppos : positions) {
            for(const position_t spos : storage[ppos].positions) {
                punions.set(spos);
            }
        }

        std::size_t result { 0 };

        for(const position_t cmp : storage[pos].positions) {
            result += punions[cmp];
        }

        return result;
    }

    // find similarity of one object compared to the OR'd result of a list of objects
    double weighted_union_similarity(const position_t pos, const std::vector<position_t> & positions, const std::array<double, Width> & weights) const
    {
        std::bitset<Width> punions;

        for(const position_t ppos : positions) {
            for(const position_t spos : storage[ppos].positions) {
                punions.set(spos);
            }
        }

        double result { 0 };

        for(const position_t cmp : storage[pos].positions) {
            result += punions[cmp] * weights[cmp];
        }

        return result;
    }

    // find most similar to object at pos
    // first refers to position
    // second refers to matching number of bits
    std::vector<std::pair<position_t, std::size_t>> closest(const position_t pos, const std::size_t amount) const
    {
        std::vector<position_t> idx(storage.size());
        std::vector<unsigned>     v(storage.size());

        // if there are less than amount in storage, just return amount that exist
        const std::size_t partial_amount { (amount >= idx.size()) ? idx.size() : amount };

        std::iota(std::begin(idx), std::end(idx), 0);

        // count matching bits for each
        for(const position_t spos : storage[pos].positions) {
            for(const position_t bpos : bitmap[spos]) {
                ++v[bpos];
            }
        }

        // we dont care about our self similarity
        v[pos] = 0;

        std::partial_sort(std::begin(idx), std::begin(idx) + partial_amount, std::end(idx), [&](
            const position_t a,
            const position_t b
        ) {
            return v[a] > v[b];
        });

        // create std::pair for result
        std::vector<std::pair<position_t, std::size_t>> ret;
        ret.reserve(partial_amount);

        for(std::size_t i=0; i<partial_amount; ++i) {
            const position_t m { idx[i] };
            ret.push_back(std::make_pair(m, static_cast<std::size_t>(v[m])));
        }

        return ret;
    }

    std::future<std::vector<std::pair<position_t, std::size_t>>> async_closest(const position_t pos, const std::size_t amount) const
    {
        return std::async(std::launch::async, &bank<Width>::closest, this, pos, amount);
    }

    // find most similar to object at pos
    // first refers to position
    // second refers to matching number of bits
    std::vector<std::pair<position_t, double>> weighted_closest(const position_t pos, const std::size_t amount, const std::array<double, Width> & weights) const
    {
        std::vector<position_t> idx(storage.size());
        std::vector<double>       v(storage.size());

        // if there are less than amount in storage, just return amount that exist
        const std::size_t partial_amount { (amount >= idx.size()) ? idx.size() : amount };

        std::iota(std::begin(idx), std::end(idx), 0);

        // count matching bits for each
        for(const position_t spos : storage[pos].positions) {
            for(const position_t bpos : bitmap[spos]) {
                v[bpos] += weights[spos];
            }
        }

        // we dont care about our self similarity
        v[pos] = 0.0;

        std::partial_sort(std::begin(idx), std::begin(idx) + partial_amount, std::end(idx), [&](
            const position_t a,
            const position_t b
        ) {
            return v[a] > v[b];
        });

        // create std::pair for result
        std::vector<std::pair<position_t, double>> ret;
        ret.reserve(partial_amount);

        for(std::size_t i=0; i<partial_amount; ++i) {
            const position_t m { idx[i] };
            ret.push_back(std::make_pair(m, v[m]));
        }

        return ret;
    }

    std::future<std::vector<std::pair<position_t, std::size_t>>> async_weighted_closest(const position_t pos, const std::size_t amount, const std::array<double, Width> & weights) const
    {
        return std::async(std::launch::async, &bank<Width>::weighted_closest, this, pos, amount, weights);
    }

    // return all items matching all in data
    std::vector<position_t> matching(const concept & concept) const
    {
        std::unordered_set<position_t> matching;

        for(const std::size_t item : concept.data) {
            for(const std::size_t pos : bitmap[item]) {
                for(const std::size_t m : concept.data) {
                    if(bitmap[m].count(pos) == 0) {
                        goto skip;
                    }
                }

                matching.insert(pos);
                skip:;
            }
        }

        return { std::begin(matching), std::end(matching) };
    }

    // has to match amount in data
    std::vector<position_t> matching(const concept & concept, const std::size_t amount) const
    {
        std::unordered_set<position_t> matching;

        for(const std::size_t item : concept.data) {
            for(const std::size_t pos : bitmap[item]) {
                std::size_t amount_matching { 0 };

                for(const std::size_t m : concept.data) {
                    amount_matching += bitmap[m].count(pos);
                }

                if(amount_matching >= amount) {
                    matching.insert(pos);
                }
            }
        }

        return { std::begin(matching), std::begin(matching) };
    }

    // has to match amount in data
    std::vector<position_t> weighted_matching(const std::vector<position_t> & data, const double amount, const std::array<double, Width> & weights) const
    {
        std::unordered_set<position_t> matching;

        for(const std::size_t item : data) {
            for(const std::size_t pos : bitmap[item]) {
                double amount_matching { 0 };

                for(const std::size_t m : data) {
                    amount_matching += bitmap[m].count(pos) * weights[m];
                }

                if(amount_matching >= amount) {
                    matching.insert(pos);
                }
            }
        }

        return { std::begin(matching), std::end(matching) };
    }
};

} //namespace sdr

#endif
