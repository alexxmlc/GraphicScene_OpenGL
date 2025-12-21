#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "stb_image.h"

#include <iostream>


// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint isLightSourceLoc;
GLint ambientStrengthLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.1f;

GLboolean pressedKeys[1024];

// models
gps::Model3D myScene;
gps::Model3D lightSun;
gps::Model3D myLightBulbs;
gps::Model3D skyDome;

// Mouse control variables
float lastX = 512.0f; 
float lastY = 384.0f; 
float yaw = -90.0f;   
float pitch = 0.0f;  
bool firstMouse = true; 
float sensitivity = 0.1f; 

// shaders
gps::Shader myBasicShader;
gps::Shader skyboxShader;
gps::Shader depthMapShader;

// Wireframe mode
bool wireframeMode = false;

bool isFullscreen = false;
// Store the window state to restore it later
int savedXPos, savedYPos;
int savedWidth, savedHeight;

// Show texture
GLint showTexture;
int renderTexture = 1;

// for sun
GLfloat lightAngle = 0.0f;

// lanterns
glm::vec3 lanternPositions[] = {
    glm::vec3(-3.48753f, -15.2142f, 24.3723f),
    glm::vec3(-9.77415f, -16.2577f, 4.60281f),
    glm::vec3(-10.2632f, -17.2263f, -10.7292f),
    glm::vec3(-36.2527f, -15.4282f, 18.5812f),
    glm::vec3(-36.2642f, -15.3815f, 43.5164f),
    glm::vec3(-32.7196f, -17.4244f, -0.485049f)
};

// skybox
GLuint textureDay;
GLuint textureNight;

// Shadow Mapping variables
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

// Fog
GLint fogDensityLoc;
float fogDensity = 0.0f;

GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);

    if (height == 0) height = 1;

    glViewport(0, 0, width, height);

    projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    // Cand apesi tasta P, iti zice unde e camera
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        glm::vec3 pos = myCamera.cameraPosition; // Sau cum ai tu variabila camerei
        std::cout << "GLM Vec3: glm::vec3("
            << pos.x << "f, "
            << pos.y << "f, "
            << pos.z << "f)," << std::endl;
    }


    // Num 1: SOLID 
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        renderTexture = 1; 
    }

    // Num 2: WIREFRAME (Sarma)
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // Num 3: POLIGONAL 
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glPointSize(3.0f);
    }

    // Num 4: SMOOTH 
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        renderTexture = 0; 
    } glPointSize(3.0f);
    

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }

    // Fullscreen mode
    if (key == GLFW_KEY_F10 && action == GLFW_PRESS) {
        if (!isFullscreen) {
            glfwGetWindowPos(window, &savedXPos, &savedYPos);
            glfwGetWindowSize(window, &savedWidth, &savedHeight);

            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else {
            glfwSetWindowMonitor(window, nullptr, savedXPos, savedYPos, savedWidth, savedHeight, 0);
        }
        isFullscreen = !isFullscreen;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; 
    lastX = (float)xpos;
    lastY = (float)ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    myCamera.rotate(pitch, yaw);
}

void processMovement() {
    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
    }
    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
    }

    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle -= 0.5f;
    }
    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle += 0.5f;
    }

    // F = Fog (Creste ceata)
    if (pressedKeys[GLFW_KEY_F]) {
        fogDensity += 0.001f; 
        if (fogDensity > 0.05f) fogDensity = 0.05f; 
    }

    // G = Gone (Dispare ceata)
    if (pressedKeys[GLFW_KEY_G]) {
        fogDensity -= 0.001f; 
        if (fogDensity < 0.0f) fogDensity = 0.0f; 
    }
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project - Village");
}
void setWindowCallbacks() {
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);

    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f); //bright blue
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); 
    glDepthFunc(GL_LESS);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

GLuint LoadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    // load img to mem
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data); // free ram memory
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void initModels() {
    myScene.LoadModel("models/village/village.obj");
    lightSun.LoadModel("models/sun/sun.obj");
    myLightBulbs.LoadModel("models/lantern/lantern.obj");

    skyDome.LoadModel("models/sky/sphere.obj"); 
    textureDay = LoadTexture("models/sky/day.jpg");     
    textureNight = LoadTexture("models/sky/night.jpg");
}

void initShaders() {
    myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    skyboxShader.loadShader("shaders/skybox.vert", "shaders/skybox.frag");
    depthMapShader.loadShader("shaders/shadowMap.vert", "shaders/shadowMap.frag");
}

