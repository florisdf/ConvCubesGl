#include <boost/format.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <opencv2/opencv.hpp>

#include "shader.h"
#include "cube.h"
#include "camera.h"

#include <filesystem>
#include <iostream>
#include <bits/stdc++.h>
#include <set>

using directory_iterator = std::filesystem::directory_iterator;
using namespace std;
namespace fs = std::filesystem;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void saveFrameBuffer(const std::string& filename);
double randDouble();

enum class EasingType : int {
    LINEAR, IN_QUAD, OUT_QUAD, IN_OUT_QUAD,
    IN_CUBIC, OUT_CUBIC, IN_OUT_CUBIC,
    HOLD
};

const int TRANS_KEYFRAMES = 170; // Fixed keyframe count per instance

struct InstanceDataStill {
    float color[4];
    float position[3];
    float time;
};

struct InstanceDataTrans {
    float maxDuration;
    EasingType easing;
    int keyframeCount;
    float startTimes[TRANS_KEYFRAMES];
    int endIdxs[TRANS_KEYFRAMES];
};

struct ChanInfo {
    ChanInfo(int l, int c) : layer(l), channel(c) {};
    const int layer;
    const int channel;
};

ChanInfo pathToInfo(const fs::path &path) {
    std::regex del("_");
    std::string stem = path.stem();
    // Create a regex_token_iterator to split the string
    std::sregex_token_iterator it(stem.begin(), stem.end(), del, -1);
    string layer_num_str = *it;
    int layer_num = std::stoi(layer_num_str);
    int channel_num = std::stoi(*(++it));
    return {layer_num, channel_num};
}

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

const unsigned int FB_HEIGHT = 3840*4;
const unsigned int FB_WIDTH = 2160*4;

Camera camera(glm::vec3(0.0f, 0.0f, 50.0f));

