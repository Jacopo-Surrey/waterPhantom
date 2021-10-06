import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

fileName = "output_nt_2_deposited_energy.csv"

data = pd.read_csv(fileName, names=["E_kev", "l_um", "Z", "d_mm", "ID"], comment="#")

sumE = data.groupby(['d_mm'])["E_kev"].sum()

E = sumE.to_numpy()

plt.plot(E)

plt.show()
