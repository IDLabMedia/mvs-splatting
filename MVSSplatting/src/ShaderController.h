#ifndef GL_SHADERCONTROLLER_H
#define GL_SHADERCONTROLLER_H


class ShaderController {	
	// setup the singleton
public:
	static ShaderController& getInstance()
	{
		static ShaderController instance;
		return instance;
	}
private:
	ShaderController() {}

public:
	ShaderController(ShaderController const&) = delete;
	void operator=(ShaderController const&) = delete;

	// actual functionality
public:
	Shader sfmPointsShaders;
	Shader depthMapShader;
	Shader showImageShader;

	// rough depth map
	Shader quadDepthErrorShader;
	Shader sumRadiusShader;
	Shader errorToDepthShader0;
	Shader errorToDepthShader1;

	// splat generation
	Shader maskBadPixelsShader;
	Shader writeSplats;

	const std::string basePath = cmakelists_dir + "/src/shaders/";

public:

	bool Init(Intrinsics intrinsics, int width_g, int height_g, float scale_g) {

		std::cout << "Reading GLSL files from " << basePath << std::endl;

		// render mesh
		if (!CompileShader(sfmPointsShaders, "sfm_points.vs", "sfm_points.fs")) return false;
		if (!CompileShader(depthMapShader, "depthmap.vs", "depthmap.fs")) return false;
		if (!CompileShader(showImageShader, "copy_tex.vs", "copy_tex.fs")) return false;
		if (!CompileShader(quadDepthErrorShader, "copy_tex.vs", "quad_depth_error.fs")) return false;
		if (!CompileShader(sumRadiusShader, "copy_tex.vs", "sum_radius2.fs")) return false;
		if (!CompileShader(errorToDepthShader0, "copy_tex.vs", "error2depth0.fs")) return false;
		if (!CompileShader(errorToDepthShader1, "copy_tex.vs", "error2depth1.fs")) return false;
		if (!CompileShader(maskBadPixelsShader, "copy_tex.vs", "mask_bad_pixels3.fs")) return false;
		if (!CompileShader(writeSplats, "write_splats.vs", "", "write_splats.gs")) return false;

		sfmPointsShaders.use();
		sfmPointsShaders.setFloat("width", static_cast<float>(width_g));
		sfmPointsShaders.setFloat("height", static_cast<float>(height_g));
		sfmPointsShaders.setVec2("focal", glm::vec2(intrinsics.fx, intrinsics.fy) * scale_g);
		sfmPointsShaders.setVec2("pp", glm::vec2(intrinsics.cx, intrinsics.cy) * scale_g);
		sfmPointsShaders.setVec2("out_near_far", glm::vec2(0.1f, 500.0f));

		depthMapShader.use();
		depthMapShader.setInt("colorTex", 0); 
		depthMapShader.setInt("depthTex", 1); 
		depthMapShader.setInt("maskTex", 2); 
		depthMapShader.setFloat("width", static_cast<float>(intrinsics.width));
		depthMapShader.setFloat("height", static_cast<float>(intrinsics.height));
		depthMapShader.setVec2("in_f", glm::vec2(intrinsics.fx, intrinsics.fy));
		depthMapShader.setVec2("in_pp", glm::vec2(intrinsics.cx, intrinsics.cy));
		depthMapShader.setFloat("out_width", static_cast<float>(width_g));
		depthMapShader.setFloat("out_height", static_cast<float>(height_g));
		depthMapShader.setVec2("out_near_far", glm::vec2(0.1f, 500.0f));
		depthMapShader.setVec2("out_f", glm::vec2(intrinsics.fx, intrinsics.fy) * scale_g);
		depthMapShader.setVec2("out_pp", glm::vec2(intrinsics.cx, intrinsics.cy) * scale_g);
		depthMapShader.setFloat("triangle_deletion_margin", 0.14f  ); // determines how elongated a triangle must be do be deleted
		depthMapShader.setInt("showDepth", 0);
		depthMapShader.setInt("useMask", 1);

		showImageShader.use();
		showImageShader.setInt("inputTex", 0);

		quadDepthErrorShader.use();
		quadDepthErrorShader.setInt("mainColorTex", 0);
		quadDepthErrorShader.setInt("neighborColorTex", 1);
		quadDepthErrorShader.setFloat("width", static_cast<float>(intrinsics.width));
		quadDepthErrorShader.setFloat("height", static_cast<float>(intrinsics.height));
		quadDepthErrorShader.setVec2("focal", glm::vec2(intrinsics.fx, intrinsics.fy));
		quadDepthErrorShader.setVec2("pp", glm::vec2(intrinsics.cx, intrinsics.cy));

		sumRadiusShader.use();
		sumRadiusShader.setFloat("width", static_cast<float>(intrinsics.width));
		sumRadiusShader.setFloat("height", static_cast<float>(intrinsics.height));
		
		maskBadPixelsShader.use();
		maskBadPixelsShader.setInt("mainColorTex", 0);
		maskBadPixelsShader.setInt("mainDepthTex", 1);
		maskBadPixelsShader.setInt("neighborColorTex", 2);
		maskBadPixelsShader.setInt("neighborDepthTex", 3);
		maskBadPixelsShader.setFloat("width", static_cast<float>(intrinsics.width));
		maskBadPixelsShader.setFloat("height", static_cast<float>(intrinsics.height));
		maskBadPixelsShader.setVec2("focal", glm::vec2(intrinsics.fx, intrinsics.fy));
		maskBadPixelsShader.setVec2("pp", glm::vec2(intrinsics.cx, intrinsics.cy));

		const char* varyings[] = { "out_position", "out_color", "out_scale" };
		if (!writeSplats.InitTransformFeedShader(3, varyings)) return false;
		writeSplats.use();
		writeSplats.setInt("colorTex", 0);
		writeSplats.setInt("depthTex", 1);
		writeSplats.setInt("maskTex", 2);

		return true;
	}

private:
	bool CompileShader(Shader& shaderToCompile, std::string vertex, std::string fragment) {
		if (!shaderToCompile.init((basePath + vertex).c_str(), (basePath + fragment).c_str())) {
			printf("Error: failed to compile %s or %s\n", vertex.c_str(), fragment.c_str());
			return false;
		}
		return true;
	}

	bool CompileShader(Shader& shaderToCompile, std::string vertex, std::string fragment, std::string geometry) {
		if (fragment == "") {
			if (!shaderToCompile.init((basePath + vertex).c_str(), nullptr, (basePath + geometry).c_str())) {
				printf("Error: failed to compile %s or %s\n", vertex.c_str(), geometry.c_str());
				return false;
			}
		}
		else {
			if (!shaderToCompile.init((basePath + vertex).c_str(), (basePath + fragment).c_str(), (basePath + geometry).c_str())) {
				printf("Error: failed to compile %s or %s or %s\n", vertex.c_str(), fragment.c_str(), geometry.c_str());
				return false;
			}
		}
		
		return true;
	}
};

#endif
