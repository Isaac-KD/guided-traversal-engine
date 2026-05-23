#pragma once
#include <vector>
#include "IndexReader.hpp"

struct TopResult {
    uint32_t doc_id;
    float score;
    bool operator<(const TopResult& other) const {
        // Tas-min basé sur le score
        return score > other.score; 
    }
};

class MaxScoreGT {
public:
    // Recherche les K meilleurs résultats triés par score décroissant.
    // Si use_gti est vrai, le score est BM25 + DeepImpact.
    // Sinon, c'est uniquement DeepImpact.
    static std::vector<TopResult> search(
        const IndexReader& index,
        const std::vector<uint32_t>& query,
        int k = 10,
        bool use_gti = false
    );
};
