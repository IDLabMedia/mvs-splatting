## Dowload link

Easily download the datasets from [NextCloud](https://cloud.ilabt.imec.be/index.php/s/is2SjDgZBsdYAed). Read the rest of this readme for more information.

## Sources

----------------------

Scroll down to see a visual overview of all datasets.

Our dataset is called **Ghent29** and consists of 29 scenes. If you use our datasets in your work, please [cite us](https://idlabmedia.github.io/mvs-splatting/).

Existing datasets that were used in the paper:

* [DTU](https://roboimagedata.compute.dtu.dk/?page_id=36)
* [Local light field fusion (llff)](https://bmild.github.io/llff/)  [(download link)](https://github.com/Fyusion/LLFF/issues/45)
* [MipNerf360](https://jonbarron.info/mipnerf360/)
* [Ommo](https://github.com/luchongshan/OMMO?tab=readme-ov-file)
* [Shiny](https://nex-mpi.github.io/)
* [Tanks & Temples (tnt)](https://www.tanksandtemples.org/download/)
* [UCL (University College London)](http://visual.cs.ucl.ac.uk/pubs/deepblending/datasets.html)

Disclaimer: I do not own or claim any rights to the linked datasets. All rights remain with their respective creators or copyright holders. This repository is provided for reference and convenience only.

## Additional information

----------------------

**Resoluton of images**

Of datasets with a lot of images, the images' resolution have been downscaled to reduce 3DGS training times and VRAM usage.
This also speeds up COLMAP. Note that 3DGS downscales images with dimensions > 1600 pixels anyway.

**Colmap's PINHOLE camera model**

The structure-from-motion step of COLMAP has already been ran on every dataset, including an **undistortion** step.
So cameras.bin / cameras.txt will look like this:
`1 PINHOLE width height focal_x focal_y principal_point_x principal_point_y`

where focal_x == focal_y and (principal_point_x, principal_point_y) == 0.5 * (width, height)

This means that the directory structure of each dataset looks like this:

```bash
dataset/
├── images/                # undistorted
└── sparse/                # PINHOLE
    └── 0/
        ├── cameras.bin
        ├── images.bin
        └── points3D.bin
```

**Dataset specific changes**

* For the **Shiny** benchmark, the original datasets contains many more images. We only use every third image, since this research is focused more on sparsely sampled datasets.
* Idem **Ommo** benchmark.

## Screenshots of all datasets

------------

![dataset_overview.svg](dataset_overview.svg)
