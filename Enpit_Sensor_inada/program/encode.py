# -*- coding: utf-8 -*-
#センサデバイスに送信するためのコマンドを生成し、文字列として返す。
#文字列の終わりには改行も含まれているため、そのままwriteで使える
#during_sec:何秒ごとにデータを送るか。(最低でも60以上の数字を入れること)
#send_no:何回送った後に夜間休暇に入るか(例:10分おきで10時間(600秒)連続稼働の場合は 600/10 = 60と入力)
#wait_sec:夜間休暇の秒数
#なお、夜間休暇明けには再度スタートコマンドを送る必要があります
def make_senser_start_cmd(during_sec,send_no,wait_sec):
    DEV_ID = 0X0F
    CMD = 0X00

    send_cmd = ":" + format(DEV_ID,"02X") + format(CMD,"02X") 
    send_cmd = send_cmd + format(during_sec,"04X") + format(send_no,"04X") + format(wait_sec,"08X")
    send_cmd = send_cmd + "X" +"\r\n"

    return send_cmd

#室外デバイスに送信するためのコマンドを生成し、文字列として返す。
#なお、色の名前についてはデータベースからの応答の通り全部大文字であること
def make_LED_cmd(color):
    DEV_ID = 0X10

    if color == "RED" :
        CMD = 0X01
    elif color == "YELLOW":
        CMD = 0X02
    elif color == "GREEN":
        CMD = 0X03
    elif color == "":
        CMD = 0X00
    else:
        return

    send_cmd = ":" + format(DEV_ID,"02X") + format(CMD,"02X") + "X" +"\r\n"

    return send_cmd
    