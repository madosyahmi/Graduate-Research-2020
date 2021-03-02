# -*- coding: utf-8 -*-
import serial
import traceback
import time
import numpy as np
import decode
import encode
import dbFunction

class UART:

    def __init__(self):

        # シリアル通信設定　
        # ボーレートは115200に設定
        # 1分間センサからの入力が来なければ終了
        try:
            self.uartport = serial.Serial(
                port="/dev/ttyTHS1",
                baudrate=115200,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=30,
            )
        except serial.SerialException:
            print(traceback.format_exc())

        # 受信バッファ、送信バッファクリア
        self.uartport.reset_input_buffer()
        self.uartport.reset_output_buffer()
        time.sleep(1)

    # データ送信関数
    def send_serial(self, cmd): 

        try:
             # 文字列をバイナリに変換して送信
            self.uartport.write((cmd).encode("ascii"))
        except serial.SerialException:
            print(traceback.format_exc())


    # データ受信関数
    def receive_serial(self):

        try:
            rcvdata = self.uartport.readline()
        except serial.SerialException:
            print(traceback.format_exc())
    
        # 受信したバイナリデータを文字列に変換　改行コードを削除
        return rcvdata.decode("ascii").rstrip()

    def end_serial(self):
        self.uartport.close()

def main():
    uart = UART()
    #最終待ち時間、endMin分信号なければ終了
    endMin = 30
    duringMin = 3
    sendNo = 240
    waitMin = 12*60
    firstSend = time.time()

    #3hunngoto,240(12h)kai,12h(720m)yasumi
    #3分毎、240(12h)回、12h夜間休み
    send_cmd = encode.make_senser_start_cmd(duringMin*60,sendNo,waitMin*60)
    uart.send_serial(send_cmd)
    duringSec = duringMin * 60
    while True:
        #屋外デバイスへの送信
        risk_color = dbFunction.get_last_enter_risk()
        if len(risk_color) == 1:
            send_cmd = encode.make_LED_cmd(risk_color[0])
            if send_cmd is not None:
                uart.send_serial(send_cmd)

        # データ受信 
        data = uart.receive_serial()

        #受信データがなかった場合
        if data == "":
            keika = time.time()-firstSend
            if (keika % duringSec >= (duringSec*2/3)) or ((keika % duringSec) >= (duringSec-60)):
                send_msg_flg = True
                FinishNo = int(keika / duringSec)+1 #切り上げ
            elif (keika % duringSec <= (duringSec/3)) or ((keika % duringSec) <= 65):
                send_msg_flg = True
                FinishNo = int(keika / duringSec) #切り捨て
            else:
                send_msg_flg = False
                FinishNo = int (keika/ duringSec) + 1

            if (FinishNo < sendNo) and send_msg_flg:
                send_cmd = encode.make_senser_start_cmd(duringSec,sendNo-FinishNo,waitMin*60)
                uart.send_serial(send_cmd)
            if(FinishNo >= sendNo):
                endMin -= 1
                if endMin <= 0:
                    uart.end_serial()
                    break

        #送信に対する返答でないことを確認
        elif not data.startswith(".."):
            try:
                data1 = decode.sencer_data(data)
                dbFunction.add_record(data1.dt,data1.from_serial_ID,data1.LQI,data1.temp,data1.humid,data1.pressure,data1.VCC,data1.CO2)
            except ValueError():
                pass

if __name__ == '__main__':
    main()