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
    IN_QUART, OUT_QUART, IN_OUT_QUART,
    IN_EXPO, OUT_EXPO, IN_OUT_EXPO,
    IN_ELASTIC, OUT_ELASTIC, IN_OUT_ELASTIC,
    IN_BOUNCE, OUT_BOUNCE, IN_OUT_BOUNCE
};

const int MAX_KEYFRAMES = 4; // Fixed keyframe count per instance
struct Float4Keyframe {
    float value[4];
    float time;
    EasingType easing = EasingType::LINEAR;
};
struct Float3Keyframe {
    float value[3];
    float time;
    EasingType easing = EasingType::LINEAR;
};

struct InstanceData {
    Float4Keyframe colorKfs[MAX_KEYFRAMES];
    int numColorKfs = 0;
    Float3Keyframe positionKfs[MAX_KEYFRAMES];
    int numPositionKfs = 0;
};

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

    // generate a list of 100 cube locations/translation-vectors
    // ---------------------------------------------------------
    set<fs::path> sorted_files;
    for (auto &entry : fs::directory_iterator("../scripts/layer_outputs")) {
        sorted_files.insert(entry.path());
    }

    int numCubes = 0;
    vector<fs::path> sel_files{};
    map<int, vector<fs::path>> layerChanFiles;
    int pathCounter = 0;
    std::regex del("_");
    for (const auto& path : sorted_files) {
        cv::Mat img;
        cv::imread(path.string(), img);
        numCubes += 2 * img.rows * img.cols;
        sel_files.push_back(path);

        std::string stem = path.stem();
        // Create a regex_token_iterator to split the string
        std::sregex_token_iterator it(stem.begin(), stem.end(), del, -1);
        string layer_num_str = *it;
        int layer_num = std::stoi(layer_num_str);
        layerChanFiles[layer_num].push_back(path);

        ++pathCounter;
        if (pathCounter == 2) break;
    }

    vector<InstanceData> instanceData(numCubes);

    float z = 0;
    int flatIdx = 0;
    for (const auto& path : sel_files) {
        std::string stem = path.stem();
        // Create a regex_token_iterator to split the string
        std::sregex_token_iterator it(stem.begin(), stem.end(), del, -1);
        string layer_num_str = *it;
        int layer_num = std::stoi(layer_num_str);
        int channel_num = std::stoi(*(++it));
        if (layer_num > 0) break;

        cv::Mat img1;
        cv::imread(path.string(), img1);
        cv::cvtColor(img1, img1, cv::COLOR_BGR2RGB);
        img1.convertTo(img1, CV_32FC1);
        img1 /= 255;

        float offsetX = -img1.cols/2;
        float offsetY = -img1.rows/2;
        if (layer_num != 0 ) {
            if (channel_num == 0) z += 1.0;
            z += 1.0;
        }

        // Still image
        for (int y = 0; y < img1.rows; ++y)
        {
            for (int x = 0; x < img1.cols; ++x)
            {
                auto* kfData0 = &instanceData[flatIdx];
                auto* colorKf0 = &kfData0->colorKfs[0];
                float* colorKf0Val = colorKf0->value;
                auto px = img1.at<cv::Vec3f>(y, x);
                copy_n(px.val, 3, colorKf0Val);
                colorKf0Val[3] = 1.0;
                colorKf0->time = 0.0;
                kfData0->numColorKfs = 1;

                auto* posKf0 = &kfData0->positionKfs[0];
                float* posKf0Val = posKf0->value;
                posKf0Val[0] = (x + offsetX); posKf0Val[1] = - (y + offsetY); posKf0Val[2] = z;
                posKf0->time = 0.0;
                kfData0->numPositionKfs = 1;
                ++flatIdx;
            }
        }

        // Transition image
        if (!layerChanFiles.count(layer_num + 1)) {
            continue;
        }
        int prevFlatIdx = flatIdx;
        int chanCounter = 0;
        for (auto chanPath : layerChanFiles[layer_num + 1]) {
            flatIdx = prevFlatIdx;
            cv::Mat img2;
            cv::imread(chanPath, img2);
            cv::cvtColor(img2, img2, cv::COLOR_BGR2RGB);
            img2.convertTo(img2, CV_32FC1);
            img2 /= 255;

            float offset2X = -img2.cols/2;
            float offset2Y = -img2.rows/2;

            float timeOffset = chanCounter * 2.0;
            for (int y = 0; y < img1.rows; ++y)
            {
                for (int x = 0; x < img1.cols; ++x)
                {
                    int y2 = y/2;
                    int x2 = x/2;
                    float z2 = z + 1.;

                    // The pixel should fly separately to each channel in the next layer
                    auto* kfData = &instanceData[flatIdx];

                    // Color keyframes
                    // #0
                    auto* colorKf0 = &kfData->colorKfs[kfData->numColorKfs];
                    auto* colorKf0Val = colorKf0->value;
                    auto px = img1.at<cv::Vec3f>(y, x);
                    copy_n(px.val, 3, colorKf0Val);
                    colorKf0Val[3] = 1.0;
                    colorKf0->time = 0.0 + timeOffset;
                    colorKf0->easing = EasingType::IN_OUT_EXPO;
                    kfData->numColorKfs++;
                    // #1
                    auto* colorKf1 = &kfData->colorKfs[kfData->numColorKfs];
                    float* colorKf1Val = colorKf1->value;
                    auto px2 = img2.at<cv::Vec3f>(y2, x2);
                    copy_n(px2.val, 3, colorKf1Val);
                    colorKf1Val[3] = 1.0;
                    colorKf1->time = 1. - (x+y)/img1.total() + timeOffset;
                    kfData->numColorKfs++;

                    // Position keyframes
                    // #0
                    auto* posKf0 = &kfData->positionKfs[kfData->numPositionKfs];
                    auto* posKf0Val = posKf0->value;
                    posKf0Val[0] = (x + offsetX); posKf0Val[1] = - (y + offsetY); posKf0Val[2] = z;
                    posKf0->time = 0.0 + timeOffset;
                    posKf0->easing = EasingType::IN_OUT_EXPO;
                    kfData->numPositionKfs++;
                    // #1
                    auto* posKf1 = &kfData->positionKfs[kfData->numPositionKfs];
                    float* posKf1Val = posKf1->value;
                    posKf1Val[0] = x2+offset2X; posKf1Val[1] = -(y2+offset2Y); posKf1Val[2] = z2;
                    posKf1->time = 1. - ((float)(x+y))/(2*img1.cols) + timeOffset;
                    kfData->numPositionKfs++;

                    ++flatIdx;
                }
            }
            ++chanCounter;
        }
    }

    Cube cube(5, 1.0);

    // store instance data in an array buffer
    // --------------------------------------
    // Upload to SSBO
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, instanceData.size() * sizeof(InstanceData), (const void*)instanceData.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);

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
    auto startTime = chrono::steady_clock::now();
    float currentTime, prevTime;
    float fps = 30.;
    float maxTime = 3;
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
        glm::vec3 camPos{-50.f, 0.f, 150.f};
        glm::vec3 camLookAt{0.f, 0.f, 0.f};
        glm::vec3 camUp{0.f, 1.f, 0.f};
        glm::mat4 view = glm::lookAt(camPos, camLookAt, camUp);

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        prevTime = currentTime;
        // currentTime = (chrono::duration<double>(chrono::steady_clock::now() - startTime)).count();
        currentTime = frameCount / fps;
        shader.setFloat("currentTime", currentTime);

        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        glBindVertexArray(cubeVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, cube.getNumIndices(), numCubes);
        //glDrawArrays(GL_TRIANGLES, 0, cube.getNumIndices());
        glBindVertexArray(0);
        double t1Loop = (double)cv::getTickCount();
        double tLoop = (t1Loop - t0Loop) / cv::getTickFrequency();
        std::cout << "tLoop: " << tLoop << " s" << std::endl;

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

        if (saveFrame) {
            saveFrameBuffer(str(boost::format("frames/frame_%04d.png") % frameCount));
            cout << currentTime << endl;
            if (frameCount/fps >= maxTime) break;
        } else {
            break;
        }

        ++frameCount;
        glfwPollEvents();
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
