from deepface import DeepFace
import cv2
import os

# === STEP 1: Build face database (folder with known faces) ===
db_path = "faces"

if not os.path.exists(db_path):
    os.makedirs(db_path)
    print("Created 'faces/' folder. Add some face images there first.")
    exit()

print("Loading known faces from:", db_path)

# === STEP 2: Start webcam and match live faces ===
cap = cv2.VideoCapture(0)
print("Press 'q' to quit")

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Save current frame temporarily
    cv2.imwrite("live.jpg", frame)

    try:
        results = DeepFace.find(img_path="live.jpg", db_path=db_path, enforce_detection=True)

        if len(results) > 0 and not results[0].empty:
            best_match = results[0].iloc[0]
            name_with_ext = os.path.basename(best_match["identity"])
            name = os.path.splitext(name_with_ext)[0]
            text = f"Hello, {name} 😊"
            color = (0, 255, 0)
        else:
            text = "Unknown face ❌"
            color = (0, 0, 255)

    except Exception as e:
        text = "Face not detected ❗"
        color = (0, 0, 255)

    cv2.putText(frame, text, (30, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, color, 2)
    cv2.imshow("Face Recognition", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()

# Delete the temporary image
if os.path.exists("live.jpg"):
    os.remove("live.jpg")
