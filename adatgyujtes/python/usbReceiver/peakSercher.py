import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

csvFile = "data(17).csv"

SAMPLE_NUMBER = 100

def readCSV(fileName):
    data = pd.read_csv(fileName, header=None)

    #print(len(data))

    #print(data[2:-1])

    return data[2:-1]

def peaks(data):
    D = data.to_numpy()

    D = D.astype(float)

    t = D[:, 0]
    ax = D[:, 1]
    ay = D[:, 2]
    az = D[:, 3]

    magnitude = calculateMagnitude(ax, ay, az)

    pozTreshold = 20
    negTreshold = -10

    Pozcounter = 0
    negCounter = 0
    prevTime = 0


    print(len(ax))

    for i in range(len(magnitude)):
        if magnitude[i] > pozTreshold:
            if Pozcounter == 0:
                print("Pozitív Treshold növekvő irányban átlépve Ax value: ", magnitude[i] , "Idő: ", t[i], "Frekvencia: ", 1/((t[i] - prevTime) / 1000), " Hz")
                Pozcounter += 1
                prevTime = t[i]

        if magnitude[i] < pozTreshold:
            if Pozcounter != 0:
                print("Pozitív Treshold csökkenő irányban átlépve Ax value: ", magnitude[i], "Idő: ", t[i])
                Pozcounter = 0

        if magnitude[i] < negTreshold:
            if negCounter == 0:
                print("Negatív Treshold csökkenő irányban átlépve Ax value: ", ax[i], "Idő: ", t[i])
                negCounter += 1

        if magnitude[i] > negTreshold:
            if negCounter != 0:
                print("Negatív Treshold növekvő irányban átlépve Ax value: ", magnitude[i], "Idő: ", t[i])
                negCounter = 0

    plt.figure(figsize=(15, 9))

    plt.plot(t, magnitude, "o")
    plt.legend(["magnitude"])
    plt.ylabel(" Acceleration (g)")
    plt.xlabel("Time (ms)")

    plt.show()

def calculateMagnitude(ax, ay, az):

    axSquare = ax ** 2
    aySquare = ay ** 2
    azSauqre = az ** 2

    sum = axSquare + aySquare + azSauqre

    #print(sum)

    magnitude = np.sqrt(sum)

    #print(magnitude)

    return magnitude

if __name__ == "__main__":

    data = readCSV(csvFile)
    peaks(data)

    #print(data)

