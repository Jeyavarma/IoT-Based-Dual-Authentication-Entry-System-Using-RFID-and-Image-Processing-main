from flask import Flask, request
import os
from datetime import datetime
from deepface import DeepFace

app = Flask(__name__)

# === Setup ===
db_path = "faces"
if not os.path.exists(db_path):
    os.makedirs(db_path)
    print("Created 'faces/' folder. Add authorized faces inside.")

print("[INFO] Loading known faces from:", db_path)

UPLOAD_FOLDER = 'uploads'
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

@app.route('/upload', methods=['POST'])
def upload_image():
    if not request.data:
        return "false"

    # Save the received image
    now = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"capture_{now}.jpg"
    filepath = os.path.join(UPLOAD_FOLDER, filename)

    with open(filepath, 'wb') as f:
        f.write(request.data)

    print(f"✅ Image saved: {filepath}")

    try:
        # Use DeepFace to search face
        results = DeepFace.find(img_path=filepath, db_path=db_path, enforce_detection=True)

        if len(results) > 0 and not results[0].empty:
            # Face recognized
            best_match = results[0].iloc[0]
            matched_identity = os.path.basename(best_match["identity"])
            matched_name = os.path.splitext(matched_identity)[0]
            print(f"✅ Face recognized: {matched_name}")
            return "true"
        else:
            print("❌ No matching face found.")
            return "false"

    except Exception as e:
        print(f"Error during face recognition: {e}")
        return "false"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
