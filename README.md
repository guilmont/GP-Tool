
<!-- PROJECT LOGO -->
<h1 align="center">GP-Tool</h1>
<img src="./screenshots/screenshot.png" alt="gptool_demo" width="98%"">
  <p align="center">
    GP-Tool: A user-friendly graphical interface to apply GP-FBM
</p>



<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#about">About</a></li>
    <li><a href="#bioxrv">BioRxv</a></li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#windows-binaries">Windows binaries</a></li>
        <li><a href="#source-code">Source code</a></li>
      </ul>
    </li>
    <li><a href="#license">License</a></li>
    </ol>
</details>

## About
GP-Tool is a set of methods I developed for the analysis of dynamic particles in the living cells. It comprehends of 4 major libraries:

- Movie: Loads tif files with ImageJ or OME metadata. It allows for lzw compressing;
- Trajectory: Import track produced by Icy Software in the XML format. Differently, CSV files with columns "ParticleID, Frame, Position_X, Position_Y" are also accepted. 
- Alignment: Corrects for camera mis-alignment for microscopy setups with more than on camera. Corrects for chromatic aberration
- GP-FBM: Uses Gaussian processes and Fractional Brownian motion to measure effective diffusion coefficient, anomalous coefficients and more. It implicitly corrects for background/substrate movement for sets with 2 or more particles. 

For more information, consider the following journal publication and/or "gp_documentation.pdf".

## BioRxv

<h3> Precise measurements of chromatin diffusion dynamics by modeling using Gaussian processes</h3> 

GM Oliveira <i>et al</i>

bioRxiv: https://doi.org/10.1101/2021.03.16.435699 


<b>Abstract:</b> <i>The spatiotemporal organization of chromatin influences many nuclear processes: from chromo-some segregation to transcriptional regulation. To get a deeper understanding of these processes it is essential to go beyond static viewpoints of chromosome structures, and to accurately characterize chromatin mobility and its diffusion properties. Here, we present GP-FBM: a new computational framework based on Gaussian processes and fractional Brownian motion to analyze and extract diffusion properties from stochastic trajectories of labeled chromatin loci. GP-FBM is able to optimally use the higher-order correlations present in the data and therefore outperforms existing methods. Furthermore, GP-FBM is able to extrapolate trajectories from missing data and account for substrate movement automatically. Using our method we show that diffusive chromatin diffusion properties are surprisingly similar in interphase and mitosis in mouse embryonic stem cells. Moreover, we observe surprising heterogeneity in local chromatin dynamics, which correlates with transcriptional activity. We also present GP-Tool, a user-friendly graphical interface to facilitate the use of GP-FBM by the research community for future studies of nuclear dynamics.</i>


<!-- GETTING STARTED -->
## Getting Started

### Windows binaries

GP-Tool (GUI + libraries) is pre-compiled for Windows 10 (x64). Just download the binaries zip-file, extract it and you are good to go.


### Source code 

1. Clone the repo. As I include several sub-modules as vendors, recursive clone is needed.
   ```
   git clone --recurse-submodules -j4 https://github.com/guilmont/GP-Tool.git
   ```

2. Building from source code allows these following options:
  - HIDPI: Scales GUI by a factor of 2, allows for appropriate sizing in hidpi screens (default OFF);
  - COMPILE_GUI: Compiles graphical user interface (default ON);
  - COMPILE_LIBRARIES: Compiles and install dynamic libraries for batching with C++ (default ON);

3. Build it with CMake setting CMAKE_BUILD_TYPE (usually Release) and CMAKE_INSTALL_PREFIX. Compile and install.


<!-- LICENSE -->
## License

Distributed under the Apache2 license. See `LICENSE` for more information.


<br/>


---

Copyright 2020-2021 Guilherme MONTEIRO OLIVEIRA