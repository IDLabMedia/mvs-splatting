#ifndef GUI_H
#define GUI_H

#include <gtc/type_ptr.hpp>

class GuiCamera {
private:

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = 0.0f; 
    float pitch = 0.0f;
    float roll = 0.0f;

    glm::vec3 cameraVelocity = glm::vec3(0);
    glm::vec3 cameraAcceleration = glm::vec3(0);
    glm::vec2 rotationVelocity = glm::vec2(0);
    glm::vec2 rotationAcceleration = glm::vec2(0);

    float maxSpeed = 30.0f;        
    float accelRate = 30.0f;       
    float decelRate = 5.0f;        
    float maxRotSpeed = 360;       
    float rotAccelRate = 360.0f;   
    float rotDecelRate = 5.0f;    

    // when moving to a certain viewpoint
    glm::mat4 modelToMoveTo;

public:
    bool shouldMoveToModel = false;
    glm::mat4 view;

    GuiCamera() {}

    GuiCamera(CameraPose initPose) {

        cameraPos = initPose.pos;
        cameraFront = initPose.forward;
        cameraUp = glm::normalize(glm::vec3(initPose.model[1]));

        yaw = glm::degrees(atan2(cameraFront.z, cameraFront.x));
        pitch = glm::degrees(asin(cameraFront.y));
    }

    void ProcessInput(GLFWwindow* window, bool& quit) {
        // if a key is pressed, override shouldMoveToModel to false
        if (shouldMoveToModel && (
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)) {
            shouldMoveToModel = false;
        }
        // check if we should quit
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) quit = true;

        float deltaTime = 0.016f; // TODO;
        glm::vec3 moveDir(0.0f);
        glm::vec2 rotDir(0.0f);

        if (shouldMoveToModel) {
            float interpolationFactor = glm::clamp(deltaTime * 5.0f, 0.0f, 1.0f);
            cameraPos = glm::mix(cameraPos, glm::vec3(modelToMoveTo[3]), interpolationFactor);
            glm::mat3 targetRotation = glm::mat3(modelToMoveTo);
            cameraFront = glm::normalize(glm::mix(cameraFront, -targetRotation[2], interpolationFactor));
            cameraUp = glm::normalize(glm::mix(cameraUp, targetRotation[1], interpolationFactor));
            yaw = glm::degrees(atan2(cameraFront.z, cameraFront.x));
            pitch = glm::degrees(asin(cameraFront.y));
            if (interpolationFactor > 0.99f) {
                shouldMoveToModel = false;
            }
        }
        else {

            // Camera movement (WASDQE)
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir -= cameraUp;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir += cameraUp;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= glm::normalize(glm::cross(cameraFront, cameraUp));
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += glm::normalize(glm::cross(cameraFront, cameraUp));
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) moveDir -= cameraFront;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) moveDir += cameraFront;

            // Camera rotation (IJKL)
            if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) rotDir.y += 1.0f;
            if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) rotDir.y -= 1.0f;
            if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) rotDir.x += 1.0f;
            if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) rotDir.x -= 1.0f;

            // Apply acceleration
            cameraAcceleration = moveDir * accelRate;
            rotationAcceleration = rotDir * rotAccelRate;

            // Update velocity with acceleration and deceleration
            cameraVelocity *= glm::max(0.0f, 1.0f - decelRate * deltaTime);
            cameraVelocity += cameraAcceleration * deltaTime;
            cameraVelocity = glm::clamp(cameraVelocity, -maxSpeed, maxSpeed);

            rotationVelocity *= glm::max(0.0f, 1.0f - rotDecelRate * deltaTime);
            rotationVelocity += rotationAcceleration * deltaTime;
            rotationVelocity = glm::clamp(rotationVelocity, -maxRotSpeed, maxRotSpeed);

            // Apply movement and rotation
            cameraPos += cameraVelocity * deltaTime;
            yaw += rotationVelocity.x * deltaTime;
            pitch += rotationVelocity.y * deltaTime;

            // Clamp pitch
            pitch = glm::clamp(pitch, -89.0f, 89.0f);

            // Update front vector
            cameraFront = glm::normalize(glm::vec3(
                cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
                sin(glm::radians(pitch)),
                sin(glm::radians(yaw)) * cos(glm::radians(pitch))
            ));
        }

        // Update view matrix
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    }

    void MoveTo(glm::mat4 model) {
        shouldMoveToModel = true;
        modelToMoveTo = model;
    }
};

class Gui {

private:
    GLFWwindow* window = NULL;
    ShaderController& shaders;
    FrameBufferController& framebuffers;
    TexController& textures;
    Intrinsics intrinsics;
    Extrinsics extrinsics;
	int nrMvsLayers;

    GuiCamera camera_g;
    std::vector<int> keyCameras;

    // gui options
    std::vector<bool> renderKeyView;
    bool showDepth = false;
    float pointsize = 3;
    int camToJumpTo = 0;
    bool showGroundtruth = false;
    bool showSfmPoints = false;
    bool enableMask = true;

public:

