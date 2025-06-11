#ifndef GL_TEX_CONTROLLER_H
#define GL_TEX_CONTROLLER_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <unordered_set>

class TexController {
	// setup the singleton
public:
	static TexController& getInstance()
	{
		static TexController instance;
		return instance;
	}
private:
	TexController() {}
	

public:
	TexController(TexController const&) = delete;
	void operator=(TexController const&) = delete;

	// actual functionality
private:

	Intrinsics intrinsics;

public:
	
	std::map<int, GLuint> images;
	std::map<int, GLuint> mvs_rough;
	std::map<int, GLuint> masks;
	std::vector<GLuint> tmpFloat_neighbors;
	std::vector<GLuint> tmpFloat_layers;
	std::vector<GLuint> tmpVec3;

	// dummy textures for framebuffers
	GLuint fbo_ca0;
	GLuint fbo2_ca0;
	GLuint fbo2_ca1;

public:
	
	bool Init(Intrinsics intrinsics, Extrinsics extrinsics, std::vector<int> keyCamIds, std::map<int, std::vector<int>> mvsNeighbors, std::string imagesPath, int nrMvsNeighbors, int nrMvsLayers) {
		this->intrinsics = intrinsics;

		std::vector<GLubyte> data(intrinsics.width * intrinsics.height, 255);

				for (int& id : keyCamIds) {
			// depth maps
			GLuint texture;
			glGenTextures(1, &texture);
			glDefineTexture(texture, GL_R32F, intrinsics.width, intrinsics.height, GL_RED, GL_FLOAT);
			mvs_rough[id] = texture;
			// masks
			glGenTextures(1, &texture);
			glDefineTexture(texture, GL_R8, intrinsics.width, intrinsics.height, GL_RED, GL_UNSIGNED_BYTE);
			masks[id] = texture;
		}
		
		// calculate which images need to be loaded (only those of key cameras and their neighbors)
		std::unordered_set<int> imageIdsToLoad;
		for (int& keyCamId : keyCamIds) {
			imageIdsToLoad.insert(keyCamId);
			for (int& neighborId : mvsNeighbors[keyCamId]) {
				imageIdsToLoad.insert(neighborId);
			}
		}

		bool checkedAlignment = false;
		for (const int& id : imageIdsToLoad) {
			// images
			std::string filename = imagesPath + extrinsics.imageNames[id];
			int width, height, channels;
			unsigned char* image = stbi_load(filename.c_str(), &width, &height, &channels, 0);
			if (!image) {
				std::cout << "Error: failed to load image " << filename << std::endl;
				return false;
			}

			if (!checkedAlignment && width % 4 != 0) {
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				checkedAlignment = true;
			}

			GLuint texture;
			glGenTextures(1, &texture);
			glDefineTexture(texture, GL_RGB8, width, height, GL_RGB, GL_UNSIGNED_BYTE, image, false);
			images[id] = texture;
			stbi_image_free(image);
		}

		// textures for intermediate calculations during multi-view stereo
		tmpFloat_neighbors = std::vector<GLuint>(nrMvsNeighbors, 0);
		for (int n = 0; n < nrMvsNeighbors; n++) {
			glGenTextures(1, &(tmpFloat_neighbors[n]));
			glDefineTexture(tmpFloat_neighbors[n], GL_R32F, intrinsics.width, intrinsics.height, GL_RED, GL_FLOAT, 0);
		}

		tmpFloat_layers = std::vector<GLuint>(nrMvsLayers, 0);
		for (int n = 0; n < nrMvsLayers; n++) {
			glGenTextures(1, &(tmpFloat_layers[n]));
			glDefineTexture(tmpFloat_layers[n], GL_R32F, intrinsics.width, intrinsics.height, GL_RED, GL_FLOAT, 0);
		}

		// framebuffer dummy textures
		glGenTextures(1, &fbo_ca0);
		glGenTextures(1, &fbo2_ca0);
		glGenTextures(1, &fbo2_ca1);
		glDefineTexture(fbo_ca0, GL_R32F, intrinsics.width, intrinsics.height, GL_RED, GL_FLOAT, 0);
		glDefineTexture(fbo2_ca0, GL_R32F, intrinsics.width, intrinsics.height, GL_RED, GL_FLOAT, 0);
		glDefineTexture(fbo2_ca1, GL_R8, intrinsics.width, intrinsics.height, GL_RED, GL_UNSIGNED_BYTE);

		return true;
	}

	void CreateTmpVec3(int nrTextures) {
		tmpVec3 = std::vector<GLuint>(nrTextures, 0);
		for (int n = 0; n < nrTextures; n++) {
			glGenTextures(1, &(tmpVec3[n]));
			glDefineTexture(tmpVec3[n], GL_RGB32F, intrinsics.width, intrinsics.height, GL_RGB, GL_FLOAT, 0);
		}
	}

	void Cleanup() {
		for (auto const& pair : images) {
			glDeleteTextures(1, &pair.second);
		}
		images.clear();

		for (auto const& pair : mvs_rough) {
			glDeleteTextures(1, &pair.second);
		}
		mvs_rough.clear();

		for (auto const& pair : masks) {
			glDeleteTextures(1, &pair.second);
		}
		masks.clear();
		
		for (GLuint& t : tmpFloat_neighbors) {
			glDeleteTextures(1, &t);
		}
		tmpFloat_neighbors.clear();
		
		for (GLuint& t : tmpFloat_layers) {
			glDeleteTextures(1, &t);
		}
		tmpFloat_layers.clear();

		for (GLuint& t : tmpVec3) {
			glDeleteTextures(1, &t);
		}
		tmpVec3.clear();

		glDeleteTextures(1, &fbo_ca0);
		glDeleteTextures(1, &fbo2_ca0);
		glDeleteTextures(1, &fbo2_ca1);
		
	}
	
private:
	static void glDefineTexture(GLuint tex, GLint internalformat, int w, int h, GLenum format, GLenum type, void* data = NULL, bool nearest=true) {
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, w, h, 0, format, type, data == NULL ? 0 : data);
	}
};

#endif
