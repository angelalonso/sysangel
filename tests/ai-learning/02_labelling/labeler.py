import json
import random
from pathlib import Path

# Paths
WORDS_FILE = Path("words.txt")  # Plain text, one word per line
LABELS_FILE = Path("word_sentiments.json")  # Stores {word: {"score": 0.5, "count": 2}}

# Load or initialize labels
if LABELS_FILE.exists():
    with open(LABELS_FILE, 'r') as f:
        labels_db = json.load(f)
else:
    labels_db = {}

# Load base word list
with open(WORDS_FILE, 'r') as f:
    all_words = [line.strip() for line in f if line.strip()]

# Interactive labeling
while True:
    # Prioritize unlabeled or rarely labeled words
    candidate_words = [
        word for word in all_words 
        if word not in labels_db or labels_db[word]["count"] < 3
    ]
    if not candidate_words:
        print("\nAll words labeled sufficiently!")
        break

    word = random.choice(candidate_words)
    print(f"\nWord: {word}")
    response = input("1=Positive, 0=Neutral, -1=Negative, q=Quit: ").strip().lower()

    if response == 'q':
        break
    if response not in {'1', '0', '-1'}:
        continue

    # Update label (weighted average)
    sentiment = float(response)
    if word not in labels_db:
        labels_db[word] = {"score": sentiment, "count": 1}
    else:
        old_score = labels_db[word]["score"]
        old_count = labels_db[word]["count"]
        labels_db[word]["score"] = (old_score * old_count + sentiment) / (old_count + 1)
        labels_db[word]["count"] += 1

    # Save after each update
    with open(LABELS_FILE, 'w') as f:
        json.dump(labels_db, f, indent=2)

print(f"\nLabels saved to {LABELS_FILE}")
