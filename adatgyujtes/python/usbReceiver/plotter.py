import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

fileCsv = "data(6).csv"

def readCSV(fileName):
    data = pd.read_csv(fileName, header=None)

    print(len(data))

    print(data[2:-1])

    return data[2:-1]

def plotDatas(data):

    D = data.to_numpy()

    D = D.astype(float)

    print(len(D))


    t = D[:, 0]
    ax = D[:, 1]
    ay = D[:, 2]
    az = D[:, 3]

    pictureName = fileCsv.strip(".csv") + "merged.png"

    saveXaccleData(t, ax, fileCsv)
    saveYaccleData(t, ay, fileCsv)
    saveZaccleData(t, az, fileCsv)

    plt.figure(figsize=(15, 9))

    plt.plot(t, ax)
    plt.plot(t, ay)
    plt.plot(t, az)

    plt.legend(["accelX", "accelY", "accleZ"])
    plt.ylabel("accel (m/s^2)")
    plt.xlabel("Time (ms)")

    #plt.xlim([500, 3000])

    plt.savefig(pictureName)
    plt.show()


def saveXaccleData(t, ax, fileName):

    name = fileName.strip(".csv") + "AccelX.png"
    print(fileName)

    fig, plt2 = plt.subplots(figsize=(15, 9))

    plt2.plot(t, ax)
    plt2.legend(["accelX"])
    plt2.set_ylabel("accel (m/s^2)")
    plt2.set_xlabel("Time (ms)")
    fig.savefig(name)

    fig.clear()

def saveYaccleData(t, ay, fileName):

    name = fileName.strip(".csv") + "AccelY.png"

    fig, plt2 = plt.subplots(figsize=(15, 9))

    plt2.plot(t, ay, 'orange')
    plt2.legend(["accelY"])
    plt2.set_ylabel("accel (m/s^2)")
    plt2.set_xlabel("Time (ms)")
    fig.savefig(name)

    fig.clear()

def saveZaccleData(t, az, fileName):

    name = fileName.strip(".csv") + "AccelZ.png"

    fig, plt2 = plt.subplots(figsize=(15, 9))

    plt2.plot(t, az, 'green')
    plt2.legend(["accelZ"])
    plt2.set_ylabel("accel (m/s^2)")
    plt2.set_xlabel("Time (ms)")
    fig.savefig(name)

    fig.clear()

if __name__ == "__main__":

    data = readCSV(fileCsv)
    #print(data)
    plotDatas(data)

