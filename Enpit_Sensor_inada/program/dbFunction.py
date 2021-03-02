# -*- coding: utf-8 -*-
import mysql.connector as mydb
import datetime
import random #テスト用、あとで消去
def add_record(dt,from_serial_ID,LQI,temp,humid,pressure,VCC,CO2):
    conn = mydb.connect(
        user="EnviMonitor",
        password="EnviMonitor",
        host="localhost",
        database="SensorDB"
    )
    cur = conn.cursor(buffered = True)

    #devID特定
    sql = 'SELECT devID FROM device WHERE serialNo = '+ str(from_serial_ID)
    cur.execute(sql)
    if cur.rowcount != 1:
        cur.fetchall()
        sql = 'INSERT INTO device (serialNo) VALUES ('+str(from_serial_ID)+')'
        cur.execute(sql)
        conn.commit()
        sql = 'SELECT devID FROM device WHERE serialNo = '+ str(from_serial_ID)
        cur.execute(sql)
    dev = cur.fetchone()
    dev_ID = dev[0]

    sql = 'INSERT INTO records (dt,devID,LQI,temp,humid,pressure,VCC,CO2) VALUES ("'+ \
        dt.strftime("%Y-%m-%d %H:%M:%S") +'",'+str(dev_ID)+','+str(LQI)+','+ \
            str(temp)+','+str(humid)+','+str(pressure)+','+str(VCC)+','+str(CO2)+')'
    cur.execute(sql)
    conn.commit()
    cur.close()
    conn.close()


def get_all_record(srcDate):
    conn = mydb.connect(
        user="EnviMonitor",
        password="EnviMonitor",
        host="localhost",
        database="SensorDB"
    )
    cur = conn.cursor(buffered = True,dictionary= True)

    #sql = 'SELECT dt,device.serialNo,LQI,temp,humid,pressure,VCC,CO2 FROM records JOIN device USING (devID)'
    sql = 'SELECT dt,devID,LQI,temp,humid,pressure,VCC,CO2 FROM records '+'WHERE DATE_FORMAT(dt, "%Y-%m-%d") = "'+ \
        srcDate.strftime("%Y-%m-%d")+'"  ORDER BY dt DESC'
    cur.execute(sql)
    ans = cur.fetchall()

    #キーの名前をわかりやすく変換
    #for record in ans:
    #    record['from_serial_ID'] = record['serialNo']
    #    del record['serialNo']

    cur.close()
    conn.close()
    return ans

def get_all_devID():
    conn = mydb.connect(
        user="EnviMonitor",
        password="EnviMonitor",
        host="localhost",
        database="SensorDB"
    )
    cur = conn.cursor(buffered = True)

    sql = 'SELECT devID FROM device'
    cur.execute(sql)
    ans = cur.fetchall()

    IDarray = []
    for rec in ans:
        IDarray.append(rec[0])

    cur.close()
    conn.close()
    return IDarray

#devIDの直近num個のレコードを取得
def get_records(devID,num,srcDate):

    conn = mydb.connect(
        user="EnviMonitor",
        password="EnviMonitor",
        host="localhost",
        database="SensorDB"
    )
    cur = conn.cursor(buffered = True)

    sql = 'SELECT dt,devID,LQI,temp,humid,pressure,VCC,CO2 FROM records WHERE devID = '+ str(devID) +' AND DATE_FORMAT(dt, "%Y-%m-%d") = "'+ \
        srcDate.strftime("%Y-%m-%d")+'"  ORDER BY dt DESC LIMIT '+str(num)
    cur.execute(sql)
    ans = cur.fetchall()
    
    cur.close()
    conn.close()
    return ans

def regi_enter_risk(enterLisk):
    conn = mydb.connect(
        user="EnviMonitor",
        password="EnviMonitor",
        host="localhost",
        database="RiskDB"
    )
    cur = conn.cursor(buffered = True)

    #色表記は大文字に
    EnterLisk = enterLisk.upper()

    dt = datetime.datetime.now()

    sql = 'INSERT INTO enterLisk (dt,risk) VALUES ("'+ \
        dt.strftime("%Y-%m-%d %H:%M:%S") +'","'+EnterLisk+'")'
    cur.execute(sql)
    conn.commit()
    cur.close()
    conn.close()

def get_last_enter_risk():
    conn = mydb.connect(
        user="EnviMonitor",
        password="EnviMonitor",
        host="localhost",
        database="RiskDB"
    )
    cur = conn.cursor(buffered = True)

    sql = 'SELECT risk FROM enterLisk ORDER BY dt DESC LIMIT 1'
    cur.execute(sql)
    ans = cur.fetchall()

    IDarray = []
    for rec in ans:
        IDarray.append(rec[0])

    cur.close()
    conn.close()
    return IDarray

