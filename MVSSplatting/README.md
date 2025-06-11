# MVS-Splatting MVS implementation

This directory contains the C++ source code for our paper, and consists of 5 parts:

* `KeyViewsCalculator.h`: A way to choose the key cameras, and neighbors per key camera.

* `MultiViewStereo.h`: A Multi-View-Stereo (MVS) depth estimator, which uses plane sweeps to generate a depth map per key camera, as well as a certainty map.

* `SplatGenerator.h`: A consistency checker, which generates the final mask per key camera. At this point, we have a point cloud.

* `SplatGenerator.h`: A way of converting the point cloud to 3D Gaussian splats, and writing these to a file: `sparse/0/points3D_mvs.bin`.

* `Gui.h`: There also is an optional GUI to visualize the point cloud. Use `--gui` to enable it.

### Build & dependencies

Clone the repository, and run CMake to compile and build the project.

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE Release
make
```

Uses **OpenGL 4.0** (for transform feedback).

### Running

Usage (although it is recommended  to at least once run with `--gui` enabled):

```bash
MVSSplatting.exe -s <path/to/dataset/>   # path should end in '/'
```

Optional command line args:

```
-h       print help
--eval   use test/train split
--gui    open with GUI, otherwise runs headless
-v       verbose
```

Example usage:

```bash
cd <location containing executable>
MVSSplatting.exe -s "../example/orchids/" --gui
```

To replicate the results of the paper, add `--eval` as command line argument.
