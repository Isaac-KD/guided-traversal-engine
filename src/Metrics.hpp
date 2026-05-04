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
    // Calculates Reciprocal Rank at K. Returns 1.0 / rank of the first relevant document.
    static double compute_rr(const std::vector<TopResult>& results, const std::unordered_set<uint32_t>& relevant_docs, int k = 10);
    
    // Calculates Normalized Discounted Cumulative Gain at K.
    static double compute_ndcg(const std::vector<TopResult>& results, const std::unordered_map<uint32_t, int>& doc_relevances, int k = 10);
    
    // Calculates Mean, Median, and 99th percentile of latencies (in milliseconds)
    static LatencyMetrics compute_latency_metrics(std::vector<double> latencies);
};