def del_all_records():
    conn = mydb.connect(
        user="EnviMonitor",
        password="EnviMonitor",
        host="localhost",
        database="SensorDB"
    )
    cur = conn.cursor(buffered = True)

    sql = 'DELETE FROM device'
    cur.execute(sql)
    sql = 'DELETE FROM records'
    cur.execute(sql)
    conn.commit()

    #オートインクリメント初期化
    sql = 'alter table device auto_increment = 1'
    cur.execute(sql)
    sql = 'alter table records auto_increment = 1'
    cur.execute(sql)
    conn.commit()

    cur.close()
    conn.close()

def del_all_risks():
    conn = mydb.connect(
        user="EnviMonitor",
        password="EnviMonitor",
        host="localhost",
        database="RiskDB"
    )
    cur = conn.cursor(buffered = True)

    sql = 'DELETE FROM enterLisk'
    cur.execute(sql)
    conn.commit()

    #オートインクリメント初期化
    sql = 'alter table enterLisk auto_increment = 1'
    cur.execute(sql)
    conn.commit()

    cur.close()
    conn.close()

def make_test_data():
    '''
    add_record(datetime.datetime.now() - datetime.timedelta(minutes= 16),12345,100,20.5,55.6,1000,3100,400)
    add_record(datetime.datetime.now() - datetime.timedelta(minutes= 15),67890,90,20.8,55.6,1000,3100,400)
    add_record(datetime.datetime.now() - datetime.timedelta(minutes= 12),12345,100,20.5,55.6,1000,3100,410)
    add_record(datetime.datetime.now() - datetime.timedelta(minutes= 12),67890,105,20.8,30.0,1000,3100,320)
    add_record(datetime.datetime.now() - datetime.timedelta(minutes= 9),12345,100,20.5,55.6,1000,3000,450)
    add_record(datetime.datetime.now() - datetime.timedelta(minutes= 10),67890,106,21.8,30.0,1000,3000,1000)
    add_record(datetime.datetime.now() - datetime.timedelta(minutes= 6),12345,88,20.5,55.6,1000,3000,480)
    add_record(datetime.datetime.now() - datetime.timedelta(minutes= 3),67890,108,22.7,30.0,1000,3000,400)
    add_record(datetime.datetime.now(),12345,100,20.5,55.6,1000,3000,500)
    add_record(datetime.datetime.now(),67890,120,23.6,30.0,1000,2900,450)

add_record(datetime.datetime.now()- datetime.timedelta(minutes= 3),12345,100,20.5,55.6,1000,3100,random.randint(500,1200))
add_record(datetime.datetime.now()- datetime.timedelta(minutes= 3),67890,100,20.5,55.6,1000,3100,random.randint(500,1200))
add_record(datetime.datetime.now()- datetime.timedelta(minutes= 6),12345,100,20.5,55.6,1000,3100,random.randint(500,1200))
add_record(datetime.datetime.now()- datetime.timedelta(minutes= 6),67890,100,20.5,55.6,1000,3100,random.randint(500,1200))
add_record(datetime.datetime.now()- datetime.timedelta(minutes= 9),12345,100,20.5,55.6,1000,3100,random.randint(500,1200))
add_record(datetime.datetime.now()- datetime.timedelta(minutes= 9),67890,100,20.5,55.6,1000,3100,random.randint(500,1200))
add_record(datetime.datetime.now()- datetime.timedelta(minutes= 12),12345,100,20.5,55.6,1000,3100,random.randint(500,1200))
add_record(datetime.datetime.now()- datetime.timedelta(minutes= 12),67890,100,20.5,55.6,1000,3100,random.randint(500,1200))
add_record(datetime.datetime.now()- datetime.timedelta(minutes= 15),12345,100,20.5,55.6,1000,3100,random.randint(500,1200))
add_record(datetime.datetime.now()- datetime.timedelta(minutes= 15),67890,100,20.5,55.6,1000,3100,random.randint(500,1200))

    regi_enter_risk("Yellow")
    '''
#del_all_records()
#random.seed()
#add_record(datetime.datetime.now(),12345,100,20.5,55.6,1000,3100,random.randint(500,1200))
#add_record(datetime.datetime.now(),67890,100,20.5,55.6,1000,3100,random.randint(500,1200))
