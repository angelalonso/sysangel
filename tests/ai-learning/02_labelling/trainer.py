import json
import numpy as np
from sklearn.linear_model import SGDClassifier
from sklearn.feature_extraction.text import TfidfVectorizer

# Load labeled data
with open("word_sentiments.json", 'r') as f:
    word_db = json.load(f)

# Prepare data
words, scores = [], []
for word, data in word_db.items():
    if data["count"] >= 1:  # Only use words with at least 1 label
        words.append(word)
        scores.append(1 if data["score"] > 0.3 else 0 if data["score"] > -0.3 else -1)

# Convert words to numerical features
vectorizer = TfidfVectorizer(analyzer='char', ngram_range=(2, 3))  # Captures subword patterns
X = vectorizer.fit_transform(words)

# Train incremental model
model = SGDClassifier(loss='log_loss')  # Supports partial_fit()
model.partial_fit(X, scores, classes=[-1, 0, 1])

# Save model and vectorizer
import joblib
joblib.dump(model, "sentiment_model.joblib")
joblib.dump(vectorizer, "vectorizer.joblib")
