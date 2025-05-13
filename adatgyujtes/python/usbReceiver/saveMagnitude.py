import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import math

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

    magnitude = calculateMagnitude(ax, ay, az) #gyorsulas nagysaga

    pictureName = fileCsv.strip(".csv") + "magnitude.png"

    plt.ylabel("accel (m/s^2)")
    plt.xlabel("Time (ms)")

    #plt.xlim([1900, 2400])

    #plt.show()

    plt.plot(t, magnitude, "red")
    plt.legend(["magnitude"])

    plt.savefig(pictureName)

    plt.show()

def calculateMagnitude(ax, ay, az):

    axSquare = ax ** 2
    aySquare = ay ** 2
    azSauqre = az ** 2

    sum = axSquare + aySquare + azSauqre

    print(sum)

    magnitude = np.sqrt(sum)

    print(magnitude)

    return magnitude

if __name__ == "__main__":

    data = readCSV(fileCsv)
    #print(data)
    plotDatas(data)