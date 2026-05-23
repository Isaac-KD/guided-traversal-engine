import os
import json
import re
import argparse
import subprocess
import math
from pathlib import Path

# Chemins vers les datasets
DATASETS = {
    'trec2019': 'Archive/2019/trec2019-queries.tsv',
    'trec2020': 'Archive/2020/msmarco-test2020-queries.tsv',
    'msmarco': 'Archive/msmarcoquery/queries.dev.small.tsv'
}

QRELS = {
    'trec2019': 'Archive/2019/trec2019-qrels.txt',
    'trec2020': 'Archive/2020/2020qrels-pass.txt',
    'msmarco': 'Archive/msmarcoquery/qrels.dev.small.tsv'
}

def load_vocab(vocab_path):
    print(f"Loading vocabulary from {vocab_path}...")
    with open(vocab_path, 'r') as f:
        return json.load(f)

def tokenize(text, vocab):
    # Passage en minuscules et extraction des mots alphanumériques
    words = re.findall(r'\w+', text.lower())
    token_ids = []
    for w in words:
        if w in vocab:
            token_ids.append(str(vocab[w]))
        else:
            # Token absent du vocabulaire
            pass
    return token_ids

def prepare_queries(dataset_name, vocab, output_path):
    dataset_path = DATASETS.get(dataset_name)
    if not dataset_path or not os.path.exists(dataset_path):
        raise FileNotFoundError(f"Dataset {dataset_name} not found at {dataset_path}")
    
    print(f"Preparing queries for {dataset_name}...")
    with open(dataset_path, 'r') as f_in, open(output_path, 'w') as f_out:
        for line in f_in:
            parts = line.strip().split('\t')
            if len(parts) >= 2:
                qid = parts[0]
                text = parts[1]
                token_ids = tokenize(text, vocab)
                if token_ids:
                    f_out.write(f"{qid} {' '.join(token_ids)}\n")
    print(f"Tokenized queries saved to {output_path}")

def build_engine():
    print("Building Guided Traversal Engine...")
    os.makedirs('build', exist_ok=True)
    subprocess.run(['cmake', '..'], cwd='build', check=True)
    subprocess.run(['make'], cwd='build', check=True)

def load_qrels(dataset_name):
    qrels_path = QRELS.get(dataset_name)
    qrels = {}
    with open(qrels_path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 4:
                qid = parts[0]
                docid = parts[2]
                rel = int(parts[3])
                if rel > 0:
                    if qid not in qrels:
                        qrels[qid] = {}
                    qrels[qid][docid] = rel
    return qrels

def evaluate_metrics(run_file, qrels, k=10):
    run = {}
    with open(run_file, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) >= 6:
                qid = parts[0]
                docid = parts[2]
                rank = int(parts[3])
                if qid not in run:
                    run[qid] = []
                run[qid].append((rank, docid))
                
    for qid in run:
        run[qid].sort(key=lambda x: x[0])
        
    mrr_sum = 0.0
    ndcg_sum = 0.0
    evaluated_queries = 0
    
    for qid in qrels:
        if qid not in run:
            continue
        evaluated_queries += 1
        
        # Calcul du MRR@k
        mrr = 0.0
        for i, (rank, docid) in enumerate(run[qid][:k]):
            if docid in qrels[qid]:
                mrr = 1.0 / (i + 1)
                break
        mrr_sum += mrr
        
        # Calcul du NDCG@k
        dcg = 0.0
        for i, (rank, docid) in enumerate(run[qid][:k]):
            rel = qrels[qid].get(docid, 0)
            if rel > 0:
                dcg += (2**rel - 1) / math.log2(i + 2)
                
        ideal_rels = sorted(qrels[qid].values(), reverse=True)
        idcg = 0.0
        for i, rel in enumerate(ideal_rels[:k]):
            idcg += (2**rel - 1) / math.log2(i + 2)
            
        ndcg = dcg / idcg if idcg > 0 else 0.0
        ndcg_sum += ndcg
        
    avg_mrr = mrr_sum / evaluated_queries if evaluated_queries > 0 else 0.0
    avg_ndcg = ndcg_sum / evaluated_queries if evaluated_queries > 0 else 0.0
    
    return avg_mrr, avg_ndcg, evaluated_queries

def run_evaluation(binary_path, vocab_bin, posting_bin, queries_file, run_name, output_file, qrels, dataset):
    print(f"\nRunning evaluation: {run_name}...")
    cmd = [binary_path, vocab_bin, posting_bin, queries_file, run_name, output_file]
    
    mean_lat, med_lat, p99_lat = 0.0, 0.0, 0.0
    
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, text=True)
    for line in process.stdout:
        line_str = line.strip()
        if "Index loaded" in line_str or "Warming up" in line_str:
            print(line_str)
        elif "Mean Latency" in line_str:
            mean_lat = float(line_str.split(':')[1].strip().split()[0])
        elif "Median Latency" in line_str:
            med_lat = float(line_str.split(':')[1].strip().split()[0])
        elif "P99 Latency" in line_str:
            p99_lat = float(line_str.split(':')[1].strip().split()[0])
            
    process.wait()
    if process.returncode != 0:
        raise subprocess.CalledProcessError(process.returncode, cmd)
            
    mrr, ndcg, count = evaluate_metrics(output_file, qrels)
    
    print(f"--- Results for {run_name} ---")
    print(f"Mean Latency   : {mean_lat:.1f} ms")
    print(f"Median Latency : {med_lat:.1f} ms")
    print(f"P99 Latency    : {p99_lat:.1f} ms")
    print(f"MRR@10         : {mrr:.4f}")
    print(f"NDCG@10        : {ndcg:.4f}")

def main():
    parser = argparse.ArgumentParser(description="Evaluate Guided Traversal Engine on TREC/MSMARCO datasets.")
    parser.add_argument('--dataset', type=str, choices=list(DATASETS.keys()), required=True, help='Dataset to evaluate')
    parser.add_argument('--skip-build', action='store_true', help='Skip building the C++ engine')
    parser.add_argument('--algo', type=str, choices=['GT', 'GTI', 'BOTH'], default='BOTH', help='Algorithm to use')
    
    args = parser.parse_args()

    if not args.skip_build:
        build_engine()

    vocab = load_vocab('output/term2id_expanded.json')
    
    os.makedirs('runs', exist_ok=True)
    queries_file = f"runs/{args.dataset}_queries_tokenized.txt"
    
    prepare_queries(args.dataset, vocab, queries_file)
    
    binary_path = 'build/guided_traversal_engine'
    vocab_bin = 'output/term_meta.bin'
    posting_bin = 'output/term_doc_scores.bin'
    
    qrels = load_qrels(args.dataset)
    
    print("\n========================================================")
    print(f"Evaluating Dataset: {args.dataset}")
    print("========================================================")
    
    if args.algo in ['GT', 'BOTH']:
        run_name = f"{args.dataset}_GT"
        output_file = f"runs/run_{run_name}.txt"
        run_evaluation(binary_path, vocab_bin, posting_bin, queries_file, run_name, output_file, qrels, args.dataset)
        
    if args.algo in ['GTI', 'BOTH']:
        run_name = f"{args.dataset}_GTI"
        output_file = f"runs/run_{run_name}.txt"
        run_evaluation(binary_path, vocab_bin, posting_bin, queries_file, run_name, output_file, qrels, args.dataset)

if __name__ == '__main__':
    main()
