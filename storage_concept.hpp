#ifndef SDR_STORAGE_CONCEPT_H_
#define SDR_STORAGE_CONCEPT_H_

#include "constants.hpp"
#include "concept.hpp"


#include <unordered_set>
#include <iterator>
#include <vector>


namespace sdr
{

// represents a concept in storage
// designed to be able to be quickly queried and modified
struct storage_concept
{
    // store the positions of all set bits from 0 -> width
    std::unordered_set<sdr::position_t> positions;

    storage_concept(const sdr::concept & concept)
     : positions(std::begin(concept.data), std::end(concept.data))
    {}

    void fill(const sdr::concept & concept)
    {
        std::copy(std::begin(concept.data), std::end(concept.data), std::inserter(positions, std::begin(positions)));
    }

    void clear()
    {
        positions.clear();
    }

    // conversion to concept via cast
    operator sdr::concept() const
    {
        return sdr::concept(std::vector<sdr::position_t>(std::begin(positions), std::end(positions)));
    }
};

} //namespace sdr

#endif