unsigned int cubeVAO = 0, vertexVBO = 0;
unsigned int framebuffer = 0;
unsigned int textureColorbuffer = 0;
unsigned int rbo = 0;


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Face culling
    glEnable(GL_CULL_FACE);

    // Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader shader("../shaders/vertex.shader", "../shaders/fragment.shader");
    Shader screenShader("../shaders/quad_tex_vertex.shader", "../shaders/quad_tex_fragment.shader");

    // ---------------------------------------------------------
    set<fs::path> sorted_files;
    for (auto &entry : fs::directory_iterator("../scripts/layer_outputs")) {
        sorted_files.insert(entry.path());
    }

    int numCubes = 0;
    vector<fs::path> sel_files{};
    map<int, vector<fs::path>> layerChanFiles;
    map<int, map<int, int>> pxCumCount;
    int pathCounter = 0;
    for (const auto& path : sorted_files) {
        cv::Mat img;
        cv::imread(path.string(), img);
        const auto& cInfo = pathToInfo(path);
        pxCumCount[cInfo.layer][cInfo.channel] = numCubes;
        numCubes += img.rows * img.cols;
        sel_files.push_back(path);

        layerChanFiles[cInfo.layer].push_back(path);

        ++pathCounter;
        // if (pathCounter == 10) break;
    }

    vector<InstanceDataStill> instanceDataStill(numCubes);
    vector<InstanceDataTrans> instanceDataTrans(numCubes);

    float z = 0;
    int fileIdx = 0;
    const float CHANNEL_DIST = 1.;
    const float LAYER_DIST = 1.;
    const float LAYER_DURATION = 5.;
    const float LAYER_DELAY = 1;

    for (const auto& path : sel_files) {
        const auto& cInfo = pathToInfo(path);
        cv::Mat img1;
        cv::imread(path.string(), img1);
        cv::cvtColor(img1, img1, cv::COLOR_BGR2RGB);
        img1.convertTo(img1, CV_32FC1);
        img1 /= 255;

        float offsetX = -img1.cols/2;
        float offsetY = -img1.rows/2;
        if (cInfo.layer != 0 ) {
            if (cInfo.channel == 0) z += LAYER_DIST;
            z += CHANNEL_DIST;
        }

        float currLayerStartTime = (cInfo.layer - 1) * (LAYER_DURATION + LAYER_DELAY);  // When first pixel of first channel starts to appear
        int nChansCurrLayer = pxCumCount[cInfo.layer].size();
        float chanDuration = LAYER_DURATION / nChansCurrLayer;
        float currChanStartTime = currLayerStartTime + cInfo.channel * chanDuration;
        float currChanEndTime = currChanStartTime + chanDuration;

        float nextLayerStartTime = cInfo.layer * (LAYER_DURATION + LAYER_DELAY);  // When first pixel of first channel starts to appear

        // Still image + transition image
        int flatIdxStill = pxCumCount[cInfo.layer][cInfo.channel];
        int nColsNextLayer = img1.cols / 2;
        int nChansNextLayer = pxCumCount[cInfo.layer + 1].size();
        for (int y = 0; y < img1.rows; ++y)
        {
            for (int x = 0; x < img1.cols; ++x)
            {
                auto* stillData = &instanceDataStill[flatIdxStill];
                auto px = img1.at<cv::Vec3f>(y, x);
                copy_n(px.val, 3, stillData->color);
                stillData->color[3] = 1.0;
                stillData->time = currChanEndTime;
                float* posKf0Val = stillData->position;
                posKf0Val[0] = (x + offsetX); posKf0Val[1] = - (y + offsetY); posKf0Val[2] = z;

                auto* transData = &instanceDataTrans[flatIdxStill];
                transData->easing = EasingType::IN_OUT_QUAD;
                transData->keyframeCount = 0;
                transData->maxDuration = 0.5;

                for (auto item : pxCumCount[cInfo.layer + 1]) {
                    int chanIdx = item.first;
                    float chanDuration = LAYER_DURATION / nChansNextLayer;
                    float chanStartTime = nextLayerStartTime + chanIdx * chanDuration;
                    float chanEndTime = chanStartTime + chanDuration;
                    int chanFlatIdx0 = item.second;
                    int y2 = y/2;
                    int x2 = x/2;
                    int endFlatIdx = chanFlatIdx0 + (y2*nColsNextLayer) + x2;
                    transData->endIdxs[chanIdx] = endFlatIdx;
                    float a = glm::sin((float)y2/nColsNextLayer * glm::pi<float>()/2);
                    transData->startTimes[chanIdx] = glm::mix(chanStartTime, chanEndTime, a/2);
                    transData->keyframeCount++;
                    ++chanIdx;
                }
                ++flatIdxStill;
            }
        }
    }

    Cube cube(5, 1.0);

    // store instance data in an array buffer
    // --------------------------------------
    // Upload to SSBO
    GLuint ssboStill;
    glGenBuffers(1, &ssboStill);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboStill);
    glBufferData(GL_SHADER_STORAGE_BUFFER, instanceDataStill.size() * sizeof(InstanceDataStill), (const void*)instanceDataStill.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboStill);

    GLuint ssboTrans;
    glGenBuffers(1, &ssboTrans);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboTrans);
    glBufferData(GL_SHADER_STORAGE_BUFFER, instanceDataTrans.size() * sizeof(InstanceDataTrans), (const void*)instanceDataTrans.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboTrans);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<float> dataVec = cube.getInterleavedData();
    float vertexData[dataVec.size()];
    std::copy(dataVec.begin(), dataVec.end(), vertexData);

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &vertexVBO);
    // fill vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    // position attribute
    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // Vertices
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float))); // Normals
    glEnableVertexAttribArray(1);

    // create high res framebuffer to write to; the final display output will be an anti-aliased downscaled version of this
    // --------------------------------------------------------------------------------------------------------------------
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // generate texture
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FB_WIDTH, FB_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach it to currently bound framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

    // Create render buffer for depth and stencil buffers
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, FB_WIDTH, FB_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // attach the renderbuffer object to the depth and stencil attachment of the framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // check if the framebuffer is complete now, so we can render to it
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    // unbind buffer, rendering to display again
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // define vertex buffer for screen quad
    // ------------------------------------
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // shader config
    // -------------
    screenShader.use();
    screenShader.setInt("screenTexture", 0);

    // render loop
    // -----------
    bool saveFrame = true;
    int frameCount = 0;
    int startFrame = 0;
    auto startTime = chrono::steady_clock::now();
    float currentTime, prevTime;
    float fps = 60.;
    float maxTime = ((float)startFrame/fps) + 40;
    while (!glfwWindowShouldClose(window))
    {
        double t0Loop = (double)cv::getTickCount();
        // first pass rendering to high res framebuffer
        // --------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // update the viewport size
        glViewport(0, 0, FB_WIDTH, FB_HEIGHT);

        // draw instanced cubes
        shader.use();
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

        // camera/view transformation
        //glm::mat4 view = camera.GetViewMatrix();
        // glm::vec3 camPos{-56.f, 56.f, 20.f};
        // glm::vec3 camLookAt{-56.f, 56.f, 0.f};
        glm::vec3 camPos{-270.f, 0.f, 550.f};
        glm::vec3 camLookAt{0.f, 0.f, 320.f};
        glm::vec3 camUp{0.f, 1.f, 0.f};
        glm::mat4 view = glm::lookAt(camPos, camLookAt, camUp);

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        prevTime = currentTime;
        // currentTime = (chrono::duration<double>(chrono::steady_clock::now() - startTime)).count();
        currentTime = ((float) startFrame + frameCount) / fps;
        shader.setFloat("currentTime", currentTime);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        glBindVertexArray(cubeVAO);

        // Draw white borders
        glCullFace(GL_FRONT);
        shader.setBool("isOutline", true);
        shader.setBool("isStill", false);  // Transition cubes
        glDrawArraysInstanced(GL_TRIANGLES, 0, cube.getNumIndices(), numCubes);
        shader.setInt("isStill", true);  // Still cubes
        glDrawArraysInstanced(GL_TRIANGLES, 0, cube.getNumIndices(), numCubes);
        glCullFace(GL_BACK);

        // Draw colored cubes
        shader.setBool("isOutline", false);
        shader.setBool("isStill", false);  // Transition cubes
        glDrawArraysInstanced(GL_TRIANGLES, 0, cube.getNumIndices(), numCubes);
        shader.setInt("isStill", true);  // Still cubes
        glDrawArraysInstanced(GL_TRIANGLES, 0, cube.getNumIndices(), numCubes);

        //glDrawArrays(GL_TRIANGLES, 0, cube.getNumIndices());
        glBindVertexArray(0);

        // second pass rendering to screen
        // --------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        // update the viewport size
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

        screenShader.use();
        glBindVertexArray(quadVAO);

        // draw quad
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glGenerateMipmap(GL_TEXTURE_2D);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        // glfwSwapBuffers(window);

        glfwPollEvents();

        double t1Loop = (double)cv::getTickCount();
        double tLoop = (t1Loop - t0Loop) / cv::getTickFrequency();
        std::cout << "tLoop: " << tLoop << " s" << std::endl;

        if (saveFrame) {
            saveFrameBuffer(str(boost::format("frames/frame_%04d.png") % frameCount));
            cout << currentTime << endl;
            if ((startFrame + frameCount)/fps >= maxTime) break;
        }

        ++frameCount;
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &vertexVBO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &framebuffer);

    glfwTerminate();
    return 0;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void saveFrameBuffer(const std::string& filename) {
    int width = SCR_WIDTH;
    int height = SCR_HEIGHT;

    // Create a buffer to store pixel data
    std::vector<unsigned char> pixels(3 * width * height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Read pixels from the OpenGL framebuffer
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());

    // Convert the pixel buffer into an OpenCV Mat
    cv::Mat image(height, width, CV_8UC3, pixels.data());

    // Flip the image vertically (OpenGL's origin is bottom-left, OpenCV's is top-left)
    cv::flip(image, image, 0);

    // Save the image using OpenCV
    if (cv::imwrite(filename, image)) {
        std::cout << "Saved framebuffer to " << filename << std::endl;
    } else {
        std::cerr << "Failed to save image!" << std::endl;
    }
}

double randDouble() {
    return static_cast<double>(std::rand()) / RAND_MAX;
}
