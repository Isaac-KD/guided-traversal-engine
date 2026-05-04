#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "PostingIterator.hpp"

struct TermInfo {
    float ub_bm25;
    uint64_t offset;
    uint32_t nb_postings;
};

class IndexReader {
private:
    std::unordered_map<uint32_t, TermInfo> vocab;
    int posting_fd;
    size_t posting_size;
    uint8_t* posting_map;

public:
    IndexReader();
    ~IndexReader();

    bool load(const std::string& vocab_path, const std::string& posting_path);
    
    // Returns true if term exists, and populates info
    bool get_term_info(uint32_t term_id, TermInfo& info) const;
    
    // Returns iterator. Assumes term exists.
    PostingIterator get_posting_iterator(const TermInfo& info) const;
};
