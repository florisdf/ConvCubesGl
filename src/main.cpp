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

    // build and compile shaders
    // -------------------------
    Shader shader("../shaders/vertex.shader", "../shaders/fragment.shader");
    //Shader shader("../shaders/basic_vertex.shader", "../shaders/fragment.shader");

    // generate a list of 100 cube locations/translation-vectors
    // ---------------------------------------------------------
    std::vector<std::vector<cv::Mat>> layer_imgs{};
    cv::Mat img;
    cv::imread("../assets/chihuahua_224.jpg", img);
    cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    img.convertTo(img, CV_32FC1);
    img /= 255;
    layer_imgs.push_back({img});

    set<fs::path> sorted_files;
    for (auto &entry : fs::directory_iterator("../scripts/layer_outputs"))
        sorted_files.insert(entry.path());
    std::regex del("_");

    for (const auto& path : sorted_files) {
        std::string stem = path.stem();
        // Create a regex_token_iterator to split the string
        std::sregex_token_iterator it(stem.begin(), stem.end(), del, -1);
        string layer_num_str = *it;
        int layer_num = std::stoi(layer_num_str);
        int channel_num = std::stoi(*(++it));
        int layer_idx = layer_num + 1;
        if (layer_idx == layer_imgs.size()) {
            layer_imgs.push_back({});
        }
        cv::Mat img;
        cv::imread(path.string(), img);
        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
        img.convertTo(img, CV_32FC1);
        img /= 255;
        layer_imgs[layer_idx].push_back(img);
    }

    Cube cube(3, 1.0);

    const int numCubes = img.rows * img.cols;
    glm::vec3 translations[numCubes];
    glm::vec4 colors[numCubes];
    float spherenesses[numCubes];
    float offset = -img.rows / 2;
    int index = 0;
    for (int y = 0; y < img.rows; ++y)
    {
        for (int x = 0; x < img.cols; ++x)
        {
            glm::vec3 translation;
            translation.x = (float)x + offset;
            translation.y = - ((float)y + offset);
            translation.z = 0.f;
            translations[index] = translation;
            auto px = img.at<cv::Vec3f>(y, x);
            glm::vec4 color{px[0], px[1], px[2], 1.0};
            colors[index] = color;
            spherenesses[index] = 1.0;
            ++index;
        }
    }

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

    // render loop
    // -----------
    bool saveFrame = true;
    while (!glfwWindowShouldClose(window))
    {
        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw instanced cubes
        shader.use();
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

        // camera/view transformation
        //glm::mat4 view = camera.GetViewMatrix();
        glm::vec3 camPos{-80.f, 0.f, 400.f};
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
    glDeleteBuffers(1, &cubeVBO);

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
