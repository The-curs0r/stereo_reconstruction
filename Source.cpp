#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <glad.h>
#include <GLFW/glfw3.h>

#include <Windows.h>
#include <fstream>
#include "control.hpp"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using namespace std;

GLFWwindow* window;
const int SCR_WIDTH = 1080;
const int SCR_HEIGHT = 1080;

int w, h, comp;

//vector<unsigned char> left_image;
//  vector<unsigned char> right_image;

unsigned char* left_image;
unsigned char* right_image;

glm::mat3 cameraOne ;
glm::mat3 cameraTwo ;

glm::mat3 calibMatrixOne;
glm::mat3 calibMatrixTwo;

float focalLength = 5806.559;//Initialize
float prinPointX = 1429.219;//Initialize
float prinPointY = 993.403;//Initialize
float dOffset = 114.291;//Initialize
float baseline = 174.019;//Initialize

vector<glm::vec3> points;
GLuint vao;
GLuint vbo;

GLuint texture;

float minDepth = FLT_MAX;
float maxDepth = FLT_MIN;

float maxX=INT_MIN, maxY=INT_MIN;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int initialize() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Stereo Reconstruction", NULL, NULL);

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);//For Key Input
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);//For Curs0r Movement
    glfwPollEvents();//Continously Checks For Input
    glfwSetCursorPos(window, 1920 / 2, 1080 / 2);
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    return 1;
}

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

glm::vec3 intersect(Ray& rayCamOne, Ray& rayCamTwo) {
    float dx = rayCamTwo.origin.x - rayCamOne.origin.x;
    float dy = rayCamTwo.origin.y - rayCamOne.origin.y;
    float det = rayCamTwo.direction.x * rayCamOne.direction.y - rayCamTwo.direction.y * rayCamOne.direction.x;
    if (det == 0)
        return glm::vec3(0.0f);
    float u = (dy * rayCamTwo.direction.x - dx * rayCamTwo.direction.y) / det;
    float v = (dy * rayCamOne.direction.x - dx * rayCamOne.direction.y) / det;
    return glm::vec3(rayCamOne.origin+u*rayCamOne.direction);
}

void findPoints() {
    float** leftImage = new float * [h+8];
    for (int i = 0; i < h+8; i++)
        leftImage[i] = new float[w+8];

    float** rightImage = new float * [h+8];
    for (int i = 0; i < h+8; i++)
        rightImage[i] = new float[w+8];

    int** disp = new int* [h+8];
    for (int i = 0; i < h+8; i++)
        disp[i] = new int[w+8];

    for (int i = h - 1 + 8;i >= 0;i--)
    {
        for (int j = 0;j < w + 8;j++) {

            if (i <= 3 || j <= 3 || h - 1 + 8 - i <= 3 || w - 1 + 8 - j <= 3)
            {
                leftImage[i][j] = 0.0f;
                rightImage[i][j] = 0.0f;
                disp[i][j] = 0;
                continue;
            }
            float leftImgPixel = 0.2126 * (float)*left_image + 0.7152 * (float)*(left_image + 1) + 0.0722 * (float)*(left_image + 2);
            float rightImgPixel = 0.2126 * (float)*right_image + 0.7152 * (float)*(right_image + 1) + 0.0722 * (float)*(right_image + 2);
            leftImage[i][j] = leftImgPixel;
            rightImage[i][j] = rightImgPixel;
            disp[i][j] = 0;
            left_image += 3;
            right_image += 3;
            if (comp == 4)
            {
                left_image += 1;
                right_image += 1;
            }
        }
    }
    ofstream Output_Image("Output.ppm");
    if (Output_Image.is_open())
    {
        Output_Image << "P3\n" << w + 8 << " " << h + 8 << " 255\n";

        for (int i = 4;i < h + 8 - 4;i++) {
            for (int j = 4;j < w + 8 - 4;j++) {

                float arr[9][9] = { 0 };
                int tempX = 0, tempY = 0;
                for (int row = i - 4;row < i + 5;row++) {
                    for (int col = j - 4;col < j + 5;col++) {
                        arr[tempX][tempY] = leftImage[row][col];
                        tempY++;
                    }
                    tempX++;
                    tempY = 0;
                }

                float minSSD = FLT_MAX;
                int minSSDIndex = -1;

                for (int k = j - (int)dOffset;k < j;k++) {
                    if (k <= 5)
                        k = 5;

                    float arr2[9][9] = { 0 };
                    int tempX = 0, tempY = 0;
                    for (int row = i - 4;row < i + 5;row++) {
                        for (int col = k - 4;col < k + 5;col++) {
                            arr2[tempX][tempY] = rightImage[row][col];
                            tempY++;
                        }
                        tempX++;
                        tempY = 0;
                    }

                    float diff[9][9];
                    for (int i = 0;i < 9;i++) {
                        for (int j = 0;j < 9;j++) {
                            //diff[i][j] = abs(arr[i][j] - arr2[i][j]);
                            diff[i][j] = pow(arr[i][j] - arr2[i][j],2);
                        }
                    }
                    float sum = 0;
                    for (int i = 0;i < 9;i++) {
                        for (int j = 0;j < 9;j++) {
                            sum += diff[i][j];
                        }
                    }
                    if (sum < minSSD)
                    {
                        minSSD = sum;
                        minSSDIndex = k;
                    }
                }

                disp[i][j] = j - minSSDIndex;
                if (disp[i][j] < 0)
                    disp[i][j] = 0;
            }
        }
        int maxVal = INT_MIN;
        h += 8;
        w += 8;
        for (int i = h - 1;i >= 0;i--) {
            for (int j = 0;j < w;j++) {
                if (disp[i][j] > maxVal)
                    maxVal = disp[i][j];
            }
        }
        std::cout << maxVal;

        float** disp2 = new float* [h];
        for (int i = 0; i < h; i++)
            disp2[i] = new float[w];

        for (int i = h - 1;i >= 0;i--) {
            for (int j = 0;j < w;j++) {
                disp2[i][j] = (float)disp[i][j] / (float)maxVal;
            }
        }

        for (int i = h - 1;i >= 0;i--) {
            for (int j = 0;j < w;j++) {
                Output_Image << (int)(255 * (float)disp2[i][j]) << ' ' << (int)(255 * (float)disp2[i][j]) << ' ' << (int)(255 * (float)disp2[i][j]) << "\n";
            }
        }

        for (int i = 0; i < h; i++)
            delete[] leftImage[i];
        delete[] leftImage;
        for (int i = 0; i < h; i++)
            delete[] rightImage[i];
        delete[] rightImage;



        for (int i = 4; i < h-4; i++) {
            for (int j = 4;j < w-4;j++) {

                /*if (i <= 3 || j <= 3 || h - 1 - i <= 3 || w - 1 - j <= 3)
                {
                    continue;
                }*/

                float depth = baseline * focalLength / (disp[i][j] + dOffset);

                depth = (depth / 100.0 - 58.0) * 10.0f;

                if (depth < minDepth)
                    minDepth = depth;
                else if (depth > maxDepth) {
                    maxDepth = depth;
                }
                if (j/2.0 > maxX)
                    maxX = j/2.0 ;
                if (i/2.0  > maxY)
                    maxY = i/2.0 ;
                points.push_back(glm::vec3(j/2.0 ,i/2.0, depth));
            }
        }

        
        //stbi_image_free(left_image);
        //stbi_image_free(right_image);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        Output_Image.close();
        WinExec("cd ..", 1);
        WinExec("magick \"./Output.ppm\" \"./Output.png\"", 1);
    }
    return;
}

