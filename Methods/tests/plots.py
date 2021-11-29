import numpy as np
import matplotlib.pyplot as plt
from numpy.lib.function_base import corrcoef

def loadImages():
	# Importing images and converting to RGB output
	img0 = np.genfromtxt("imageOriginal.txt")
	img1 = np.genfromtxt("imageRotated.txt")
	fixed = np.genfromtxt("imageCorrected.txt")
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

	return before, after

def main():
	before, after = loadImages()	


	posOrig = np.genfromtxt("positionOriginal.txt").transpose()
	posRot = np.genfromtxt("positionRotated.txt").transpose()
	posCor = np.genfromtxt("positionCorrected.txt").transpose()
	posDist = np.genfromtxt("distanceDistributions.txt").transpose()

	fig, ax = plt.subplots(1,3)

	ax[0].set_title("Before")
	ax[0].imshow(before, origin='lower')
	ax[0].plot(posOrig[0], posOrig[1], linestyle='', marker='.', label='Original')
	ax[0].plot(posRot[0], posRot[1], linestyle='', marker='.', label='Rotated')
	
	ax[1].set_title("After")
	ax[1].imshow(after, origin='lower')
	ax[1].plot(posOrig[0], posOrig[1], linestyle='', marker='.', label='Original')
	ax[1].plot(posCor[0], posCor[1], linestyle='', marker='.', label='Corrected')

	ax[2].set_title("Distance between original and corrected")
	ax[2].hist(posDist[0], density=True, label='Corrected')
	ax[2].hist(posDist[1], density=True, label='Original')
	ax[2].set_xlabel("Pixels")


	for v in ax:
		v.legend()
	
	plt.show()




if __name__ == "__main__":
	main()