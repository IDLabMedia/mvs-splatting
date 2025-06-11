#ifndef GL_FRAMEBUFFERCONTROLLER_H
#define GL_FRAMEBUFFERCONTROLLER_H


class FrameBufferController {
	// setup the singleton
public:
	static FrameBufferController& getInstance()
	{
		static FrameBufferController instance;
		return instance;
	}
private:
	FrameBufferController() {}

public:
	FrameBufferController(FrameBufferController const&) = delete;
	void operator=(FrameBufferController const&) = delete;

	// actual functionality
private:

	SfmPoints sfmPoints;
	int nrSfmPoints;

	// use a quad for copying/processing textures
	GLuint quadVAO, quadVBO = 0;
	GLuint sfmVAO, sfmVBO = 0;
	
	// use a point cloud to render a depth map
	GLuint pcVAO, pcVBO = 0;
	int nrPoints;
	
	// transform feedback
	GLuint tfbo;
	GLuint tf;
	GLuint tfVAO, tfVBO;
	GLuint tfQuery;
	int nrPointsTf;
	
	GLuint depthRBO;
	

public:
	float tfSubdivisions = 3;
	// framebuffers
	GLuint fbo;
	GLuint fbo2; // 2 draw buffers

	bool Init(SfmPoints sfmPoints, int width, int height, int nrKeyCams) {

		TexController& textures = TexController::getInstance();

		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glBindTexture(GL_TEXTURE_2D, textures.fbo_ca0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures.fbo_ca0, 0);
		glGenRenderbuffers(1, &depthRBO);
		glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); // TODO only works because everything here has same resolution
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawBuffers);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cerr << "Framebuffer fbo not complete!" << std::endl;
			return false;
		}

		glGenFramebuffers(1, &fbo2);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo2);
		glBindTexture(GL_TEXTURE_2D, textures.fbo2_ca0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures.fbo2_ca0, 0);
		glBindTexture(GL_TEXTURE_2D, textures.fbo2_ca1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textures.fbo2_ca1, 0);
		GLenum drawBuffers2[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, drawBuffers2);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cerr << "Framebuffer fbo2 not complete!" << std::endl;
			return false;
		}



		// Setup a quad that fills the screen for processing an entire texture
		{
			float quadVertices[] = {
				// positions (NDC)  // texCoords
				 1.0f,  1.0f,       1.0f, 1.0f,  // 0 top right
				 1.0f, -1.0f,       1.0f, 0.0f,  // 1 bottom right
				-1.0f,  1.0f,       0.0f, 1.0f,  // 3 top left 
				 1.0f, -1.0f,       1.0f, 0.0f,  // 1 bottom right
				-1.0f, -1.0f,       0.0f, 0.0f,  // 2 bottom left
				-1.0f,  1.0f,       0.0f, 1.0f   // 3 top left 
			};
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

			// Define VAO, VBO to render the Sfm point cloud
			glGenVertexArrays(1, &sfmVAO);
			glGenBuffers(1, &sfmVBO);

			// for the VBO, put all the poins (xyz positions) and colors in 1 float array
			std::unordered_map<int, std::vector<glm::vec3>> points = sfmPoints.points;
			std::unordered_map<int, std::vector<glm::vec3>> colors = sfmPoints.colors;
			std::vector<float> sfmVboData;
			for (const auto& pair : points) {
				int id = pair.first;

				for (size_t i = 0; i < points[id].size(); ++i) {
					sfmVboData.push_back(points[id][i].x);
					sfmVboData.push_back(points[id][i].y);
					sfmVboData.push_back(points[id][i].z);
					sfmVboData.push_back(colors[id][i].r);
					sfmVboData.push_back(colors[id][i].g);
					sfmVboData.push_back(colors[id][i].b);
				}
			}
			nrSfmPoints = sfmVboData.size() / 6;

			glBindVertexArray(sfmVBO);
			glBindBuffer(GL_ARRAY_BUFFER, sfmVBO);
			glBufferData(GL_ARRAY_BUFFER, sfmVboData.size(), sfmVboData.data(), GL_STATIC_DRAW);

			// Position Attribute (3 floats)
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
			// Color Attribute (3 floats, normalized)
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // TODO use GL_UNSIGNED_BYTE
		}


		// Setup a point cloud
		{
			nrPoints = width * height;
			float width_f = static_cast<float>(width);
			float height_f = static_cast<float>(height);
			float* pcTexCoords = new float[nrPoints * 2];
			int t = 0;
			for (int row = 0; row < height; row++) {
				for (int col = 0; col < width; col++) {
					// 2 floats (u and v) per pixel
					pcTexCoords[t] = (col + 0.5f) / width_f;
					pcTexCoords[t + 1] = (row + 0.5f) / height_f;
					t += 2;
				}
			}
			glGenVertexArrays(1, &pcVAO);
			glGenBuffers(1, &pcVBO);
			glBindVertexArray(pcVAO);
			glBindBuffer(GL_ARRAY_BUFFER, pcVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * nrPoints * 2, pcTexCoords, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			delete[] pcTexCoords;
		}

		// Setup transform feedback
		{
			// limit the number of outputted splats to 300k
			const int maxNrSplats = 300000;
			const int minNrSplats = 100000;
			int estNrSplats = width * height * nrKeyCams / float(tfSubdivisions * tfSubdivisions) * 0.6f;
			printf("w = %d, h = %d, cams = %d, tfSubdivisions = %f, estNrSplats = %d\n", width, height, nrKeyCams, tfSubdivisions, estNrSplats);
			if (estNrSplats < minNrSplats  || estNrSplats > maxNrSplats) {
				printf("Changed tfSubdivisions from %f", tfSubdivisions);
				tfSubdivisions = std::sqrt(float(width * height * nrKeyCams * 0.6f) / (estNrSplats > maxNrSplats? maxNrSplats : minNrSplats));
				printf(" to %f\n", tfSubdivisions);
			}
			int width_tf = width / tfSubdivisions;
			int height_tf = height / tfSubdivisions;
			nrPointsTf = width_tf * height_tf;
			float width_f = static_cast<float>(width_tf);
			float height_f = static_cast<float>(height_tf);
			float* tfTexCoords = new float[nrPointsTf * 2];
			int t = 0;
			for (int row = 0; row < height_tf; row++) {
				for (int col = 0; col < width_tf; col++) {
					// 2 floats (u and v) per pixel
					tfTexCoords[t] = (col + 0.5f) / width_f;
					tfTexCoords[t + 1] = (row + 0.5f) / height_f;
					t += 2;
				}
			}

			glGenVertexArrays(1, &tfVAO);
			glBindVertexArray(tfVAO);
			glGenBuffers(1, &tfVBO);
			glBindBuffer(GL_ARRAY_BUFFER, tfVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)* nrPointsTf * 2, tfTexCoords, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			
			// 7 floats per point: x, y, z, r, g, b, scale
			glGenBuffers(1, &tfbo);
			glBindBuffer(GL_ARRAY_BUFFER, tfbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * nrPointsTf * 7, nullptr, GL_DYNAMIC_READ);

			// Transform feedback setup
			glGenTransformFeedbacks(1, &tf);
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tf);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tfbo);

			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tf);

			glGenQueries(1, &tfQuery);
			
			delete[] tfTexCoords;
		}

		return true;
	}
	
	void RenderSfmPoints() {
		glBindVertexArray(sfmVAO);
		glDrawArrays(GL_POINTS, 0, nrSfmPoints);
	}

	void RenderDepthMap(GLuint colorTex, GLuint maskTex, GLuint depthTex) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, maskTex);
		glBindVertexArray(pcVAO);
		glDrawArrays(GL_POINTS, 0, nrPoints);
	}

	void RenderImage(GLuint colorTex) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorTex);
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	
	// ------ Rough NVS
	void RenderQuadWithOneDepth(GLuint mainColorTex, GLuint neighborColorTex, /*out*/ GLuint outputTex) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outputTex, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mainColorTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, neighborColorTex);
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void SumNeighborTextures(GLuint colorTex, std::vector<GLuint> inputTexs, Shader* shader, /*out*/ GLuint outputTex) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outputTex, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		int nrTextures = inputTexs.size();
		for (int i = 0; i < nrTextures; i++) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, inputTexs[i]);
			shader->setInt("errorTex[" + std::to_string(i) + "]", i);
		}
		glActiveTexture(GL_TEXTURE0 + nrTextures);
		glBindTexture(GL_TEXTURE_2D, colorTex);
		shader->setInt("colorTex", nrTextures);
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void FindLowestErrorDepth0(std::vector<GLuint> inputTexs, int nrTextures, int offset, Shader* shader, /*out*/ GLuint outputTex) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outputTex, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (int i = 0; i < nrTextures; i++) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, inputTexs[offset + i]);
			shader->setInt("errorTex[" + std::to_string(i) + "]", i);
		}
		shader->setInt("nrTextures", nrTextures);
		shader->setInt("offset", offset);
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void FindLowestErrorDepth1(std::vector<GLuint> inputTexs, Shader* shader, /*out*/ GLuint depthTex, GLuint maskTex) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo2);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, depthTex, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, maskTex, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		int nrTextures = inputTexs.size();
		for (int i = 0; i < nrTextures; i++) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, inputTexs[i]);
			shader->setInt("inputTex[" + std::to_string(i) + "]", i);
		}
		shader->setInt("nrTextures", nrTextures);
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	
	// ------ Masking off bad pixels
	void ProcessTex(GLuint inputTex, GLuint outputTex) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outputTex, 0);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, inputTex);
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void MaskOffBadlyProjectedPixels(GLuint mainColorTex, GLuint mainDepthTex, GLuint neighborColorTex, GLuint neighborDepthTex, /*out*/GLuint maskTex) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, maskTex, 0);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mainColorTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mainDepthTex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, neighborColorTex);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, neighborDepthTex);
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	
	// ------ Write splats to file
	void WriteSplatsToBuffer(GLuint colorTex, GLuint depthTex, GLuint maskTex, /*out*/std::vector<float> &buffer) {
		
		glBindVertexArray(tfVAO);
		glEnable(GL_RASTERIZER_DISCARD); // Skip fragment stage
		glActiveTexture(GL_TEXTURE0); 
		glBindTexture(GL_TEXTURE_2D, colorTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, maskTex);
		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, tfQuery);
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, nrPointsTf);
		glEndTransformFeedback();
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
		glDisable(GL_RASTERIZER_DISCARD);

		// get the exact nr of output points
		GLuint numOutputPrimitives = 0;
		glGetQueryObjectuiv(tfQuery, GL_QUERY_RESULT, &numOutputPrimitives);

		// copy to buffer
		GLfloat* feedback = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
		buffer = std::vector<float>(feedback, feedback + numOutputPrimitives * 7);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	
	void Cleanup() {
		glDeleteVertexArrays(1, &quadVAO);
		glDeleteVertexArrays(1, &sfmVAO);
		glDeleteVertexArrays(1, &pcVAO);
		glDeleteVertexArrays(1, &tfVAO);
		glDeleteBuffers(1, &sfmVBO);
		glDeleteBuffers(1, &pcVBO);
		glDeleteBuffers(1, &tfVBO);
		glDeleteFramebuffers(1, &fbo);
		glDeleteRenderbuffers(1, &depthRBO);
		glDeleteBuffers(1, &tfbo);
		glDeleteTransformFeedbacks(1, &tf);
		glDeleteQueries(1, &tfQuery);
	}

};


#endif
