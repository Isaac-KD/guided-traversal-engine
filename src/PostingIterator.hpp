#pragma once
#include <cstdint>

struct Posting {
    uint32_t doc_id;
    float bm25;
    float deepimpact;
};

class PostingIterator {
private:
    const uint8_t* base_ptr;
    uint32_t total_postings;
    uint32_t current_idx;
    Posting current_posting;

    void read_current();

public:
    // Constructeur par défaut (utile pour l'utilisation dans des conteneurs)
    PostingIterator() : base_ptr(nullptr), total_postings(0), current_idx(0) {}
    PostingIterator(const uint8_t* ptr, uint32_t nb_postings);

    // Avancer au document suivant
    void next();

    // Avancer jusqu'au premier document >= target_doc_id
    void next_GEQ(uint32_t target_doc_id);

    // Récupérer la posting courante
    const Posting& get_posting() const { return current_posting; }

    // Vérifier si la fin est atteinte
    bool is_done() const { return current_idx >= total_postings; }

    uint32_t doc_id() const { return current_posting.doc_id; }
};
