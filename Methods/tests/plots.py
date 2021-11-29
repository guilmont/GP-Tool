import json
import numpy as np
import matplotlib.pyplot as plt
from numpy.lib.function_base import corrcoef

from scipy.stats import ttest_ind

def checkAlignment():
	with open("testAlignment.json", "r") as f:
		data = json.load(f)

	# Importing images and converting to RGB output
	img0 = np.asarray(data["Original"]["Image"])
	img1 = np.asarray(data["Rotated"]["Image"])
	fixed = np.asarray(data["Corrected"]["Image"])
	zero = np.zeros(shape=img0.shape)

	# Scaling values between 0 and 1
	top, bot = np.max(img0), np.min(img0)
	img0 = (img0 - bot) / (top-bot)
	
	top, bot = np.max(img1), np.min(img1)
	img1 = (img1 - bot) / (top-bot)

	top, bot = np.max(fixed), np.min(fixed)
	fixed = (fixed - bot) / (top-bot)

	# Composing final image
	before = np.dstack((img0, img1, zero))
	after = np.dstack((img0, fixed, zero))


	######################################
	# Loading positional data

	posOrig = np.asarray(data["Original"]["Positions"])
	posRot = np.asarray(data["Rotated"]["Positions"])
	posCor = np.asarray(data["Corrected"]["Positions"])
	
	dRot, dCor = posRot - posOrig, posCor - posOrig

	rot = np.sqrt(dRot[0]**2 + dRot[1]**2)
	cor = np.sqrt(dCor[0]**2 + dCor[1]**2)

	print(ttest_ind(rot, cor))

	fig, ax = plt.subplots(1,3)

	ax[0].set_title("Before")
	ax[0].imshow(before, origin='lower')
	ax[0].plot(posOrig[1], posOrig[0], linestyle='', marker='.', label='Original')
	ax[0].plot(posRot[1], posRot[0], linestyle='', marker='.', label='Rotated')
	
	ax[1].set_title("After")
	ax[1].imshow(after, origin='lower')
	ax[1].plot(posOrig[1], posOrig[0], linestyle='', marker='.', label='Original')
	ax[1].plot(posCor[1], posCor[0], linestyle='', marker='.', label='Corrected')

	ax[2].set_title("Distance between original and corrected")
	ax[2].hist(cor, density=True, bins='sturges', label='Corrected')
	ax[2].hist(rot, density=True, bins='sturges', label='Original')
	ax[2].set_xlabel("Pixels")
	
	for v in ax:
		v.legend()


def checkEnhancement():
	with open("enhancement.json", 'r') as f:
		data = json.load(f)

	distX = np.asarray(data["errorX"])
	distY = np.asarray(data["errorY"])

	fig, ax = plt.subplots(1,1)
	ax.hist(distX, density=True, edgecolor='black', bins='sturges', label='X-axis')
	ax.hist(distY, density=True, edgecolor='black', bins='sturges', label='Y-axis')
	ax.set_title("Positional error")
	ax.set_xlabel("Pixels")
	
	ax.set_axisbelow(True)
	ax.grid(linestyle=':')
	ax.legend()

def checkGPFBM():
	single = np.genfromtxt("singleGP.txt").transpose()
	coupled = np.genfromtxt("coupledGP.txt").transpose()

	fig, ax = plt.subplots(1,1)
	ax.hist(single[0], density=True, bins='sturges', edgecolor='black', label='D')
	ax.hist(single[1], density=True, bins='sturges', edgecolor='black', label='A', alpha=0.6)

	ax.legend()
	ax.set_axisbelow(True)
	ax.grid(linestyle=':')


	fig, ax = plt.subplots(1,3)
	ax[0].hist(coupled[0], density=True, bins='sturges', edgecolor='black', label='D')
	ax[0].hist(coupled[1], density=True, bins='sturges', edgecolor='black', label='A', alpha=0.6)
	ax[0].set_title("Particle 1")

	ax[1].hist(coupled[2], density=True, bins='sturges', edgecolor='black', label='D')
	ax[1].hist(coupled[3], density=True, bins='sturges', edgecolor='black', label='A', alpha=0.6)
	ax[1].set_title("Particle 2")

	ax[2].hist(coupled[4], density=True, bins='sturges', edgecolor='black', label='D')
	ax[2].hist(coupled[5], density=True, bins='sturges', edgecolor='black', label='A', alpha=0.6)
	ax[2].set_title("Background")

	for v in ax:
		v.legend()
		v.set_axisbelow(True)
		v.grid(linestyle=':')

def main():
	# checkAlignment()
	# checkEnhancement()
	checkGPFBM()

	
	plt.show()




if __name__ == "__main__":
	main()