    int width_g = 800;
    int height_g = 600;
    float scale_g = 1;

	Gui(Intrinsics intrinsics, Extrinsics extrinsics, int nrMvsLayers) :
        shaders(ShaderController::getInstance()), 
        framebuffers(FrameBufferController::getInstance()), 
        textures(TexController::getInstance()), 
        intrinsics(intrinsics),
        extrinsics(extrinsics),
		nrMvsLayers(nrMvsLayers) {
        float aspect = static_cast<float>(intrinsics.height) / intrinsics.width;
        if (aspect > 1) {
            // height > width
            height_g = 1000;
            width_g = 1000 / aspect;
        }
        else {
            // width > height
            width_g = 1800;
            height_g = 1800 * aspect;
        }
        scale_g = static_cast<float>(width_g) / intrinsics.width;
    }

	bool InitWindow(bool headless) {
        glfwInit();
        if(headless) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window = glfwCreateWindow(width_g, height_g, "Depth Estimation", NULL, NULL);
        glfwMakeContextCurrent(window);
        
        if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD\n";
            return false;
        }

        int xpos, ypos;
        glfwGetMonitorPos(glfwGetPrimaryMonitor(), &xpos, &ypos);
        glfwSetWindowPos(window, xpos, ypos + 20); // Move window to top-left

        glEnable(GL_DEPTH_TEST);
        glPointSize(3);
        return true;
	}

    void Run(std::vector<int> keyCameras) {
        this->keyCameras = keyCameras;

        // setup camera and gui options
        camera_g = GuiCamera(extrinsics.poses[keyCameras[0]]);
        renderKeyView = std::vector<bool>(keyCameras.size(), true);

        // prepare OpenGL and ImGui for the render loop
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width_g, height_g);
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        // render loop
        printf("Start gui loop\n");
        bool quit = false;
        while (!glfwWindowShouldClose(window) && !quit) {

            // move camera
            camera_g.ProcessInput(window, quit);

            // render to screen
            RenderImGUI();
            RenderOpenGL();

            // finalize
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        // cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void RenderImGUI() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Options");
        if (ImGui::Checkbox("show depth", &showDepth)) {
            shaders.depthMapShader.use();
            shaders.depthMapShader.setInt("showDepth", showDepth ? 1 : 0);
        }
        ImGui::SameLine();
        ImGui::Checkbox("show sfm points", &showSfmPoints);
        ImGui::SameLine();
        if (ImGui::Checkbox("enable mask", &enableMask)) {
            shaders.depthMapShader.use();
            shaders.depthMapShader.setInt("useMask", enableMask ? 1 : 0);
        }
        if (ImGui::SliderFloat("pointsize", &pointsize, 1, 8)) {
            glPointSize(pointsize);
        }
        if (ImGui::InputInt("Jump to", &camToJumpTo, 1, 10)) {
            camToJumpTo = std::min(std::max(0, camToJumpTo), static_cast<int>(extrinsics.imageIds.size()) - 1);
            camera_g.MoveTo(extrinsics.poses[extrinsics.imageIds[camToJumpTo]].model);
        }
        ImGui::SameLine();
        ImGui::Checkbox("Show groundtruth", &showGroundtruth);
        if (ImGui::Button("Show all")) {
            renderKeyView = std::vector<bool>(keyCameras.size(), true);
        }
        ImGui::SameLine();
        if (ImGui::Button("Hide all")) {
            renderKeyView = std::vector<bool>(keyCameras.size(), false);
        }

        for (size_t i = 0; i < renderKeyView.size(); i++) {
            int t = keyCameras[i];
            std::string label = extrinsics.imageNames[t];
            bool value = renderKeyView[i];
            ImGui::Checkbox(label.c_str(), &value);
            renderKeyView[i] = value;
        }
        ImGui::End();
    }

    void RenderOpenGL() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!showGroundtruth) {
            
            // depth maps
            shaders.depthMapShader.use();
            shaders.depthMapShader.setMat4("view", camera_g.view);
            for (size_t i = 0; i < renderKeyView.size(); i++) {
                int id = keyCameras[i];
                if (!renderKeyView[i]) continue;
                shaders.depthMapShader.setMat4("model", extrinsics.poses[id].model);
                framebuffers.RenderDepthMap(textures.images[id], textures.masks[id], textures.mvs_rough[id]);
            }

            // sfm points
            if (showSfmPoints) {
                shaders.sfmPointsShaders.use();
                shaders.sfmPointsShaders.setMat4("view", camera_g.view);
                framebuffers.RenderSfmPoints();
            }
        }
        else {
            shaders.showImageShader.use();
            framebuffers.RenderImage(textures.images[extrinsics.imageIds[camToJumpTo]]);
        }
    }

    ~Gui() {
        if(window != NULL) glfwDestroyWindow(window);
        glfwTerminate();
    }

};




#endif