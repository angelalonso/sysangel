import pandas as pd
import numpy as np
from tensorflow.keras import layers, models
import os
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"  # Disable GPU

## Dataset with clear patterns (20 words)
#words = ["happy", "hate", "joy", "sad", "love", "anger", "peace", "war", 
#         "kind", "cruel", "smile", "fear", "hope", "pain", "fun", "hurt",
#         "good", "evil", "nice", "mean"]
#labels = [1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0]

def load_data(filepath):
    df = pd.read_csv(filepath)  # Loads 100k+ rows in milliseconds
    words = df['word'].values
    labels = df['label'].values
    return words, labels


# Manual feature engineering: 
# 1. Ratio of vowels to consonants (positive words often have more vowels)
# 2. Contains letter 'e' (common in negative words like "hate", "anger")
def extract_features(word):
    vowels = sum(1 for c in word if c in 'aeiou')
    consonants = len(word) - vowels
    vowel_ratio = vowels / len(word) if len(word) > 0 else 0
    has_e = 1 if 'e' in word else 0
    return [vowel_ratio, has_e]

words, labels = load_data('sentiment_data.csv')
X = np.array([extract_features(word) for word in words])
y = np.array(labels)


# SIMPLEST POSSIBLE MODEL (logistic regression)
model = models.Sequential([
    layers.Dense(1, input_shape=(2,), activation='sigmoid')  # Just 1 neuron!
])
model.compile(optimizer='adam', loss='binary_crossentropy')

# Train
model.fit(X, y, epochs=50, verbose=0)

# Test
test_words = ["love", "hate", "joy", "unknown"]
for word in test_words:
    test_input = np.array([extract_features(word)])
    prediction = model.predict(test_input, verbose=0)[0][0]
    print(f"'{word}' → {prediction:.2f}")
