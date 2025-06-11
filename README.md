# [MVS-Splatting: Fast Multi-View Stereo Depth Fusion for 3D Gaussian Splatting Initialization](https://idlabmedia.github.io/mvs-splatting/)

This is the code repository for the paper with the same name. We also publish a dataset called `Ghent29`.

## Repository structure

This repository contains 4 sub-repositories, each with their own README.md.

1. `datasets`: the instructions to download the datasets used in the paper.

2. `gaussian-splatting`:  fork of [3DGS](https://github.com/graphdeco-inria/gaussian-splatting) adapted for the paper.

3. `MVSGaussian`: fork of [MVSGaussian (ECCV'24)](https://github.com/TQTQliu/MVSGaussian) adapter for the paper.

4. `MVSSplatting`: our code (depth estimation etc.)

## How to use

Assuming you have a dataset with the SfM (and **undistortion**) step of COLMAP already applied:

```bash
dataset/
├── images/                # undistorted
└── sparse/                # PINHOLE
    └── 0/
        ├── cameras.bin
        ├── images.bin
        └── points3D.bin
```

Use `MVSSplatting` to generate  `sparse/0/points3D_mvs.bin`. This contains the 3D Gaussian Splats to initialize the training process (see `gaussian-splatting`) with, instead of the COLMAP SfM point cloud. During/after training, you can use 3DGS' realtime viewers to visualize the splats.

`MVSSplatting` also has an optional GUI.

## BibTex

```
Our paper is currently under review. Once published, the citation will become available here.
```

Paper and dataset by [IDLab Media](https://media.idlab.ugent.be/)