void loadImagesPNG() {
    string imageToLoad = "./Images/L_11.png";
    left_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb_alpha);
    if (left_image == nullptr) {
        std::cout << "Failed to load left image" << "\n";
        throw(string("Failed to load left image"));
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (comp == 3) {
        left_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, left_image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (comp == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, left_image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    string imageToLoad2 = "./Images/R_11.png";
    right_image = stbi_load(imageToLoad2.c_str(), &w, &h, &comp, STBI_rgb_alpha);
    if (right_image == nullptr) {
        std::cout << "Failed to load right image"<<"\n";
        throw(string("Failed to load right image"));
    }
    if (comp == 3) {
        right_image = stbi_load(imageToLoad2.c_str(), &w, &h, &comp, STBI_rgb);
    }
    std::cout << comp << "\n";

    return;
}

void loadImagesJPG() {
    string imageToLoad = "./Images/L_8.jpg";
    left_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb);
    if (left_image == nullptr) {
        throw(string("Failed to load left image"));
    }

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (comp == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w - 8, h - 8, 0, GL_RGB, GL_UNSIGNED_BYTE, left_image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (comp == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w - 8, h - 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, left_image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    imageToLoad = "./Images/R_1.jpg";
    right_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb);
    if (right_image == nullptr) {
        throw(string("Failed to load right image"));
    }
    return;
}

void genCameraMatrices() {
    cameraOne = glm::mat3(glm::vec3(focalLength, 0.0f, prinPointX), glm::vec3(0.0f, focalLength, prinPointY), glm::vec3(0.0f, 0.0f, 1.0f));
    cameraTwo = glm::mat3(glm::vec3(focalLength, 0.0f, prinPointX + dOffset), glm::vec3(0.0f, focalLength, prinPointY), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat3 temp = glm::mat3(glm::vec3(w,0.0f,0.0f), glm::vec3(0.0f,h,0.0f), glm::vec3(0.0f,0.0f,1.0f));
    calibMatrixOne = temp * cameraOne;
    calibMatrixTwo = temp * cameraTwo;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    
    if (initialize() < 0)
        return -1;

    loadImagesPNG();
    genCameraMatrices();
    findPoints();
    
    Shader shaderProgram("vShader.vertexShader.glsl", "fShader.fragmentShader.glsl");
    shaderProgram.use();

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        calcMatrixFromInp(window);
        glm::mat4 mv_matrix = getViewMatrix();
        glm::mat4 proj_matrix = getProjMatrix();
        shaderProgram.setMat4("mv_matrix", mv_matrix);
        shaderProgram.setMat4("proj_matrix", proj_matrix);
        shaderProgram.setFloat("minDepth", minDepth);
        shaderProgram.setFloat("maxDepth", maxDepth);
        shaderProgram.setFloat("maxX", maxX);
        shaderProgram.setFloat("maxY", maxY);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);   

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(points), &points[0], GL_STATIC_DRAW);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glDrawArrays(GL_POINTS, 0, points.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
	return 0;
}
