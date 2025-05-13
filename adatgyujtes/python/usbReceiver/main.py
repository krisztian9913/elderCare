from serial.tools import list_ports
import serial
import pandas
import time
import csv
import os

def listPorts():

    ports = list_ports.comports()

    for port in ports:
        print(port)

def connectSerial():
    try:
        serialCom = serial.Serial("COM11", 115200)
    except Exception as e:
        print("Hiba csatlakozáskor: ", e)

        return

    return serialCom

def readSerial(serialCom):

    fileName = "data"
    fileNameTmp = fileName + ".csv"

    if os.path.isfile(fileNameTmp):
        i = 0
        while os.path.isfile(fileNameTmp):
            i += 1
            fileNameTmp = fileName + "(" + str(i) + ").csv"

    f = open(fileNameTmp, "w", newline="")
    f.truncate()

    #print("File opened")

    k = 0
    dataCounter = 0

    while True:
        try:
            s_bytes = serialCom.readline()

            decoded_bytes = s_bytes.decode("utf-8").strip("\r\n")

            #print(decoded_bytes)

            if k < 2:
                values = decoded_bytes.split(",")
            else:
                values = [float(x) for x in decoded_bytes.split(",")]

            print(type(values[1]))

            writer = csv.writer(f, delimiter=",")
            writer.writerow(values)

            dataCounter += 1
            k += 1

            if values[1] == -100.0:
                print("File close!", "Wrote data: ", dataCounter)
                f.close()
                return

        except Exception as e:
            print(e)


if __name__ == '__main__':
    listPorts()

    serialCom = connectSerial()

    if serialCom == None:
        exit(-1)

    readSerial(serialCom)
    exit(0)

