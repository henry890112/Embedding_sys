import cv2
import threading
import time

recognizer = cv2.face.LBPHFaceRecognizer_create()
recognizer.read('face.yml')
cascade_path = "xml/haarcascade_frontalface_default.xml"
face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')

flag_lock = threading.Lock()
camera_flag = True  # True代表攝像頭1，False代表攝像頭2

def switch_camera_flag():
    global camera_flag
    while True:
        time.sleep(5)  # 每5秒切換一次攝像頭
        with flag_lock:
            camera_flag = not camera_flag

def start_camera(camera_index):
    cap = cv2.VideoCapture(camera_index)
    if not cap.isOpened():
        print(f"無法打開攝像頭 {camera_index}")
        return None
    return cap


def display_frames(camera_index):
    cap = start_camera(camera_index)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)


    while True:
        key = cv2.waitKey(5) & 0xFF

        # with flag_lock:
        #     current_camera_flag = camera_flag
        current_camera_flag = True

        if current_camera_flag and camera_index == 0:
            print("I am thread :",camera_index )
            ret, img = cap.read()
            if not ret:
                print("無法獲取畫面")
                break

            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            faces = face_cascade.detectMultiScale(gray, minNeighbors=10)

            name = {
                '1': 'Angus',
                '2': 'Henrry',
                '3': 'widden'
            }

            for (x, y, w, h) in faces:
                cv2.rectangle(img, (x, y), (x + w, y + h), (0, 255, 0), 2)
                idnum, confidence = recognizer.predict(gray[y:y + h, x:x + w])
                if confidence < 60:
                    text = name[str(idnum)]
                else:
                    text = '???'
                cv2.putText(img, text, (x, y - 5), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2, cv2.LINE_AA)

            cv2.imshow('camera', img)

            # cap.release()
            # cv2.destroyAllWindows()

        elif not current_camera_flag and camera_index == 2:
            print("I am thread :",camera_index )
            ret, img = cap.read()
            if not ret:
                print("無法獲取畫面")
                break

            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            faces = face_cascade.detectMultiScale(gray, minNeighbors=10)

            name = {
                '1': 'Angus',
                '2': 'Henrry',
                '3': 'widden'
            }

            for (x, y, w, h) in faces:
                cv2.rectangle(img, (x, y), (x + w, y + h), (0, 255, 0), 2)
                idnum, confidence = recognizer.predict(gray[y:y + h, x:x + w])
                if confidence < 60:
                    text = name[str(idnum)]
                else:
                    text = '???'
                cv2.putText(img, text, (x, y - 5), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2, cv2.LINE_AA)

            cv2.imshow('camera', img)

            cap.release()
            # cv2.destroyAllWindows()

        if key == ord('q'):
            cap.release()
            cv2.destroyAllWindows()
            break

def main():
    # switch_thread = threading.Thread(target=switch_camera_flag)
    display_thread1 = threading.Thread(target=display_frames, args=(0,))
    display_thread2 = threading.Thread(target=display_frames, args=(2,))

    # switch_thread.daemon = True
    display_thread1.daemon = True
    display_thread2.daemon = True

    # switch_thread.start()
    display_thread1.start()
    display_thread2.start()

    display_thread1.join()  # 等待開關攝像頭的線程結束，即程式結束

if __name__ == "__main__":
    main()
