import cv2

# 打開webcam
cap = cv2.VideoCapture(2)  # 通常0代表預設的webcam，如果有多個webcam，可以嘗試1、2等等 2(黑白), 4(彩色), 6(內建鏡頭)可用

if not cap.isOpened():
    print("無法打開webcam")
    exit()

while True:
    # 讀取一幀視訊
    ret, frame = cap.read()
    # 變成320*240
    frame = cv2.resize(frame, (320, 240))

    if not ret:
        print("無法讀取視訊流")
        break

    # 顯示視訊幀
    cv2.imshow('Webcam', frame)

    # 存取一張320*240的圖片
    # cv2.imwrite('webcam.jpg', frame)

    # 按下'q'鍵退出迴圈
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# 釋放資源
cap.release()
cv2.destroyAllWindows()
