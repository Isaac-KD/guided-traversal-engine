"""Diagnose potential causes of NDCG gap vs article."""
import json
import re

# Load vocab
with open('output/term2id_expanded.json') as f:
    vocab = json.load(f)

print(f"Vocabulary size: {len(vocab)} terms")

# Check query term coverage for each dataset
datasets = {
    'trec2019': 'Archive/2019/trec2019-queries.tsv',
    'trec2020': 'Archive/2020/msmarco-test2020-queries.tsv',
}

for name, path in datasets.items():
    total_terms = 0
    found_terms = 0
    dropped_terms = []
    queries_with_drops = 0
    total_queries = 0
    
    with open(path) as f:
        for line in f:
            parts = line.strip().split('\t')
            if len(parts) < 2:
                continue
            qid, text = parts[0], parts[1]
            tokens = re.findall(r'\w+', text.lower())
            total_queries += 1
            has_drop = False
            for t in tokens:
                total_terms += 1
                if t in vocab:
                    found_terms += 1
                else:
                    dropped_terms.append(t)
                    has_drop = True
            if has_drop:
                queries_with_drops += 1
    
    coverage = found_terms / total_terms * 100 if total_terms > 0 else 0
    print(f"\n=== {name} ===")
    print(f"Queries: {total_queries}")
    print(f"Total query terms: {total_terms}")
    print(f"Found in vocab: {found_terms} ({coverage:.1f}%)")
    print(f"Dropped: {total_terms - found_terms}")
    print(f"Queries with at least 1 dropped term: {queries_with_drops}/{total_queries}")
    if dropped_terms:
        from collections import Counter
        top_dropped = Counter(dropped_terms).most_common(15)
        print(f"Top dropped terms: {top_dropped}")

# Check tokenized queries vs original
print("\n=== Tokenized query comparison (first 5) ===")
with open('Archive/2019/trec2019-queries.tsv') as f:
    orig_lines = f.readlines()[:5]
with open('runs/trec2019_queries_tokenized.txt') as f:
    tok_lines = f.readlines()[:5]

for orig, tok in zip(orig_lines, tok_lines):
    orig_parts = orig.strip().split('\t')
    tok_parts = tok.strip().split(' ')
    qid = orig_parts[0]
    orig_tokens = re.findall(r'\w+', orig_parts[1].lower())
    tok_term_ids = tok_parts[1:]
    print(f"  Q {qid}: {len(orig_tokens)} tokens → {len(tok_term_ids)} term IDs (dropped {len(orig_tokens) - len(tok_term_ids)})")
    if len(orig_tokens) != len(tok_term_ids):
        found = set()
        for t in orig_tokens:
            if t in vocab:
                found.add(t)
        missing = [t for t in orig_tokens if t not in found]
        print(f"    Missing: {missing}")
