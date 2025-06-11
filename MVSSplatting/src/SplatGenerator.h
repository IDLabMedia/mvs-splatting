#ifndef SPLAT_GENERATOR_H
#define SPLAT_GENERATOR_H


class SplatGenerator {

private:
    ShaderController& shaders;
    FrameBufferController& framebuffers;
    TexController& textures;
    Intrinsics intrinsics;
    Extrinsics extrinsics;
    std::vector<int> keyCamIds;
    std::map<int, std::vector<int>> neighborIds;
    std::map<int, std::vector<int>> mvsNeighbors;

public:
    SplatGenerator(Intrinsics intrinsics, Extrinsics extrinsics, std::vector<int> keyCamIds) :
        shaders(ShaderController::getInstance()),
        framebuffers(FrameBufferController::getInstance()),
        textures(TexController::getInstance()),
        intrinsics(intrinsics),
        extrinsics(extrinsics),
        keyCamIds(keyCamIds) { }

    // textures.masks: 1 means good pixel, 0 means throw away pixel
    void MaskAwayUnnecessaryPixels() {

        // render neighbor to main and mask off bad neighbor pixels
        shaders.maskBadPixelsShader.use();
        for (int& mainId : keyCamIds) {
            shaders.maskBadPixelsShader.setMat4("view", extrinsics.poses[mainId].view);

            for (int& neighborId : keyCamIds) {
                if (neighborId == mainId) continue; // skip same cameras
                shaders.maskBadPixelsShader.setMat4("model", extrinsics.poses[neighborId].model);
                framebuffers.MaskOffBadlyProjectedPixels(textures.images[mainId], textures.mvs_rough[mainId], textures.images[neighborId], textures.mvs_rough[neighborId], textures.masks[neighborId]);
            }
        }
    }

    void WriteToFile(std::string path, float tfSubdivisions, SfmPoints sfmPoints) {

        std::ofstream file(path, std::ios::binary);
        int totalNrPoints = 0;

        float diameter = tfSubdivisions * 0.5f / intrinsics.fx;
        shaders.writeSplats.use();
        shaders.writeSplats.setFloat("diameter", diameter);

        int width_tf = intrinsics.width / tfSubdivisions;
        int height_tf = intrinsics.height / tfSubdivisions;
        
        shaders.writeSplats.setFloat("width", static_cast<float>(width_tf));
        shaders.writeSplats.setFloat("height", static_cast<float>(height_tf));
        shaders.writeSplats.setVec2("focal", glm::vec2(intrinsics.fx * width_tf / intrinsics.width, intrinsics.fy * height_tf / intrinsics.height));
        shaders.writeSplats.setVec2("pp", glm::vec2(intrinsics.cx * width_tf / intrinsics.width, intrinsics.cy * height_tf / intrinsics.height));
        
        for (int& mainId : keyCamIds) {
            shaders.writeSplats.setMat4("model", extrinsics.poses[mainId].model);
            std::vector<float> buffer;
            framebuffers.WriteSplatsToBuffer(textures.images[mainId], textures.mvs_rough[mainId], textures.masks[mainId], buffer);
        
            int nrPoints = buffer.size() / 7;
            file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(float));
            totalNrPoints += nrPoints;
        }

        printf("Nr of splats from MVS: %d\n", totalNrPoints);
        totalNrPoints = 0;

        // also convert the Colmap point cloud to splats and write to file
        for (int& id : extrinsics.imageIds) {
            std::vector<glm::vec3> points = sfmPoints.points[id];
            std::vector<glm::vec3> colors = sfmPoints.colors[id];

            // per point, put (x,y,z,r,g,b,scale) in buffer
            std::vector<float> buffer(points.size() * 7);
            for (int i = 0; i < points.size(); i++) {
                int j = i * 7;
                buffer[j    ] = points[i].x;
                buffer[j + 1] = points[i].y;
                buffer[j + 2] = points[i].z;
                buffer[j + 3] = colors[i].x;
                buffer[j + 4] = colors[i].y;
                buffer[j + 5] = colors[i].z;
                buffer[j + 6] = glm::length(points[i] - extrinsics.poses[id].pos) * diameter;
            }

            file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(float));
            totalNrPoints += points.size();
        }
        
        printf("Nr of splats from Colmap: %d %s\n", totalNrPoints, extrinsics.eval? "(removed test set points)":"");
        printf("Wrote splats to %s\n", path.c_str());
        file.close();
    }
};

#endif // !SPLAT_GENERATOR_H
