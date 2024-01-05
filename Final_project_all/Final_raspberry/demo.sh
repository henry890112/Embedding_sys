#!/bin/bash

SESSION="final"

# 检查并结束已存在的 tmux session
tmux has-session -t $SESSION 2>/dev/null
if [[ $? -eq 0 ]]
then
    tmux kill-session -t $SESSION
fi

# 创建新的 tmux session
tmux new-session -d -s $SESSION -n window

# 创建第一行的三个窗格
tmux split-window -h -t $SESSION:window.0
tmux split-window -h -t $SESSION:window.0

# 创建第二行的三个窗格
tmux select-layout -t $SESSION even-horizontal
tmux split-window -v -t $SESSION:window.0
tmux split-window -v -t $SESSION:window.1
tmux split-window -v -t $SESSION:window.2

# 启动前两个命令并获取它们的 PID
tmux send-keys -t $SESSION:window.0 "deliver_photo/front_server_tcp" C-m
tmux send-keys -t $SESSION:window.1 "deliver_photo/back_server_tcp" C-m

# 启动剩余命令
# 注意：需要手动输入前两个命令的 PID
tmux send-keys -t $SESSION:window.2 "python trig_face.py" # 等待输入
tmux send-keys -t $SESSION:window.3 "python trig_face2.py" # 等待输入
tmux send-keys -t $SESSION:window.4 "./pir_sensor" # 等待输入
tmux send-keys -t $SESSION:window.5 "sudo ./led_writer" C-m

# 連接到 session
tmux -2 attach-session -t $SESSION



