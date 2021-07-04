import numpy as np
import matplotlib.pyplot as plt



def main():
    data = np.genfromtxt("relative_error_substrate.txt").transpose()

    fig, vax = plt.subplots(2,3)

    for l in range(2):
        for k in range(3):
            vax[l, k].hist(data[2*k + l], density=True, bins='sturges', color = 'C0' if (l == 0) else 'C3', edgecolor='black', label='$\\sigma$ = %.2f' % np.std(data[2*k+l]))
            
            vax[l, k].set_axisbelow(True)
            vax[l, k].grid(linestyle=':')
            vax[l, k].legend()

    vax[0,0].set_title("Particle 1")
    vax[0,1].set_title("Particle 2")
    vax[0,2].set_title("Substrate")

    vax[0,0].set_ylabel("Relative error :: $D_\\alpha$")
    vax[1,0].set_ylabel("Relative error :: $\\alpha$")

    plt.show()



if __name__ == "__main__":
    main()