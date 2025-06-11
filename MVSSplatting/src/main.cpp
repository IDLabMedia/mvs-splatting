#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <glm.hpp>
#include <map>
#include <unordered_map>
#include <chrono>
#include <vector>
#include <algorithm>
#include <gtx/string_cast.hpp>
#ifndef CXXOPTS_NO_EXCEPTIONS
#define CXXOPTS_NO_EXCEPTIONS
#endif
#include "cxxopts.hpp"

// From CMAKE preprocessor
std::string cmakelists_dir = CMAKELISTS_SOURCE_DIR;

#include "CameraParams.h"
#include "Shader.h"
#include "ShaderController.h"
#include "TexController.h"
#include "FramebufferController.h"
#include "Gui.h"
#include "KeyViewsCalculator.h"
#include "MultiViewStereo.h"
#include "SplatGenerator.h"

class Options {
public:
    std::string undistortedPath = "";
    bool eval = false;   // use test/train split
    bool headless = true;
    bool verbose = false;

public:

	Options() {}

	Options(int argc, char* argv[]) {

		cxxopts::Options options("A fast multi-view-stereo depth estimator and 3DGS splat initializer.");
		options.add_options()
			("h,help", "Print help")
            ("s,source", "Path (must end in /) to folder that contains (undistorted!) images/ and sparse/ ", cxxopts::value<std::string>())
            ("eval", "Use test/train split, so starting from 3rd image, ignore every 8th image)")
            ("gui", "Enable gui, otherwise runs headless")
            ("v,verbose", "Print helpful information")
			;
		
		cxxopts::ParseResult result = options.parse(argc, argv);
		

        if (result.count("source")) {
            undistortedPath = result["source"].as<std::string>();
        }
        if (result.count("eval")) {
            eval = true;
        }
        if (result.count("gui")) {
            headless = false;
        }
        if (result.count("verbose")) {
            verbose = true;
        }
        
        if (undistortedPath.size() < 1 || undistortedPath.compare(undistortedPath.size() - 1, 1, "/") != 0) {
            printf("Error: source path should end in / \n");
            std::cout << options.help() << std::endl;
            exit(0);
        }

        if (verbose) {
            printf("Source path: %s\n", undistortedPath.c_str());
            printf("Eval       : %s\n", eval ? "true" : "false");
            printf("Gui        : %s\n", headless ? "false" : "true");
            printf("Verbose    : %s\n", verbose ? "true" : "false");
        }
	}
};

int main(int argc, char** argv) {

    // parse command line args
    Options options = Options(argc, argv);
    
    // define the paths to the dataset
    std::string imagesPath = options.undistortedPath + "images/";
    std::string sparse0Path = options.undistortedPath + "sparse/0/";
    
    // read in the COLMAP camera parameters
    Intrinsics intrinsics(sparse0Path + "cameras.bin");
    Extrinsics extrinsics(sparse0Path + "images.bin", options.eval);
    SfmPoints sfmPoints(sparse0Path + "points3D.bin");
    if (!intrinsics.Init()) return -1;
    if (!extrinsics.Init()) return -1;
    if (!sfmPoints.Init(extrinsics)) return -1;

    // Init GLFW and glad
    // let the gui already determine the window size
    Gui gui(intrinsics, extrinsics, MultiViewStereo::nrLayers);
    gui.InitWindow(options.headless);
    
    // choose the key views for which to estimate the depth map, which in turn are used to create gaussian splats
    KeyViewsCalculator keyViewsCalculator(intrinsics, extrinsics, options.verbose);
    keyViewsCalculator.EstimateOverlapBetweenCameras();
    if (!keyViewsCalculator.CalculateKeyCameras()) return -1; 
    keyViewsCalculator.CalculateMvsNeighbors();
    keyViewsCalculator.Cleanup();

    // Setup some OpenGL helpers
    ShaderController& shaders = ShaderController::getInstance();
    TexController& textures = TexController::getInstance();
    FrameBufferController& framebuffers = FrameBufferController::getInstance();
    if (!shaders.Init(intrinsics, gui.width_g, gui.height_g, gui.scale_g)) return false;
    if (!textures.Init(intrinsics, extrinsics, keyViewsCalculator.keyCameras, keyViewsCalculator.mvsNeighbors, imagesPath, keyViewsCalculator.nrMvsNeighbors, MultiViewStereo::nrLayers)) return false;
    if (!framebuffers.Init(sfmPoints, intrinsics.width, intrinsics.height, keyViewsCalculator.keyCameras.size())) return false;

    // MVS
    MultiViewStereo mvs(intrinsics, extrinsics, keyViewsCalculator.mvsNeighbors, keyViewsCalculator.keyCameras, sfmPoints.depthRanges);
    mvs.CalculateRoughDepth();

    // Mask off bad depth map pixels
    SplatGenerator splatGenerator(intrinsics, extrinsics, keyViewsCalculator.keyCameras);
    splatGenerator.MaskAwayUnnecessaryPixels();
    splatGenerator.WriteToFile(sparse0Path + "points3D_mvs.bin", framebuffers.tfSubdivisions, sfmPoints);

    // visualize depth maps etc.
    if (!options.headless) {
        gui.Run(keyViewsCalculator.keyCameras);
    }

    textures.Cleanup();
    framebuffers.Cleanup();

    return 0;
}