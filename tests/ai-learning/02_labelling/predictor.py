import joblib

# Load model and vectorizer
model = joblib.load("sentiment_model.joblib")
vectorizer = joblib.load("vectorizer.joblib")

while True:
    word = input("Enter a word (or 'q' to quit): ").strip().lower()
    if word == 'q':
        break
    X = vectorizer.transform([word])
    proba = model.predict_proba(X)[0]  # Returns [P(-1), P(0), P(1)]
    print(f"Negative: {proba[0]:.2f}, Neutral: {proba[1]:.2f}, Positive: {proba[2]:.2f}")
