from math import *

samplingFrequency = 4000
samples = 128
freqMedia = [62.5, 250, 1000]
decay = [10, 10, 10]
minval = 0.001

def frecuencia(i):
	return ((i * 1.0 * samplingFrequency) / samples)

for color in range(3):
	print(color)
	for ifreq in range(samples // 2):
		freq = frecuencia(ifreq)
		if(freq != 0):
			deltaFreq = (log(freq) - log(freqMedia[color]))/(log(9)-log(8))
			val = exp(-pow(deltaFreq, 2) / decay[color])
			if(val >= minval):	
				print(str(freq) + "Hz:" + str(ifreq) + ":" + str(exp(-pow(deltaFreq, 2) / decay[color])))