#import sys
import datetime
#引数にシリアルからの入力をいれてオブジェクト作成をするとオブジェクト内の各変数に値が格納される。
#対応したデータ以外を入れるとValueErrorを吐くことがあるためtry等を用いる。

class sencer_data:
    #オブジェクト変数一覧
    #dt
    #from_dev_ID,rom_serial_ID,LQI,data_len
    #data_name,nd,raw_temp,temp,raw_humid,humid,pressure,VCC,CO2

    def __init__(self,args):
        #引数にシリアルからの入力(UTF-8など文字列,改行なし)を渡す

        #取得した時の日時も記録しておく
        self.dt = datetime.datetime.now()
        
        args_array = list(args)
        #文字列の長さを軽く確認
        if len(args_array) < 50:
            #短すぎるとエラー
            raise ValueError("incorrect get msg.")
        #最初の三文字が正しいかだけ確認
        if args_array[0] != ":" or args_array[1] != "0" or args_array[2] != "F":
            #最初の三文字が正しくないときエラーを吐く
            raise ValueError("incorrect get msg.")

        #最初の:削除
        del args_array[0]
        nums_array = []
        for letter in args_array:
            nums_array.append(int(letter,16))
            

        #print("送信元論理デバイスID")
        self.from_dev_ID = 0
        for i in range(2):
            self.from_dev_ID *= 16
            self.from_dev_ID += nums_array.pop(0)


        #print("応答元シリアルID")
        self.from_serial_ID = 0
        for i in range(4*2):
            self.from_serial_ID *= 16
            self.from_serial_ID += nums_array.pop(0)

        #print("LQI")
        self.LQI = 0
        for i in range(1*2):
            self.LQI *= 16
            self.LQI += nums_array.pop(0)

        #print("データ長")
        self.data_len = 0
        for i in range(1*2):
            self.data_len *= 16
            self.data_len += nums_array.pop(0)

        #print("データ最初の固定文字(SBS1)")
        self.data_name = 0
        for i in range(4*2):
            self.data_name *= 16
            self.data_name += nums_array.pop(0)

        #print("温湿度(未使用、0)")
        self.nd = 0
        for i in range(4*2):
            self.nd *= 16
            self.nd += nums_array.pop(0)

        #print("温度(100倍)")
        self.raw_temp = 0
        for i in range(2*2):
            self.raw_temp *= 16
            self.raw_temp += nums_array.pop(0)
        self.temp = self.raw_temp /100.0

        #print("湿度(100倍)")
        self.raw_humid = 0
        for i in range(2*2):
            self.raw_humid *= 16
            self.raw_humid += nums_array.pop(0)
        self.humid = self.raw_humid / 100.0

        #print("気圧")
        self.pressure = 0
        for i in range(2*2):
            self.pressure *= 16
            self.pressure += nums_array.pop(0)

        #print("電源電圧")
        self.VCC = 0
        for i in range(2*2):
            self.VCC *= 16
            self.VCC += nums_array.pop(0)

        #print("CO2濃度")
        self.CO2 = 0
        for i in range(2*2):
            self.CO2 *= 16
            self.CO2 += nums_array.pop(0)

        #print("チェックサム")
        #self.check_sum = 0
        #for i in range(1*2):
        #    self.check_sum *= 16
        #    self.check_sum += nums_array.pop(0)

    def print_all(self):
        print("Get TimeDate:{0}".format(self.dt))
        print("送信元論理デバイスID：{0}".format(self.from_dev_ID))
        print("応答元シリアルID：{0}".format(self.from_serial_ID))
        print("LQI：{0}".format(self.LQI))
        print("データ長：{0}".format(self.data_len))
        print("データ最初の固定文字(SBS1)：{0}".format(self.data_name))
        print("温湿度(未使用、0)：{0}".format(self.nd))
        print("温度：{0}".format(self.temp))
        print("湿度：{0}".format(self.humid))
        print("気圧：{0}".format(self.pressure))
        print("電源電圧：{0}".format(self.VCC))
        print("CO2濃度：{0}".format(self.CO2))
        #print("チェックサム：{0}".format(self.check_sum))