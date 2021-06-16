import json
import tifffile
import numpy as np
from numpy.random import uniform, poisson


def main():
    # Basic info
    NUM_POINTS = 1000
    NX, NY = 50, 50
    L, I0, B = 1.0, 100.0, 100.0

    # Generating images
    x = np.arange(0.5, NX, 1.0, dtype="float").reshape(1, -1)
    y = np.arange(0.5, NY, 1.0, dtype="float").reshape(-1, 1)
    vecImages = np.zeros(shape=(NUM_POINTS, 1, 2, NY, NX), dtype="float")
    vecPos = np.zeros(shape=(2, NUM_POINTS, 2), dtype="float")

    for ch in [0, 1]:
        rx, ry = 0.5 * NX, 0.5 * NY
        for num in range(NUM_POINTS):
            vecPos[ch, num] = [rx, ry]

            dx, dy = (x - rx) / L, (y - ry) / L
            img = I0 * np.exp(-0.5 * (dx * dx + dy * dy)) + B

            if ch == 1:
                img *= 0.75

            vecImages[num, 0, ch] = poisson(img)

            nx, ny = rx + uniform(-0.5, 0.5), ry + uniform(-0.5, 0.5)
            rx = nx if (nx < NX - 3 or nx > 3) else rx
            ry = ny if (ny < NY - 3 or ny > 3) else ry

    # Let's save ttrajectories for verification later
    traj = {
        "channel0": vecPos[0].transpose().tolist(),
        "channel1": vecPos[1].transpose().tolist(),
    }

    with open("../original_positions.json", "w") as arq:
        json.dump(traj, arq)

    # Save movie
    name = "../test_localization.tif"
    tifffile.imsave(name, vecImages.astype("uint16"))


if __name__ == "__main__":
    main()
