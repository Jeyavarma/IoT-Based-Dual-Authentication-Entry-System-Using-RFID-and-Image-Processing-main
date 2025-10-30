from deepface import DeepFace
import cv2
import os

# === SETUP ===
db_path = "faces"
if not os.path.exists(db_path):
    os.makedirs(db_path)

# === START CAMERA ===
cap = cv2.VideoCapture(0)
print("[INFO] Press 'c' to capture face")
print("[INFO] Press 'q' to quit")

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Display instructions
    cv2.putText(frame, "Press 'c' to capture, 'q' to quit", (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    # Save temporary frame to use with DeepFace
    cv2.imwrite("live.jpg", frame)

    # === FACE RECOGNITION ===
    try:
        results = DeepFace.find(img_path="live.jpg", db_path=db_path, enforce_detection=True)

        if len(results) > 0 and not results[0].empty:
            best_match = results[0].iloc[0]
            name_with_ext = os.path.basename(best_match["identity"])
            name = os.path.splitext(name_with_ext)[0]
            text = f"{name}"
            color = (0, 255, 0)
        else:
            text = "Unknown"
            color = (0, 0, 255)

    except:
        text = "No face"
        color = (0, 0, 255)

    # Display result
    cv2.putText(frame, text, (30, 60), cv2.FONT_HERSHEY_SIMPLEX, 1, color, 2)
    cv2.imshow("Face Recognition", frame)

    key = cv2.waitKey(1) & 0xFF

    # === CAPTURE NEW FACE ===
    if key == ord('c'):
        name = input("Enter name: ").strip()
        if name:
            filename = os.path.join(db_path, f"{name}.jpg")
            cv2.imwrite(filename, frame)
            print(f"[INFO] Face saved as {filename}")
        else:
            print("[WARN] Name cannot be empty!")

    elif key == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()

# Clean up
if os.path.exists("live.jpg"):
    os.remove("live.jpg")
