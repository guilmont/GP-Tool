import numpy as np
import matplotlib.pyplot as plt



def main():
    data = np.genfromtxt("relative_error.txt").transpose()

    fig, vax = plt.subplots(1,2)

    x = np.linspace(-1, 1, 25)

    for l in range(2):
        vax[l].hist(data[l], density=True, bins=x , color = 'C0' if (l == 0) else 'C3', edgecolor='black', label='$\\sigma$ = %.2f' % np.std(data[l]))
            
        vax[l].set_axisbelow(True)
        vax[l].grid(linestyle=':')
        vax[l].legend()


    vax[0].set_xlabel("Relative error :: $D_\\alpha$")
    vax[1].set_xlabel("Relative error :: $\\alpha$")

    plt.show()



if __name__ == "__main__":
    main()