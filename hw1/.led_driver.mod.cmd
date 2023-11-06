cmd_/home/henry/Embedding_sys/hw1/led_driver.mod := printf '%s\n'   led_driver.o | awk '!x[$$0]++ { print("/home/henry/Embedding_sys/hw1/"$$0) }' > /home/henry/Embedding_sys/hw1/led_driver.mod