void initUniforms() {
    myBasicShader.useShaderProgram();

    // Model matrix 
    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // View matrix camera
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Normal matrix for light
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Projection matrix camera lens
    projection = glm::perspective(glm::radians(45.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 1000.0f); 
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // sun
    lightDir = glm::vec3(0.5f, 1.0f, 0.5f); // light can bee seen from above and sides
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // white light
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    showTexture = glGetUniformLocation(myBasicShader.shaderProgram, "showTexture");
    glUniform1i(showTexture, renderTexture);

    // light source
    isLightSourceLoc = glGetUniformLocation(myBasicShader.shaderProgram, "isLightSource");

    ambientStrengthLoc = glGetUniformLocation(myBasicShader.shaderProgram, "ambientStrength");

    myBasicShader.useShaderProgram();
    for (unsigned int i = 0; i < 6; i++) { 
        std::string number = std::to_string(i);

        // Pos
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, ("pointLights[" + number + "].position").c_str()), 1, glm::value_ptr(lanternPositions[i]));

        // Color
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, ("pointLights[" + number + "].color").c_str()), 1, glm::value_ptr(glm::vec3(4.0f, 3.2f, 2.4f)));

        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, ("pointLights[" + number + "].constant").c_str()), 1.0f);
        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, ("pointLights[" + number + "].linear").c_str()), 0.7f);
        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, ("pointLights[" + number + "].quadratic").c_str()), 1.8f);
    }

    fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
}

void initFBO() {
    // generate framebuffer
    glGenFramebuffers(1, &shadowMapFBO);

    // generate depth texture
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);

    // make it sexy
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // make the edges white (no weird shadows out of the map)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // add the texture to the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    // only need depth
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // get back to normal
}

void renderSkybox() {
    skyboxShader.useShaderProgram();

    // calc blend factor (how much day / how much night
    float blendFactor = 0.0f;
    if (lightDir.y > 0.1f)
        blendFactor = 1.0f;
    else if (lightDir.y < -0.1f)
        blendFactor = 0.0f;
    else {
        blendFactor = (lightDir.y + 0.1f) / 0.2f;
    }

    // send the blend factorto the shader
    glUniform1f(glGetUniformLocation(skyboxShader.shaderProgram, "blendFactor"), blendFactor);

    // matrix
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // make the sphere big boy
    glm::mat4 modelSky = glm::mat4(1.0f);
    modelSky = glm::scale(modelSky, glm::vec3(500.0f, 500.0f, 500.0f));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelSky));

    // set textures
    glUniform1i(glGetUniformLocation(skyboxShader.shaderProgram, "dayTexture"), 0);
    glUniform1i(glGetUniformLocation(skyboxShader.shaderProgram, "nightTexture"), 1);

    // activate day texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureDay);

    // activate night texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureNight);

    // draw
    skyDome.Draw(skyboxShader);
}

void renderScene(gps::Shader shader) {
    // ------------------------------------------------------------------------
    //  INITIALIZATION AND STUFF
    // ------------------------------------------------------------------------

    // sun rotation
    glm::mat4 lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    lightDir = glm::vec3(lightRotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

    // space matrix
    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
    glm::mat4 lightView = glm::lookAt(lightDir * 50.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

    // ------------------------------------------------------------------------
    // SHADOW PASS (texture depth)
    // ------------------------------------------------------------------------

    depthMapShader.useShaderProgram();

    // send light matrix to shadow shader
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    // set viewport at shadow res
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT); 

    // draw the village
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    myScene.Draw(depthMapShader); 

    // draw the lanterns
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    myLightBulbs.Draw(depthMapShader);

    // done with shadoes
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ------------------------------------------------------------------------
    // LIGHT PASS (final scene)
    // ------------------------------------------------------------------------

    // reset viueport to original
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    // skybox
    glDepthMask(GL_FALSE);
    renderSkybox();
    glDepthMask(GL_TRUE);

    // principal shader (myBasicShader)
    shader.useShaderProgram(); 

    // update view matrix
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // update dir and color of light
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    if (lightDir.y >= 0.0f) {
        lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform1f(ambientStrengthLoc, 0.4f);
    }
    else {
        lightColor = glm::vec3(0.05f, 0.05f, 0.1f);
        glUniform1f(ambientStrengthLoc, 0.02f);
    }
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    // fog
    glUniform1f(fogDensityLoc, fogDensity);

    // send texture and light matrix to main shader
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "shadowMap"), 3);

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    // drwa the village (with textures)
    glUniform1i(isLightSourceLoc, 0);
    glUniform1i(showTexture, renderTexture);
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // recalc normal matrix (for good lighting)
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    myScene.Draw(shader); // draw again but with main shader

    // draw the lanterns (light source)
    glUniform1i(isLightSourceLoc, 1);
    model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    myLightBulbs.Draw(shader);

    // draw sun
    glm::vec3 sunPosition = lightDir * 200.0f;
    model = glm::mat4(1.0f);
    model = glm::translate(model, sunPosition);
    model = glm::scale(model, glm::vec3(1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    lightSun.Draw(shader);
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char* argv[]) {

    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initFBO();
    initModels();
    initShaders();
    initUniforms();
    setWindowCallbacks();

    glCheckError();
    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
        renderScene(myBasicShader);

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}