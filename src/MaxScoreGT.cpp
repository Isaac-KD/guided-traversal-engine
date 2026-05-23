#include "MaxScoreGT.hpp"
#include <algorithm>
#include <queue>
#include <limits>

struct QueryTerm {
    TermInfo info;
    PostingIterator it;
    uint32_t term_id;
};

std::vector<TopResult> MaxScoreGT::search(
    const IndexReader& index,
    const std::vector<uint32_t>& query,
    int k,
    bool use_gti) 
{
    std::vector<QueryTerm> terms;
    for (uint32_t tid : query) {
        TermInfo info;
        if (index.get_term_info(tid, info)) {
            terms.push_back({info, index.get_posting_iterator(info), tid});
        }
    }

    if (terms.empty()) return {};

    // Tri des termes par borne supérieure de BM25 croissante
    std::sort(terms.begin(), terms.end(), [](const QueryTerm& a, const QueryTerm& b) {
        return a.info.ub_bm25 < b.info.ub_bm25;
    });

    int n_terms = terms.size();
    std::vector<float> prefix_ub(n_terms, 0.0f);
    prefix_ub[0] = terms[0].info.ub_bm25;
    for (int i = 1; i < n_terms; ++i) {
        prefix_ub[i] = prefix_ub[i - 1] + terms[i].info.ub_bm25;
    }

    std::priority_queue<TopResult> heap_bm25;
    std::priority_queue<TopResult> heap_di;

    float theta_bm25 = 0.0f;
    int pivot = 0;
    
    // Recherche du premier document à évaluer
    uint32_t current_doc = std::numeric_limits<uint32_t>::max();
    for (int i = 0; i < n_terms; ++i) {
        if (!terms[i].it.is_done()) {
            current_doc = std::min(current_doc, terms[i].it.doc_id());
        }
    }

    while (pivot < n_terms && current_doc != std::numeric_limits<uint32_t>::max()) {
        float score_bm25 = 0.0f;
        float score_di = 0.0f;
        uint32_t next_doc = std::numeric_limits<uint32_t>::max();

        // Évaluation des termes essentiels (>= pivot)
        for (int i = pivot; i < n_terms; ++i) {
            terms[i].it.next_GEQ(current_doc);
            if (!terms[i].it.is_done()) {
                if (terms[i].it.doc_id() == current_doc) {
                    const auto& p = terms[i].it.get_posting();
                    score_bm25 += p.bm25;
                    score_di += p.deepimpact;
                }
            }
        }

        // Évaluation des termes non essentiels (< pivot) si la borne supérieure le permet
        float max_possible_bm25 = score_bm25 + (pivot > 0 ? prefix_ub[pivot - 1] : 0.0f);
        if (max_possible_bm25 > theta_bm25) {
            for (int i = pivot - 1; i >= 0; --i) {
                float max_rem = score_bm25 + prefix_ub[i];
                if (max_rem <= theta_bm25) break;

                terms[i].it.next_GEQ(current_doc);
                if (!terms[i].it.is_done() && terms[i].it.doc_id() == current_doc) {
                    const auto& p = terms[i].it.get_posting();
                    score_bm25 += p.bm25;
                    score_di += p.deepimpact;
                }
            }

            // Mise à jour du tas BM25
            if (heap_bm25.size() < (size_t)k || score_bm25 > theta_bm25) {
                heap_bm25.push({current_doc, score_bm25});
                if (heap_bm25.size() > (size_t)k) {
                    heap_bm25.pop();
                }
                theta_bm25 = heap_bm25.size() == (size_t)k ? heap_bm25.top().score : 0.0f;
                
                // Ajustement du pivot
                while (pivot < n_terms && prefix_ub[pivot] <= theta_bm25) {
                    pivot++;
                }
            }
        }

        // Guided Traversal : mise à jour du tas DeepImpact pour tous les documents visités
        float final_di_score = use_gti ? (score_bm25 + score_di) : score_di;
        heap_di.push({current_doc, final_di_score});
        if (heap_di.size() > (size_t)k) {
            heap_di.pop();
        }

        // Recherche du document suivant parmi les termes essentiels
        for (int i = pivot; i < n_terms; ++i) {
            terms[i].it.next_GEQ(current_doc + 1);
            if (!terms[i].it.is_done()) {
                next_doc = std::min(next_doc, terms[i].it.doc_id());
            }
        }
        current_doc = next_doc;
    }

    // Récupération des K meilleurs résultats du tas DeepImpact
    std::vector<TopResult> results;
    while (!heap_di.empty()) {
        results.push_back(heap_di.top());
        heap_di.pop();
    }
    std::reverse(results.begin(), results.end());
    return results;
}
