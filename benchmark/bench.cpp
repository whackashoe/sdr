#include <chrono>
#include <iostream>
#include <random>
#include "../includes/sdr.hpp"


std::size_t since_epoch()
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

int main()
{
    constexpr std::size_t Amount = 100000;
    constexpr std::size_t Width = 2048;
    constexpr std::size_t ParallelAmount = 100;
    constexpr float OnPercent = 0.02;

    sdr::bank<Width> memory;
    std::vector<std::bitset<Width>> fields;

    std::array<double, Width> weights;
    weights.fill(1.1);

    std::cout << "bench" << std::endl;
    std::cout << "\tamount: " << Amount << std::endl;
    std::cout << "\twidth: " << Width << std::endl;
    std::cout << "\tpercent: " << OnPercent << std::endl;
    std::cout << std::endl;

    {
        auto start = since_epoch();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> wdis(0, Width-1);

        const std::size_t amount_set = static_cast<std::size_t>(Width * OnPercent);

        for(std::size_t i=0; i<Amount; ++i) {
            std::bitset<Width> data;

            for(std::size_t j=0; j<amount_set; ++j) {
                data.set(wdis(gen));
            }

            fields.push_back(data);
        }

        auto end = since_epoch();

        std::cout << "generation took: " << (end-start) << "ms" << std::endl;
    }

    {
        auto start = since_epoch();

        for(auto & field : fields) {
            memory.insert(sdr::concept(field));
        }

        auto end = since_epoch();

        std::cout << "insertion took: " << (end-start) << "ms" << std::endl;
    }

    {
        auto start = since_epoch();

        memory.save_to_file("test.sdr");

        auto end = since_epoch();

        std::cout << "save to file took: " << (end-start) << "ms" << std::endl;
    }

    memory.clear();

    {
        auto start = since_epoch();

        const std::size_t storage_size { memory.load_from_file("test.sdr") };

        auto end = since_epoch();

        std::cout << "load from file (" << storage_size << " concepts) took: " << (end-start) << "ms" << std::endl;
    }

    {
        auto start = since_epoch();

        std::vector<std::future<std::vector<std::pair<sdr::position_t, std::size_t>>>> futures;

        for(sdr::position_t i=0; i < ParallelAmount; ++i) {
            futures.push_back(memory.async_closest(i, 10));
        }

        std::vector<std::vector<std::pair<sdr::position_t, std::size_t>>> closests;
        for(sdr::position_t i=0; i < ParallelAmount; ++i) {
            closests.push_back(futures[i].get());
        }

        auto end = since_epoch();

        std::cout << "finding closest (parallel => " << ParallelAmount << ") took: " << (end-start) << "ms" << std::endl;

        std::size_t counter = 0;
        for(auto & closest : closests) {
            std::cout << "\t#" << counter << std::endl;

            for(auto & it : closest) {
                std::cout << "\t\t" << it.first << "\t:: " << it.second << std::endl;
            }

            std::cout << std::endl;
            ++counter;
        }
    }

    {
        auto start = since_epoch();

        std::array<double, Width> weights;
        std::fill(weights.begin(), weights.end(), 1.1);
        auto closest = memory.weighted_closest(1, 10, weights);

        auto end = since_epoch();

        std::cout << "finding weighted closest took: " << (end-start) << "ms" << std::endl;

        std::cout << "closest: " << std::endl;
        for(auto & it : closest) {
            std::cout << "\t" << it.first << "\t:: " << it.second << std::endl;
        }
    }

    {
        std::vector<std::size_t> simlist(10);
        std::iota(simlist.begin(), simlist.end(), 1);

        auto start = since_epoch();

        auto similarity = memory.union_similarity(0, simlist);

        auto end = since_epoch();

        std::cout << "finding union similarity took: " << (end-start) << "ms" << std::endl;

        std::cout << "similarity: " << similarity << std::endl;
    }

    {
        auto start = since_epoch();

        auto matching = memory.matching(sdr::concept({ 0, 10 }));

        auto end = since_epoch();

        std::cout << "finding matching took: " << (end-start) << "ms" << std::endl;
        std::cout << "matching.size() => " << matching.size() << std::endl;

        std::cout << "matching: " << std::endl;
        for(auto & it : matching) {
            std::cout << it << std::endl;
        }
    }

    return 0;
}
