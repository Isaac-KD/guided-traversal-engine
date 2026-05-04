import json
import sys

with open('output/term2id_expanded.json', 'r') as f:
    data = json.load(f)

# Print first 20 items
count = 0
for k, v in data.items():
    print(f"{repr(k)}: {v}")
    count += 1
    if count >= 20:
        break
