from collections import Counter

for run_file in ['runs/run_trec2019_GT.txt', 'runs/run_trec2019_GTI.txt']:
    counts = Counter()
    with open(run_file) as f:
        for line in f:
            qid = line.strip().split()[0]
            counts[qid] += 1
    print(f"\n{run_file}:")
    print(f"  Total lines: {sum(counts.values())}")
    print(f"  Queries: {len(counts)}")
    print(f"  Results per query (first 5): {list(counts.items())[:5]}")
    
    # Check scores for first query
    with open(run_file) as f:
        lines = [l.strip() for l in f if l.strip()]
    first_qid = lines[0].split()[0]
    first_q_lines = [l for l in lines if l.split()[0] == first_qid]
    print(f"  Query {first_qid}: {len(first_q_lines)} results, score range: {first_q_lines[-1].split()[4]} to {first_q_lines[0].split()[4]}")

# Compare top-10 for one query between GT and GTI
print("\n--- Comparison of top-10 for first query ---")
for run_file in ['runs/run_trec2019_GT.txt', 'runs/run_trec2019_GTI.txt']:
    with open(run_file) as f:
        lines = [l.strip() for l in f if l.strip()]
    first_qid = lines[0].split()[0]
    first_q = [l for l in lines if l.split()[0] == first_qid][:10]
    print(f"\n{run_file} top-10:")
    for l in first_q:
        parts = l.split()
        print(f"  rank={parts[3]:>4} doc={parts[2]:>10} score={parts[4]}")
