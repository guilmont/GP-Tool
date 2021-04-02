
<!-- PROJECT LOGO -->
<h1 align="center">GP-Tool</h1>
<img src="./screenshots/screenshot.png" alt="gptool_demo" width="90%" style="margin-left: 5%;">
  <p align="center">
    GP-Tool: A user-friendly graphical interface to apply GP-FBM
</p>



<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#about-the-project">About The Project</a></li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#license">License</a></li>
    </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project
This software accompanies the following paper:

<h3> Precise measurements of chromatin diffusion dynamics by modeling using Gaussian processes</h3> 

GM Oliveira <i>et al</i>

bioRxiv: https://doi.org/10.1101/2021.03.16.435699 


<br/>

"GP-Tool contains 4 plugins: movie, alignment, trajectories and g-process. The movie plugin allows the user to open TIFF files, display basic ImageJ and OME metadata, define color maps for each channel and manually correct for contrast. The alignment plugin runs the algorithm described in Methods to digitally correct chromatin aberration and possible camera calibration issues. Alternatively, the user can manually modify each of the parameters. The program can perform the analysis of several cells in the same movie. The trajectories plugin provides the ROI utility which allows the user to select spots of interest and extract trajectories.  Finally,the g-process plugin allows to infer the diffusion and confinement parameters correcting for substrate movement if two or more particles are tracked. It is also possible to use a MCMC sampler to obtain the posterior probability distribution associated with each of these parameters. Once the analysis is complete, the tool provides the possibility to save the results into two file formats: JSON and HDF5."

NOTE: This is a beta version, quickly put together for journal submission. Currently, code is under refactoring and optimizations.

<br/>

<!-- GETTING STARTED -->
## Getting Started

GP-Tool uses a melange of open-source libraries at its core. Some of them are automatically included as vendor libraries, whilst others need to be preinstalled.

### Prerequisites

These libraries need to be preinstalled:
* GLM: <a href="http://glm.g-truc.net">http://glm.g-truc.net</a>
* GLFW 3.0+: <a href="https://www.glfw.org/">https://www.glfw.org/</a>
* HDF5: <a href="https://www.hdfgroup.org/hdf5">https://www.hdfgroup.org/hdf5</a>
* JsonCpp: <a href="https://github.com/open-source-parsers/jsoncpp">https://github.com/open-source-parsers/jsoncpp</a>

Under Linux, these packages can easily be installed via command line:
* Ubuntu
```
apt install libglm-dev libglfw3-dev libhdf5-dev libjsoncpp-dev
```

* Manjaro
```
pamac install glm glfw-x11 hdf5 jsoncpp
```


### Installation

1. Clone the repo. As I include several sub-modules as vendors, recursive clone is needed.
   ```
   git clone --recurse-submodules -j5 git@github.com:guilmont/GP-Tool.git
   ```

2. For the more experienced, the software can be built using cmake. Otherwise, the facility script will compile and install GP-Tool to desktop. 
   ```
   bash configure.sh
   ```
<br/>

<!-- LICENSE -->
## License

Distributed under the Apache2 license. See `LICENSE` for more information.


<br/>


---

Copyright 2020-2021 Guilherme MONTEIRO OLIVEIRA