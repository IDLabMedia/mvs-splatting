## How to replicate our results using MVSGaussian

Use the same conda/pip environment already created for the gaussian-splatting repository

```bash
conda activate gaussian-splatting
# I had to downgrade numpy for imgaug
pip install numpy==1.26.4
# install additional dependencies
pip install imageio scikit-image pyyaml termcolor kornia imgaug lpips
```

To generate `poses_bounds.npy`, run

```bash
python lib/colmap/imgs2poses.py -s <path to dataset>
```

To generate the point cloud that serves as an initialization for 3DGS, run

```bash
python run.py --type evaluate --cfg_file configs/mvsgs/benchmark_eval.yaml test_dataset.data_root <parent folder dataset> test_dataset.scene <dataset> save_ply True dir_ply <path to save ply>
```

### Source

This repository was adapted from the original [MVSGaussian](https://github.com/TQTQliu/MVSGaussian) paper.

Disclaimer: I do not own or claim any rights to the code in this repository. All rights remain with their respective creators or copyright holders. This repository is provided for reference and convenience only.

### Changes made to the original repository

* Added `configs/mvsgs/benchmark_eval.yaml`, which is based on `colmap_eval.yaml` but with `volume_planes: [64, 8]`.

* `lib/datasets/colmap/mvsgs.py`: made the train/test split consistent with the rest of the paper, so:
  
  ```python
    train_ids  = [i for i in range(img_len) if i % 8 != 2]
  ```
  
  Additionally, since the `scale_factor` plays [an important role](https://github.com/TQTQliu/MVSGaussian/issues/83) in the quality of the reconstruction, we do our best to set the scale to a good value, depending on the dataset:
  
  ```python
  near_far = [max(1.0,depth_ranges[:, 0].min().item()), depth_ranges[:, 1].max().item()]
  dtu_near_far = [425.0, 905.0]
  scale_factor_near = dtu_near_far[0] / near_far[0]
  scale_factor_far = dtu_near_far[1] / near_far[1]
  self.scale_factor =(scale_factor_near + scale_factor_far) / 2
  cfg.mvsgs.scale_factor = self.scale_factor
  ```

## Why do the MVSGaussian results look so bad

Check out the github issues [here](https://github.com/TQTQliu/MVSGaussian/issues/83) and [here](https://github.com/TQTQliu/MVSGaussian/issues/84).
MVSGaussian seems to evaluate their code as follows. They use a train/test split. They have a test camera A, for which they choose 2 or 3 neighbors, e.g. B and C. They estimate the depth maps of B and C, turn those into point clouds, and render the point clouds from the perspective of A. They can now calculate the PSNR, SSIM, etc. for test camera A. Only using 2 to 3 neighbors is not enough though, and sometimes, large areas of the depth map are guessed rather than correctly estimated.



They repeat this process for all test cameras, so if there are 10 test cameras, they will have generated 10 different scene reconstructions. Generating 1 scene reconstruction (so for 1 test camera) takes a couple of seconds. So in our paper, we limited the number of scene reconstructions to 4 (as this was already taking between 10~20 seconds). This means that the scene is only reconstructed from 4 viewpoints. For scenes with a large number of cameras, for example the Tanks & Temples scenes `truck` and `train`, this is not enough.
