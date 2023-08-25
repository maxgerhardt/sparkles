import math
import random
import csv
with open('distances.csv', "w", newline='') as file: 
    writer = csv.writer(file)
random.seed()

points = [[0, 0, 0]]
claps = []
def new_point():
    return [random.randint(0, 20), random.randint(0, 20), random.randint(0,20)]

for i in range(0,100):
    points.append(new_point())
    print(points[i])
for i in range(0,20):
    claps.append(new_point())


def calc_distances():
    distances = []
    for clap in claps:
        print("Clap at "+",".join(str(e)for e in clap))
        distance = []
        for point in points: 
            distance.append(round(math.dist(point, clap), 2))
        distances.append(distance)
    return distances

distances = calc_distances()
def csvwrite():
    with open('distances.csv', "w", newline='') as file: 
        writer = csv.writer(file)
        firstrow = ["Clap Number"]
        secondrow = ["Device Location"]
        for x in range(len(distances[0])):
            firstrow.append("D "+str(x))
            secondrow.append(str(points[x]))
        firstrow.append("Clap Location")
        writer.writerow(firstrow)
        writer.writerow(secondrow)
        counter = 0
        for distance in distances: 
            row = [counter]+distance+[str(claps[counter])]
            writer.writerow(row)
            counter+=1



        



