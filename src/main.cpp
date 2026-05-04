#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include "IndexReader.hpp"
#include "MaxScoreGT.hpp"
#include "Metrics.hpp"

// Utility to split strings
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <vocab.bin> <posting.bin> [queries_file] [run_name] [output_file]" << std::endl;
        return 1;
    }

    std::string vocab_path = argv[1];
    std::string posting_path = argv[2];

    IndexReader index;
    if (!index.load(vocab_path, posting_path)) {
        std::cerr << "Failed to load index." << std::endl;
        return 1;
    }
    std::cout << "Index loaded successfully." << std::endl;

    if (argc >= 6) {
        // Batch Evaluation Mode
        std::string queries_file = argv[3];
        std::string run_name = argv[4];
        std::string output_file = argv[5];

        std::ifstream q_in(queries_file);
        if (!q_in.is_open()) {
            std::cerr << "Could not open queries file: " << queries_file << std::endl;
            return 1;
        }

        std::ofstream q_out(output_file);
        if (!q_out.is_open()) {
            std::cerr << "Could not open output file: " << output_file << std::endl;
            return 1;
        }

        std::string line;
        std::vector<std::pair<std::string, std::vector<uint32_t>>> queries;

        while (std::getline(q_in, line)) {
            std::vector<std::string> parts = split(line, ' ');
            if (parts.size() < 2) continue;

            std::string qid = parts[0];
            std::vector<uint32_t> query;
            for (size_t i = 1; i < parts.size(); ++i) {
                query.push_back(std::stoul(parts[i]));
            }
            queries.push_back({qid, query});
        }
        q_in.close();

        bool use_gti = (run_name.find("GTI") != std::string::npos);

        // --- Warmup Pass (same k as real run to load all needed pages) ---
        std::cout << "Warming up index cache in RAM..." << std::endl;
        for (const auto& q : queries) {
            MaxScoreGT::search(index, q.second, 1000, use_gti);
        }

        // --- Evaluation Pass ---
        std::cout << "Running queries..." << std::endl;
        int query_count = 0;
        std::vector<double> latencies;

        for (const auto& q : queries) {
            auto start = std::chrono::high_resolution_clock::now();
            auto results = MaxScoreGT::search(index, q.second, 1000, use_gti);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> diff = end - start;
            latencies.push_back(diff.count());

            for (size_t i = 0; i < results.size(); ++i) {
                q_out << q.first << " Q0 " << results[i].doc_id << " " << (i + 1) << " " << results[i].score << " " << run_name << "\n";
            }
            query_count++;
            if (query_count % 100 == 0) {
                std::cout << "Processed " << query_count << " queries..." << std::endl;
            }
        }

        auto metrics = Metrics::compute_latency_metrics(latencies);
        std::cout << "Done evaluating " << query_count << " queries." << std::endl;
        std::cout << "Mean Latency   : " << metrics.mean << " ms" << std::endl;
        std::cout << "Median Latency : " << metrics.median << " ms" << std::endl;
        std::cout << "P99 Latency    : " << metrics.p99 << " ms" << std::endl;
        
        q_out.close();
        return 0;
    }

    // Interactive Mode
    std::cout << "Enter query term IDs separated by space (Ctrl+D to exit):" << std::endl;
    
    std::vector<uint32_t> query;
    uint32_t term_id;
    
    std::vector<double> latencies_gt;
    std::vector<double> latencies_gti;

    while (std::cin >> term_id) {
        query.push_back(term_id);
        if (std::cin.peek() == '\n' || std::cin.peek() == EOF) {
            if (!query.empty()) {
                auto start_gt = std::chrono::high_resolution_clock::now();
                auto results = MaxScoreGT::search(index, query, 10, false); // Increased to 10 for interactive
                auto end_gt = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> diff_gt = end_gt - start_gt;
                latencies_gt.push_back(diff_gt.count());

                std::cout << "Top 10 Results (GT) [" << diff_gt.count() << " ms]:" << std::endl;
                for (size_t i = 0; i < results.size(); ++i) {
                    std::cout << i + 1 << ". Doc " << results[i].doc_id 
                              << " Score: " << results[i].score << std::endl;
                }

                auto start_gti = std::chrono::high_resolution_clock::now();
                auto results_gti = MaxScoreGT::search(index, query, 10, true); // Increased to 10 for interactive
                auto end_gti = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> diff_gti = end_gti - start_gti;
                latencies_gti.push_back(diff_gti.count());

                std::cout << "Top 10 Results (GTI) [" << diff_gti.count() << " ms]:" << std::endl;
                for (size_t i = 0; i < results_gti.size(); ++i) {
                    std::cout << i + 1 << ". Doc " << results_gti[i].doc_id 
                              << " Score: " << results_gti[i].score << std::endl;
                }
                query.clear();
            }
        }
    }

    if (!latencies_gt.empty()) {
        auto metrics_gt = Metrics::compute_latency_metrics(latencies_gt);
        auto metrics_gti = Metrics::compute_latency_metrics(latencies_gti);

        std::cout << "\n--- Latency Metrics ---" << std::endl;
        std::cout << "GT Strategy: " << latencies_gt.size() << " queries" << std::endl;
        std::cout << "  Mean   : " << metrics_gt.mean << " ms" << std::endl;
        std::cout << "  Median : " << metrics_gt.median << " ms" << std::endl;
        std::cout << "  P99    : " << metrics_gt.p99 << " ms" << std::endl;
        
        std::cout << "\nGTI Strategy: " << latencies_gti.size() << " queries" << std::endl;
        std::cout << "  Mean   : " << metrics_gti.mean << " ms" << std::endl;
        std::cout << "  Median : " << metrics_gti.median << " ms" << std::endl;
        std::cout << "  P99    : " << metrics_gti.p99 << " ms" << std::endl;
    }

    return 0;
}
