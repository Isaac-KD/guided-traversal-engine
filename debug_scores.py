"""Deep diagnosis of score distributions and metric calculations."""
import math
from collections import defaultdict

def load_qrels(path):
    qrels = {}
    with open(path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 4:
                qid, _, docid, rel = parts[0], parts[1], parts[2], int(parts[3])
                if qid not in qrels:
                    qrels[qid] = {}
                qrels[qid][docid] = rel
    return qrels

def load_run(path):
    run = defaultdict(list)
    with open(path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 6:
                qid, docid, rank, score = parts[0], parts[2], int(parts[3]), float(parts[4])
                run[qid].append((rank, docid, score))
    for qid in run:
        run[qid].sort(key=lambda x: x[0])
    return run

qrels = load_qrels('Archive/2019/trec2019-qrels.txt')
gt_run = load_run('runs/run_trec2019_GT.txt')
gti_run = load_run('runs/run_trec2019_GTI.txt')

print("=== QRELS Analysis ===")
print(f"Queries in QRELS: {len(qrels)}")
# Check which qrels queries are in the runs
common_gt = set(qrels.keys()) & set(gt_run.keys())
common_gti = set(qrels.keys()) & set(gti_run.keys())
print(f"Queries in GT run matching QRELS: {len(common_gt)}")
print(f"Queries in GTI run matching QRELS: {len(common_gti)}")

# Check relevance levels distribution
all_rels = []
for qid in qrels:
    for docid, rel in qrels[qid].items():
        all_rels.append(rel)
from collections import Counter
print(f"Relevance level distribution: {Counter(all_rels)}")

print("\n=== Per-Query Deep Dive (first 5 queries) ===")
for qid in sorted(common_gt)[:5]:
    gt_top10 = gt_run[qid][:10]
    gti_top10 = gti_run[qid][:10]
    
    rel_docs = {d: r for d, r in qrels[qid].items() if r > 0}
    print(f"\nQuery {qid}: {len(rel_docs)} relevant docs in QRELS")
    
    # Show relevance levels
    rel_levels = Counter(qrels[qid].values())
    print(f"  Relevance levels: {dict(rel_levels)}")
    
    print(f"  GT top-10:")
    gt_mrr = 0.0
    for i, (rank, docid, score) in enumerate(gt_top10):
        rel = qrels[qid].get(docid, -1)  # -1 = not judged
        marker = ""
        if rel > 0:
            marker = f" *** RELEVANT (rel={rel}) ***"
            if gt_mrr == 0:
                gt_mrr = 1.0 / (i + 1)
        elif rel == 0:
            marker = " (judged non-relevant)"
        else:
            marker = " (unjudged)"
        print(f"    {i+1}. doc={docid:>10} score={score:.6f}{marker}")
    
    print(f"  GTI top-10:")
    gti_mrr = 0.0
    for i, (rank, docid, score) in enumerate(gti_top10):
        rel = qrels[qid].get(docid, -1)
        marker = ""
        if rel > 0:
            marker = f" *** RELEVANT (rel={rel}) ***"
            if gti_mrr == 0:
                gti_mrr = 1.0 / (i + 1)
        elif rel == 0:
            marker = " (judged non-relevant)"
        else:
            marker = " (unjudged)"
        print(f"    {i+1}. doc={docid:>10} score={score:.6f}{marker}")
    
    print(f"  GT MRR for this query: {gt_mrr:.4f}")
    print(f"  GTI MRR for this query: {gti_mrr:.4f}")
    
    # Check: are relevant docs anywhere in GT top-1000?
    gt_all_docs = {docid for _, docid, _ in gt_run[qid]}
    gti_all_docs = {docid for _, docid, _ in gti_run[qid]}
    gt_found = sum(1 for d in rel_docs if d in gt_all_docs)
    gti_found = sum(1 for d in rel_docs if d in gti_all_docs)
    print(f"  Relevant docs found in GT top-1000: {gt_found}/{len(rel_docs)}")
    print(f"  Relevant docs found in GTI top-1000: {gti_found}/{len(rel_docs)}")

print("\n=== Score Distribution ===")
# Check score ranges for GT and GTI
all_gt_scores = [s for qid in gt_run for _, _, s in gt_run[qid][:10]]
all_gti_scores = [s for qid in gti_run for _, _, s in gti_run[qid][:10]]
print(f"GT top-10 scores:  min={min(all_gt_scores):.4f} max={max(all_gt_scores):.4f} mean={sum(all_gt_scores)/len(all_gt_scores):.4f}")
print(f"GTI top-10 scores: min={min(all_gti_scores):.4f} max={max(all_gti_scores):.4f} mean={sum(all_gti_scores)/len(all_gti_scores):.4f}")

# Check overlap between GT and GTI top-10
print("\n=== Overlap Analysis ===")
overlaps = []
for qid in common_gt:
    gt_docs = set(d for _, d, _ in gt_run[qid][:10])
    gti_docs = set(d for _, d, _ in gti_run[qid][:10])
    overlap = len(gt_docs & gti_docs)
    overlaps.append(overlap)
print(f"Average top-10 overlap between GT and GTI: {sum(overlaps)/len(overlaps):.1f}/10")
