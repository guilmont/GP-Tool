import json
import numpy as np
import xml.etree.ElementTree as ET
from PIL import Image


from math import sqrt


from matplotlib import use
import matplotlib.pyplot as plt

use("Qt5Agg")
plt.rcParams["axes.labelsize"] = "x-large"
plt.rcParams["axes.titlesize"] = "x-large"
plt.rcParams["legend.fontsize"] = "large"

cor = "#e74c3c"
cor2 = "#229954"


def loadXML(path):
    tree = ET.parse(path).getroot()

    mat = list()
    for detection in tree.findall("./trackgroup/track/detection"):
        t = float(detection.attrib["t"])
        x = float(detection.attrib["x"])
        y = float(detection.attrib["y"])

        mat.append([t, 0, x, y])

    return np.asarray(mat, dtype=np.float32)


def calcDistance(mat, orig):

    distrib = list()
    for vec in mat:
        t, x, y = int(vec[0]), vec[2], vec[3]
        ox, oy = orig[t, 0], orig[t, 1]
        dx, dy = x - ox, y - oy

        distrib.append(sqrt(dx * dx + dy * dy))

    return distrib


def calcSize(mat):
    return [sqrt(vec[6] * vec[6] + vec[7] * vec[7]) for vec in mat]


def calcError(mat):
    return [sqrt(vec[4] * vec[4] + vec[5] * vec[5]) for vec in mat]


def main():
    # Loading trajectories
    vTraj1 = np.genfromtxt("../traj_green.txt", dtype=np.float32)
    vTraj2 = np.genfromtxt("../traj_red.txt", dtype=np.float32)

    with open("../test_data/original_positions.json", "r") as f:
        data = json.load(f)

    vOrig1 = np.asarray([data["channel0"][0], data["channel0"][1]], dtype=np.float32).transpose()
    vOrig2 = np.asarray([data["channel1"][0], data["channel1"][1]], dtype=np.float32).transpose()

    vIcy1 = loadXML("../test_data/test_localization_green.xml")
    vIcy2 = loadXML("../test_data/test_localization_red.xml")


    ############################################################################

    # Calculating error distribution
    dEnhanced = np.asarray(calcDistance(vTraj1, vOrig1) + calcDistance(vTraj2, vOrig2), dtype=np.float32)

    dIcy = np.asarray(calcDistance(vIcy1, vOrig1) + calcDistance(vIcy2, vOrig2), dtype=np.float32)

    # Aranging size distribution
    dSize = np.asarray(calcSize(vTraj1) + calcSize(vTraj2), dtype=np.float32)

    # Aranging error distribution
    dError = np.asarray(calcError(vTraj1) + calcError(vTraj2), dtype=np.float32)

    # Loading original trajectories
    img = Image.open("../test_data/test_localization.tif")

    # PLOTS #########################

    ax = plt.subplot(141)
    ax.set_title('a', fontweight='bold')
    ax.imshow(img, cmap="gray", aspect='auto')
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_ylabel("Synthetic frame")
    

    ###################################

    ax = plt.subplot(142)
    ax.set_title('b', fontweight='bold')

    ax.hist(dEnhanced, density=True, bins='sturges', alpha=0.8, edgecolor='C0', label="Enhanced")
    ax.hist(dIcy, density=True, bins='sturges', alpha=0.8, edgecolor='C1', label="ICY")

    ax.set_ylim(bottom=0.0)

    ax.set_axisbelow(True)
    ax.grid(linestyle=":")
    ax.set_xlabel("Localization error")
    ax.set_ylabel("Distribution")
    ax.legend()

    ###################################

    ax = plt.subplot(143)
    ax.set_title('c', fontweight='bold')

    ax.hist(dSize, density=True, alpha=0.8, edgecolor=cor2, color=cor2)

    #ax.set_xlim(x[0], x[-1])
    ax.set_ylim(bottom=0)

    ax.set_axisbelow(True)
    ax.grid(linestyle=":")
    ax.set_xlabel("Spot size")
    ax.set_ylabel("Distribution")

    ###################################

    ax = plt.subplot(144)
    ax.set_title('d', fontweight='bold')

    ax.hist(dError, density=True, alpha=0.8,edgecolor=cor, color=cor)

    ax.set_ylim(bottom=0.0)
    ax.set_axisbelow(True)
    ax.grid(linestyle=":")
    ax.set_xlabel("Estimated error")
    ax.set_ylabel("Distribution")

    plt.show()


if __name__ == "__main__":
    main()
