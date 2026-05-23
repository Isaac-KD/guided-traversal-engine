#include "PostingIterator.hpp"
#include "Float16.hpp"
#include <cstring>

PostingIterator::PostingIterator(const uint8_t* ptr, uint32_t nb_postings)
    : base_ptr(ptr), total_postings(nb_postings), current_idx(0) {
    if (!is_done()) {
        read_current();
    }
}

void PostingIterator::read_current() {
    if (is_done()) return;
    const uint8_t* p = base_ptr + current_idx * 8;
    
    std::memcpy(&current_posting.doc_id, p, 4);
    
    uint16_t h_bm25, h_di;
    std::memcpy(&h_bm25, p + 4, 2);
    std::memcpy(&h_di, p + 6, 2);
    
    current_posting.bm25 = utils::half_to_float(h_bm25);
    current_posting.deepimpact = utils::half_to_float(h_di);
}

void PostingIterator::next() {
    if (is_done()) return;
    current_idx++;
    read_current();
}

void PostingIterator::next_GEQ(uint32_t target_doc_id) {
    if (is_done() || current_posting.doc_id >= target_doc_id) return;

    // Recherche dichotomique pour target_doc_id
    uint32_t low = current_idx;
    uint32_t high = total_postings - 1;
    uint32_t ans = total_postings;

    while (low <= high) {
        uint32_t mid = low + (high - low) / 2;
        uint32_t mid_doc_id;
        std::memcpy(&mid_doc_id, base_ptr + mid * 8, 4);

        if (mid_doc_id >= target_doc_id) {
            ans = mid;
            if (mid == 0) break;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    current_idx = ans;
    read_current();
}
