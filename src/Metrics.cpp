#include "Metrics.hpp"
#include <algorithm>
#include <cmath>

double Metrics::compute_rr(const std::vector<TopResult>& results, const std::unordered_set<uint32_t>& relevant_docs, int k) {
    int limit = std::min((int)results.size(), k);
    for (int i = 0; i < limit; ++i) {
        if (relevant_docs.find(results[i].doc_id) != relevant_docs.end()) {
            return 1.0 / (i + 1.0);
        }
    }
    return 0.0;
}

double Metrics::compute_ndcg(const std::vector<TopResult>& results, const std::unordered_map<uint32_t, int>& doc_relevances, int k) {
    int limit = std::min((int)results.size(), k);
    double dcg = 0.0;
    
    for (int i = 0; i < limit; ++i) {
        auto it = doc_relevances.find(results[i].doc_id);
        if (it != doc_relevances.end()) {
            int rel = it->second;
            dcg += (std::pow(2.0, rel) - 1.0) / std::log2(i + 2.0); // log2(rang + 1)
        }
    }

    if (dcg == 0.0) return 0.0;

    // DCG Idéal
    std::vector<int> ideal_rels;
    for (const auto& kv : doc_relevances) {
        ideal_rels.push_back(kv.second);
    }
    std::sort(ideal_rels.begin(), ideal_rels.end(), std::greater<int>());
    
    double idcg = 0.0;
    int idcg_limit = std::min((int)ideal_rels.size(), k);
    for (int i = 0; i < idcg_limit; ++i) {
        idcg += (std::pow(2.0, ideal_rels[i]) - 1.0) / std::log2(i + 2.0);
    }

    return idcg > 0.0 ? dcg / idcg : 0.0;
}

LatencyMetrics Metrics::compute_latency_metrics(std::vector<double> latencies) {
    LatencyMetrics metrics = {0.0, 0.0, 0.0};
    if (latencies.empty()) return metrics;

    double sum = 0.0;
    for (double l : latencies) {
        sum += l;
    }
    metrics.mean = sum / latencies.size();

    std::sort(latencies.begin(), latencies.end());
    
    // Médiane
    size_t n = latencies.size();
    if (n % 2 == 0) {
        metrics.median = (latencies[n / 2 - 1] + latencies[n / 2]) / 2.0;
    } else {
        metrics.median = latencies[n / 2];
    }
    
    // Percentile 99 (P99)
    size_t p99_idx = std::ceil(0.99 * n) - 1;
    // Borner l'index du P99
    p99_idx = std::max((size_t)0, std::min(p99_idx, n - 1));
    metrics.p99 = latencies[p99_idx];

    return metrics;
}
