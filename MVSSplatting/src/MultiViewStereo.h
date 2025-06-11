#ifndef MULTIVIEWSTEREO_H
#define MULTIVIEWSTEREO_H


class MultiViewStereo {
private:
    ShaderController& shaders;
    FrameBufferController& framebuffers;
    TexController& textures;
    Intrinsics intrinsics;
    Extrinsics extrinsics;
    std::vector<int> keyCamIds;
    std::map<int, std::vector<int>> neighborIds;
	std::map<int, std::vector<int>> mvsNeighbors;
	std::unordered_map<int, glm::vec2> depthRanges;

public:

	static const int nrLayers = 50;


    MultiViewStereo(Intrinsics intrinsics, Extrinsics extrinsics, std::map<int, std::vector<int>> mvsNeighbors, std::vector<int> keyCamIds, std::unordered_map<int, glm::vec2> depthRanges) :
        shaders(ShaderController::getInstance()), 
        framebuffers(FrameBufferController::getInstance()), 
        textures(TexController::getInstance()), 
        extrinsics(extrinsics),
        intrinsics(intrinsics),
		mvsNeighbors(mvsNeighbors),
		keyCamIds(keyCamIds),
		depthRanges(depthRanges) { }

	void CalculateRoughDepth() {

		std::vector<float> depthPerLayer;
		glm::vec2 layer_to_depth;
		textures.CreateTmpVec3(static_cast<int>(std::ceil(nrLayers / 32.0f)));
		glViewport(0, 0, intrinsics.width, intrinsics.height);

		for (int& mainId : keyCamIds) {
			ChooseDepthPerLayer(mainId, /*out*/depthPerLayer, layer_to_depth);

			shaders.quadDepthErrorShader.use();
			shaders.quadDepthErrorShader.setMat4("model", extrinsics.poses[mainId].model);

			shaders.sumRadiusShader.use();
			shaders.sumRadiusShader.setInt("radius", 12);
			shaders.sumRadiusShader.setInt("step", 3);
			shaders.sumRadiusShader.setInt("nrTextures", mvsNeighbors[mainId].size());

			// for each neighbor, and for each layer, calculate the error map
			for (int layer = 0; layer < nrLayers; layer++) {
				float depth = depthPerLayer[layer];
				int n = 0;
				for (int& neighborId : mvsNeighbors[mainId]) {
					// calculate the error for a certain depth
					shaders.quadDepthErrorShader.use();
					shaders.quadDepthErrorShader.setFloat("depth", depth);
					shaders.quadDepthErrorShader.setMat4("view", extrinsics.poses[neighborId].view);
					framebuffers.RenderQuadWithOneDepth(textures.images[mainId], textures.images[neighborId], /*out*/ textures.tmpFloat_neighbors[n]);
					n++;
				}

				// smooth and sum the error of each neighbor
				shaders.sumRadiusShader.use();
				framebuffers.SumNeighborTextures(textures.images[mainId], textures.tmpFloat_neighbors, &shaders.sumRadiusShader, /*out*/textures.tmpFloat_layers[layer]);
			}

			// For each pixel, find the depth that gives the lowest error.
			// Can have up to 32 textures binded as input textures.
			// Outputs a vec3, which is actually:
			//  - float (actually int) : the best layer
			//	- float                : the lowest error
			//  - float (actually int) : nr layers close to lowest error
			shaders.errorToDepthShader0.use();
			int i = 0;
			for (int offset = 0; offset < nrLayers; offset += 32) {
				int nrTexturesToProcess = std::min(32, nrLayers - offset);
				shaders.errorToDepthShader0.use();
				framebuffers.FindLowestErrorDepth0(textures.tmpFloat_layers, nrTexturesToProcess, offset, &shaders.errorToDepthShader0, textures.tmpVec3[i]);
				i++;
			}

			// Since only 32 error maps could be processed at a time, combine all the 
			// results here into 1 depth
			shaders.errorToDepthShader1.use();
			shaders.errorToDepthShader1.setVec2("layer_to_depth", layer_to_depth);
			framebuffers.FindLowestErrorDepth1(textures.tmpVec3, &shaders.errorToDepthShader1, /*out*/ textures.mvs_rough[mainId], textures.masks[mainId]);
		}
	}

private:

	void ChooseDepthPerLayer(int keyCamId, /*out*/std::vector<float>& depthPerLayer, glm::vec2& layer_to_depth) {
		// wider margin
		float near = depthRanges[keyCamId].x * 0.95f;
		float far  = depthRanges[keyCamId].y * 1.2f;
		//printf("---> %s depth range = [%f, %f]:\n", extrinsics.imageNames[keyCamId].c_str(), near, far);

		depthPerLayer = std::vector<float>(nrLayers, 0);
		layer_to_depth = glm::vec2((far - near) / (nrLayers * nrLayers - 2 * nrLayers), near);
		for (int layer = 0; layer < nrLayers; layer++) {
			depthPerLayer[layer] = layer_to_depth.x * layer * layer + layer_to_depth.y;
		}
		
	}

};


#endif // !MULTIVIEWSTEREO_H
