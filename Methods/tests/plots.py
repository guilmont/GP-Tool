import json
import numpy as np
import matplotlib.pyplot as plt
from numpy.lib.function_base import corrcoef

from scipy.stats import ttest_ind

def checkAlignment():
	with open("output/testAlignment.json", "r") as f:
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
	with open("output/enhancement.json", 'r') as f:
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
	single = np.genfromtxt("output/singleGP.txt").transpose()
	coupled = np.genfromtxt("output/coupledGP.txt").transpose()

	fig, ax = plt.subplots(1,1)
	ax.hist(single[0], density=True, bins='sturges', edgecolor='black', label='D')
	ax.hist(single[1], density=True, bins='sturges', edgecolor='black', label='A', alpha=0.6)
	ax.set_title("Relative error distribution :: Single model")

	ax.legend()
	ax.set_axisbelow(True)
	ax.grid(linestyle=':')


	fig, ax = plt.subplots(1,3)
	ax[0].hist(coupled[0], density=True, bins='sturges', edgecolor='black', label='D')
	ax[0].hist(coupled[1], density=True, bins='sturges', edgecolor='black', label='A', alpha=0.6)
	ax[0].set_title("Relative error distribution :: Particle 1")

	ax[1].hist(coupled[2], density=True, bins='sturges', edgecolor='black', label='D')
	ax[1].hist(coupled[3], density=True, bins='sturges', edgecolor='black', label='A', alpha=0.6)
	ax[1].set_title("Relative error distribution :: Particle 2")

	ax[2].hist(coupled[4], density=True, bins='sturges', edgecolor='black', label='D')
	ax[2].hist(coupled[5], density=True, bins='sturges', edgecolor='black', label='A', alpha=0.6)
	ax[2].set_title("Relative error distribution :: Background")

	for v in ax:
		v.legend()
		v.set_axisbelow(True)
		v.grid(linestyle=':')


def checkBatchingSingle():
	# Set values for simulations
	_D, _A= 0.1, 0.5

	with open("output/batchResults_Single.json", 'r') as f:
		data = json.load(f)

	vDA = list()
	for mov in data["Movies"]:
		try:
			DA = data["Movies"][mov]["GProcess"]["Single"]["dynamics"][0]
			vDA.append([(DA[2] / 0.5**DA[3] - _D)/_D, (DA[3] - _A)/_A])
		
		except Exception as ex:
			print(ex, mov)


	vDA = np.asarray(vDA).transpose()

	fig, ax = plt.subplots(1,2)
	ax[0].hist(vDA[0], density=True, bins='sturges', edgecolor='black')
	ax[0].set_title("Apparent Diffusion :: avg = %.3f" % vDA[0].mean())

	ax[1].hist(vDA[1], density=True, bins='sturges', edgecolor='black')
	ax[1].set_title("Anomalous Coefficient :: avg = %.3f" % vDA[1].mean())

	fig.suptitle("Batching test :: Single")

	for v in ax:
		v.set_axisbelow(True)
		v.grid(linestyle=':')
		v.set_xlabel("Relative error")

def checkBatchingCoupled():
	# Set values for simulations
	_D1, _D2, _DR = 0.1, 0.08, 0.5
	_A1, _A2, _AR = 0.45, 0.5, 1.0

	with open("output/batchResults_Coupled.json", 'r') as f:
		data = json.load(f)

	vD, vA = list(), list()

	for mov in data["Movies"]:
		DA1 = data["Movies"][mov]["GProcess"]["Corrected"]["dynamics"][0]
		DA2 = data["Movies"][mov]["GProcess"]["Corrected"]["dynamics"][1]

		DR = data["Movies"][mov]["GProcess"]["Corrected"]["Substrate"]["DR"]
		AR = data["Movies"][mov]["GProcess"]["Corrected"]["Substrate"]["AR"]

		vD.append([(DA1[2] / 0.5**DA1[3]  - _D1)/_D1, (DA2[2] / 0.5**DA2[3] - _D2)/_D2, (DR / 0.5**AR -_DR)/_DR])
		vA.append([(DA1[3] - _A1)/_A1, (DA2[3] - _A2)/_A2, (AR-_AR)/_AR])


	vD = np.asarray(vD).transpose()
	vA = np.asarray(vA).transpose()

	fig, ax = plt.subplots(1,3)
	ax[0].hist(vD[0], density=True, bins='sturges', edgecolor='black', label="%.3f" % vD[0].mean())
	ax[0].hist(vD[1], density=True, bins='sturges', edgecolor='black', alpha=0.8, label="%.3f" % vD[1].mean())
	ax[0].set_title("Apparent Diffusion")

	ax[1].hist(vA[0], density=True, bins='sturges', edgecolor='black', label="%.3f" % vA[0].mean())
	ax[1].hist(vA[1], density=True, bins='sturges', edgecolor='black', alpha=0.8, label="%.3f" % vA[1].mean())
	ax[1].set_title("Anomalous Coefficient")


	ax[2].hist(vD[2], density=True, bins='sturges', edgecolor='black', label='DR = %.3f' % vD[2].mean())
	ax[2].hist(vA[2], density=True, bins='sturges', edgecolor='black', label='AR = %.3f' % vA[2].mean(), alpha=0.8)
	ax[2].set_title("Substrate")

	fig.suptitle("Batching test")

	for v in ax:
		v.set_axisbelow(True)
		v.grid(linestyle=':')
		v.set_xlabel("Relative error")
		v.legend(title="Avg")

if __name__ == "__main__":
	checkAlignment()
	checkEnhancement()
	checkGPFBM()
	checkBatchingSingle()
	checkBatchingCoupled()

	
	plt.show()
