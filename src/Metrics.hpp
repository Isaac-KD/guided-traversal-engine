#pragma once
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include "MaxScoreGT.hpp"

struct LatencyMetrics {
    double mean;
    double median;
    double p99;
};

class Metrics {
public:
    // Calcul du Reciprocal Rank à K (1.0 / rang du premier doc pertinent).
    static double compute_rr(const std::vector<TopResult>& results, const std::unordered_set<uint32_t>& relevant_docs, int k = 10);
    
    // Calcul du NDCG à K (Normalized Discounted Cumulative Gain).
    static double compute_ndcg(const std::vector<TopResult>& results, const std::unordered_map<uint32_t, int>& doc_relevances, int k = 10);
    
    // Calcul des métriques de latence en ms (moyenne, médiane, p99).
    static LatencyMetrics compute_latency_metrics(std::vector<double> latencies);
};
