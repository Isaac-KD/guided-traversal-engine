"""Compare our NDCG calculation with a reference implementation."""
import math
from collections import defaultdict

def load_qrels_full(path):
    """Load ALL qrels including rel=0."""
    qrels = {}
    with open(path) as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 4:
                qid, docid, rel = parts[0], parts[2], int(parts[3])
                if qid not in qrels:
                    qrels[qid] = {}
                qrels[qid][docid] = rel
    return qrels

def load_run(path):
    run = defaultdict(list)
    with open(path) as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 6:
                qid, docid, rank = parts[0], parts[2], int(parts[3])
                run[qid].append((rank, docid))
    for qid in run:
        run[qid].sort(key=lambda x: x[0])
    return run

def ndcg_at_k(run_list, qrel_dict, k=10):
    """Standard NDCG@k."""
    dcg = 0.0
    for i, (rank, docid) in enumerate(run_list[:k]):
        rel = qrel_dict.get(docid, 0)
        if rel > 0:
            dcg += (2**rel - 1) / math.log2(i + 2)
    
    # Ideal: sort all qrel rels descending
    ideal_rels = sorted([r for r in qrel_dict.values() if r > 0], reverse=True)
    idcg = 0.0
    for i, rel in enumerate(ideal_rels[:k]):
        idcg += (2**rel - 1) / math.log2(i + 2)
    
    return dcg / idcg if idcg > 0 else 0.0

# Test with both datasets
for dataset, qrels_path, run_gt, run_gti in [
    ('trec2019', 'Archive/2019/trec2019-qrels.txt', 'runs/run_trec2019_GT.txt', 'runs/run_trec2019_GTI.txt'),
    ('trec2020', 'Archive/2020/2020qrels-pass.txt', 'runs/run_trec2020_GT.txt', 'runs/run_trec2020_GTI.txt'),
]:
    qrels = load_qrels_full(qrels_path)
    
    # Count queries with relevant docs
    queries_with_rel = {qid for qid, docs in qrels.items() if any(r > 0 for r in docs.values())}
    
    for run_path, label in [(run_gt, 'GT'), (run_gti, 'GTI')]:
        run = load_run(run_path)
        
        ndcg_sum = 0.0
        mrr_sum = 0.0
        count = 0
        
        per_query = []
        
        for qid in queries_with_rel:
            if qid not in run:
                continue
            count += 1
            
            n = ndcg_at_k(run[qid], qrels[qid], k=10)
            ndcg_sum += n
            
            # MRR
            mrr = 0.0
            for i, (rank, docid) in enumerate(run[qid][:10]):
                if qrels[qid].get(docid, 0) > 0:
                    mrr = 1.0 / (i + 1)
                    break
            mrr_sum += mrr
            
            per_query.append((qid, n, mrr))
        
        avg_ndcg = ndcg_sum / count if count > 0 else 0
        avg_mrr = mrr_sum / count if count > 0 else 0
        
        print(f"\n{dataset} {label}: NDCG@10={avg_ndcg:.4f}, MRR@10={avg_mrr:.4f} ({count} queries)")
        
        # Show worst queries
        per_query.sort(key=lambda x: x[1])
        print(f"  Worst 5 NDCG queries:")
        for qid, n, m in per_query[:5]:
            rel_docs = sum(1 for r in qrels[qid].values() if r > 0)
            found_in_top10 = sum(1 for _, d in run[qid][:10] if qrels[qid].get(d, 0) > 0)
            print(f"    Q{qid}: NDCG={n:.4f} MRR={m:.4f} (rel_docs={rel_docs}, found_top10={found_in_top10})")
