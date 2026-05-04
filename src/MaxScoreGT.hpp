#pragma once
#include <vector>
#include "IndexReader.hpp"

struct TopResult {
    uint32_t doc_id;
    float score;
    bool operator<(const TopResult& other) const {
        // min-heap based on score
        return score > other.score; 
    }
};

class MaxScoreGT {
public:
    // search returns top K results sorted by score descending.
    // if use_gti is true, the score is BM25 + DeepImpact.
    // otherwise, the score is pure DeepImpact.
    static std::vector<TopResult> search(
        const IndexReader& index,
        const std::vector<uint32_t>& query,
        int k = 10,
        bool use_gti = false
    );
};
