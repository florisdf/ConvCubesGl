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

const int MAX_KEYFRAMES = 2; // Fixed keyframe count per instance
struct Vec3Keyframe {
    float time;  // When this keyframe occurs
    glm::vec3 value;
};
struct Vec4Keyframe {
    float time;
    glm::vec4 value;
};
struct FloatKeyframe {
    float time;
    float value;
};
struct InstanceData {
    Vec3Keyframe positionKeyframes[MAX_KEYFRAMES];
    int numPositionKeyframes;

    Vec4Keyframe colorKeyframes[MAX_KEYFRAMES];
    int numColorKeyframes;

    FloatKeyframe spherenessKeyframes[MAX_KEYFRAMES];
    int numSpherenessKeyframes;

    FloatKeyframe scaleKeyframes[MAX_KEYFRAMES];
    int numScaleKeyframes;
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
    int pathCounter = 0;
    for (const auto& path : sorted_files) {
        cv::Mat img;
        cv::imread(path.string(), img);
        numCubes += img.rows * img.cols;
        if (pathCounter == 0) {
            // Generate transition pixels, as first layer has max num pixels
            // We need 9 transition pixels per pixel
            numCubes += img.rows * img.cols * 9;
        }
        sel_files.push_back(path);
        ++pathCounter;
        if (pathCounter == 2) break;
    }

    vector<InstanceData> instanceData(numCubes);

    std::regex del("_");
    float z = 0;
    int flat_idx = 0;
    cv::Mat img0 = cv::imread(sel_files[0].string());
    for (const auto& path : sel_files) {
        std::string stem = path.stem();
        // Create a regex_token_iterator to split the string
        std::sregex_token_iterator it(stem.begin(), stem.end(), del, -1);
        string layer_num_str = *it;
        int layer_num = std::stoi(layer_num_str);
        int channel_num = std::stoi(*(++it));

        cv::Mat img1;
        cv::imread(path.string(), img1);
        cv::cvtColor(img1, img1, cv::COLOR_BGR2RGB);
        img1.convertTo(img1, CV_32FC1);
        img1 /= 255;

        cv::Mat img2;
        cv::imread(path.parent_path() / "01_0000.jpg", img2);
        cv::cvtColor(img2, img2, cv::COLOR_BGR2RGB);
        img2.convertTo(img2, CV_32FC1);
        img2 /= 255;

        float offsetX = -img1.cols/2;
        float offsetY = -img1.rows/2;
        if (layer_num != 0 ) {
            if (channel_num == 0) z += 1.0;
            z += 1.0;
        }
        float offset2X = -img2.cols/2;
        float offset2Y = -img2.rows/2;
        for (int y = 0; y < img1.rows; ++y)
        {
            for (int x = 0; x < img1.cols; ++x)
            {
                auto* kfData = &instanceData[flat_idx];

                // Position keyframes
                // #0
                auto* posKf0 = &kfData->positionKeyframes[0];
                posKf0->value = {(x + offsetX), - (y + offsetY), z};
                posKf0->time = 0.;
                // #1
                auto* posKf1 = &kfData->positionKeyframes[0];
                int y2 = y/2;
                int x2 = x/2;
                float z2 = z+1;
                posKf1->value = {x2+offset2X, -(y2+offset2Y), z2};
                posKf1->time = 2.;
                kfData->numPositionKeyframes = 2;

                // Color keyframes
                // #0
                auto* colKf0 = &kfData->colorKeyframes[0];
                auto px = img1.at<cv::Vec3f>(y, x);
                colKf0->value = {px[0], px[1], px[2], 1.0};
                colKf0->time = 0.;
                // #1
                auto* colKf1 = &kfData->colorKeyframes[1];
                auto px2 = img2.at<cv::Vec3f>(y2, x2);
                colKf0->value = {px2[0], px2[1], px2[2], 1.0};
                colKf0->time = 2.;
                //
                kfData->numColorKeyframes = 2;

                // Sphereness keyframes
                // #0
                auto* sphereKf0 = &kfData->spherenessKeyframes[0];
                sphereKf0->value = 0.;
                sphereKf0->time = 0.;
                //
                kfData->numSpherenessKeyframes = 1;

                // Scale keyframes
                // #0
                auto* scaleKf0 = &kfData->scaleKeyframes[0];
                scaleKf0->value = 1.;
                scaleKf0->time = 0.;
                //
                kfData->numScaleKeyframes = 1;

                ++flat_idx;
            }
        }
    }

    Cube cube(5, 1.0);

    // store instance data in an array buffer
    // --------------------------------------
    // Upload to SSBO
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<glm::vec3> positions = cube.getPositions();
    std::vector<glm::vec3> normals = cube.getNormals();
    std::vector<uint32_t> indices = cube.getIndices();

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
        glfwSwapBuffers(window);

        if (saveFrame) {
            saveFrameBuffer("frame.png");
            saveFrame = false;
            break;
        }
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &vertexVBO);
    glDeleteBuffers(1, &posVBO);
    glDeleteBuffers(1, &colorVBO);
    glDeleteBuffers(1, &spherenessVBO);
    glDeleteBuffers(1, &scaleVBO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &framebuffer);

    delete [] translations;
    delete [] colors;
    delete [] spherenesses;
    delete [] scales;

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
