import cv2
import os
import signal
import time
import sys
import subprocess




def handle_signal(signum, frame):
    print("收到信號，開始人臉識別。")
    global detecting
    detecting = True

def face_detection(gray, img):
    global bad_list
    faces = face_cascade.detectMultiScale(gray, minNeighbors=10)
    for (x, y, w, h) in faces:
        cv2.rectangle(img, (x, y), (x + w, y + h), (0, 255, 0), 2)
        idnum, confidence = recognizer.predict(gray[y:y + h, x:x + w])
        if confidence < 60:
            text = name[str(idnum)]
            bad_list.append(0)
        else:
            text = '???'
            bad_list.append(1)
        cv2.putText(img, text, (x, y - 5), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2, cv2.LINE_AA)



# 設定信號處理函數
signal.signal(signal.SIGUSR1, handle_signal)

# 初始化人臉識別
recognizer = cv2.face.LBPHFaceRecognizer_create()
recognizer.read('face.yml')
face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')


# 建立姓名和 id 的對照表
name = {
    '1': 'Angus',
    '2': 'Henrry',
    '3': 'Widden'
}

if len(sys.argv) != 2:
    print("Usage: python trig_face.py <tcp_server_pid>")
    sys.exit(1)

tcp_server1_pid = int(sys.argv[1])

print("正在執行 face_detection.py, 前門PID:", os.getpid())

detecting = False
camera = True
bad_guy_flag = False
bad_list = []
first_in_flag = True

# # 初始化攝影機
# cap = cv2.VideoCapture(0)
# cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
# cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)

while (1):
    
    if(camera == True):
        
        while not detecting:
            time.sleep(1)
   
        if(first_in_flag):
            # 初始化攝影機
            cap = cv2.VideoCapture(0)
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)
            first_in_flag = False
        if not cap.isOpened():
            print("Cannot open camera")
            exit()

        window_name = 'Back Door'
        cv2.namedWindow(window_name)
        cv2.moveWindow(window_name, 500, 100)  # 視窗的 x, y 位置
        count = 0
        start_time = time.perf_counter()
        # 主迴圈
        while True:
            ret, img = cap.read()
            count = count +1
            if not ret:
                print("Cannot receive frame")
                break
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

            face_detection(gray, img)
            cv2.imshow('Back Door', img)
            key = cv2.waitKey(5) & 0xFF
            if key == ord('q'):
                break
            elapsed_time = time.perf_counter() - start_time
            if elapsed_time > 10:
                break
        
        # bad_guy_ratio = sum(bad_list) / len(bad_list) if len(bad_list) > 0 else 0
        bad_guy_ratio = sum(bad_list) / count if count > 0 else 0
        if(bad_guy_ratio > 0.6):
            bad_guy_flag = True
            file_name = "./deliver_photo/back_camera.jpg"
            cv2.imwrite(file_name, img)

        print("ratio = {}".format(bad_guy_ratio))
        print("len = {}".format(count))

        if(bad_guy_flag):
            os.kill(tcp_server1_pid, signal.SIGUSR1)
            cap.release()
            cv2.destroyAllWindows()
            camera = False
            detecting = False  # 重置 detect 狀態
            
            # 開啟UDP server
            subprocess.Popen(["./deliver_realtime/back_realtime_server_udp"])
            print("!!!!!!!!!!")
            bad_guy_flag = False
        else:
            # cap.release()
            cv2.destroyAllWindows()
            detecting = False




    
    
