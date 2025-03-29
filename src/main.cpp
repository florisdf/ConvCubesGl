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

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

const unsigned int FB_HEIGHT = 3840*4;
const unsigned int FB_WIDTH = 2160*4;

Camera camera(glm::vec3(0.0f, 0.0f, 50.0f));

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
        sel_files.push_back(path);
        ++pathCounter;
    }

    glm::vec3* translations = new glm::vec3[numCubes];
    glm::vec4* colors = new glm::vec4[numCubes];
    float* spherenesses = new float[numCubes];
    float* scales = new float[numCubes];

    std::regex del("_");
    float z = 0;
    int flat_idx = 0;
    cv::Mat img0 = cv::imread(sel_files[0].string());
    double scale = 1./img0.cols;
    for (const auto& path : sel_files) {
        std::string stem = path.stem();
        // Create a regex_token_iterator to split the string
        std::sregex_token_iterator it(stem.begin(), stem.end(), del, -1);
        string layer_num_str = *it;
        int layer_num = std::stoi(layer_num_str);
        int channel_num = std::stoi(*(++it));

        cv::Mat img;
        cv::imread(path.string(), img);
        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
        img.convertTo(img, CV_32FC1);
        img /= 255;

        float offsetX = -img.cols/2;
        float offsetY = -img.rows/2;
        if (layer_num != 0 ) {
            if (channel_num == 0) z += scale;
            z += scale;
        }
        for (int y = 0; y < img.rows; ++y)
        {
            for (int x = 0; x < img.cols; ++x)
            {
                glm::vec3 translation;
                translation.x = (x + offsetX)*scale;
                translation.y = - (y + offsetY)*scale;
                translation.z = z;
                translations[flat_idx] = translation;
                auto px = img.at<cv::Vec3f>(y, x);
                glm::vec4 color{px[0], px[1], px[2], 1.0};
                colors[flat_idx] = color;
                spherenesses[flat_idx] = 1.0;
                scales[flat_idx] = 0.9*scale;
                ++flat_idx;
            }
        }
    }

    Cube cube(5, 1.0);

    // store instance data in an array buffer
    // --------------------------------------
    unsigned int cubeTfmVBO;
    glGenBuffers(1, &cubeTfmVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeTfmVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * numCubes, &translations[0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    unsigned int cubeColorVBO;
    glGenBuffers(1, &cubeColorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeColorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * numCubes, &colors[0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    unsigned int spherenessVBO;
    glGenBuffers(1, &spherenessVBO);
    glBindBuffer(GL_ARRAY_BUFFER, spherenessVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * numCubes, &spherenesses[0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    unsigned int scaleVBO;
    glGenBuffers(1, &scaleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, scaleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * numCubes, &scales[0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<glm::vec3> positions = cube.getPositions();
    std::vector<glm::vec3> normals = cube.getNormals();
    std::vector<uint32_t> indices = cube.getIndices();

    std::vector<float> dataVec = cube.getInterleavedData();
    float data[dataVec.size()];
    std::copy(dataVec.begin(), dataVec.end(), data);

    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    // fill vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    // position attribute
    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // Vertices
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float))); // Normals
    glEnableVertexAttribArray(1);

    // set instanced transforms
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, cubeTfmVBO); // this attribute comes from a different vertex buffer
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.
    // set instanced colors
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, cubeColorVBO); // this attribute comes from a different vertex buffer
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(3, 1); // tell OpenGL this is an instanced vertex attribute.
    // set instanced spherenesses
    glEnableVertexAttribArray(4);
    glBindBuffer(GL_ARRAY_BUFFER, spherenessVBO); // this attribute comes from a different vertex buffer
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(4, 1); // tell OpenGL this is an instanced vertex attribute.
    // set instanced scales
    glEnableVertexAttribArray(5);
    glBindBuffer(GL_ARRAY_BUFFER, scaleVBO); // this attribute comes from a different vertex buffer
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(5, 1); // tell OpenGL this is an instanced vertex attribute.

    // create high res framebuffer to write to; the final display output will be an anti-aliased downscaled version of this
    // --------------------------------------------------------------------------------------------------------------------
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // generate texture
    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FB_WIDTH, FB_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach it to currently bound framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);  

    // Create render buffer for depth and stencil buffers
    unsigned int rbo;
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
        glm::vec3 camPos{-1.f, 0.f, 2.f};
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
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeTfmVBO);
    glDeleteBuffers(1, &cubeColorVBO);
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
