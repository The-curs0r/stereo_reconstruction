#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <glad.h>
#include <GLFW/glfw3.h>
#include <string>

#include <Windows.h>
#include <fstream>
#include "control.hpp"
#include "Shader.h"
#include "loadImages.hpp"

#include <glm/gtc/matrix_transform.hpp>

using namespace std;

GLFWwindow* window;
const int SCR_WIDTH = 1080;
const int SCR_HEIGHT = 1080;

int w, h, comp;

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

float maxX=FLT_MIN, maxY=FLT_MIN;

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

void findPoints() {

    h += 8;
    w += 8;

    float** leftImage = new float * [h];
    for (int i = 0; i < h; i++)
        leftImage[i] = new float[w];

    float** rightImage = new float * [h];
    for (int i = 0; i < h; i++)
        rightImage[i] = new float[w];

    int** disp = new int* [h];
    for (int i = 0; i < h; i++)
        disp[i] = new int[w];


    //Find the intensity value for each pixel by converting RGB to grayscale.
    for (int i = h - 1 ;i >= 0;i--)
    {
        for (int j = 0;j < w ;j++) {

            if (i <= 3 || j <= 3 || h - 1 - i <= 3 || w - 1 - j <= 3)
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
    //Disparity map written to Output.ppm
    ofstream Output_Image("Output.ppm");
    if (Output_Image.is_open())
    {
        Output_Image << "P3\n" << w  << " " << h  << " 255\n";

        for (int i = 4;i < h  - 4;i++) {
            for (int j = 4;j < w  - 4;j++) {

                //Calculate window in left image
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

                    //For each k, calculate window in right image
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

                    //Find SSD
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

                    //Pick best of all SSD
                    if (sum < minSSD)
                    {
                        minSSD = sum;
                        minSSDIndex = k;
                    }
                }

                //Update disparity
                disp[i][j] = j - minSSDIndex;
                if (disp[i][j] < 0)
                    disp[i][j] = 0;
            }
        }

        //Divide by maximum disparity
        int maxVal = INT_MIN;
        for (int i = h - 1;i >= 0;i--) {
            for (int j = 0;j < w;j++) {
                if (disp[i][j] > maxVal)
                    maxVal = disp[i][j];
            }
        }
        //std::cout << maxVal;

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

        Output_Image.close();
        //WinExec("cd ..", 1);
        //WinExec("magick \"./Output.ppm\" \"./Output.png\"", 1);

        for (int i = 0; i < h; i++)
            delete[] leftImage[i];
        delete[] leftImage;
        for (int i = 0; i < h; i++)
            delete[] rightImage[i];
        delete[] rightImage;

        //Basic conversion to generate point cloud
        for (int i = 4; i < h-4; i++) {
            for (int j = 4;j <w-4 ;j++) {

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

        for (int i = 0; i < h; i++)
            delete[] disp[i];
        delete[] disp;

        //Generate vao and vbo
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        
    }
    return;
}

void findPointsDynamic() {

    float** leftImage = new float* [h];
    for (int i = 0; i < h; i++)
        leftImage[i] = new float[w];

    float** rightImage = new float* [h];
    for (int i = 0; i < h; i++)
        rightImage[i] = new float[w];

    int** disp = new int* [h];
    for (int i = 0; i < h; i++)
        disp[i] = new int[w];

    /*int** dispRight = new int* [h];
    for (int i = 0; i < h; i++)
        dispRight[i] = new int[w];*/
        
    //Find the intensity value for each pixel by converting RGB to grayscale.
    for (int i = h - 1;i >= 0;i--)
    {
        for (int j = 0;j < w;j++) {

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


    int OcculsionCost = 40;
    //For each row
    for (int row = 0;row < h; row++) {

        //Initilize cost and direction arrays
        int** cost = new int* [w];
        for (int i = 0; i < w; i++)
            cost[i] = new int[w];

        int** direction = new int* [w];
        for (int i = 0; i < w; i++)
            direction[i] = new int[w];

        for (int i = 1;i < w;i++) {
            cost[i][0] = i * OcculsionCost;
            cost[0][i] = i * OcculsionCost;
        }

        
        for (int i = 1; i < w; i++)
        {
            for (int j = 1; j < w; j++)
            {
                //Find minimum cost match
                int min1 = cost[i - 1][j - 1] + abs((int)leftImage[row][i] - (int)rightImage[row][j]);
                int min2 = cost[i - 1][j] + OcculsionCost;
                int min3 = cost[i][j-1] + OcculsionCost;
                int temp = min(min1, min2);
                int minimumCost = min(temp, min3);
                cost[i][j] = minimumCost;
                //Choose minimum cost so as to minimize number of horizontal and vertical discontinuities
                if (minimumCost == min1)
                    direction[i][j] = 1;
                if (minimumCost == min2)
                    direction[i][j] = 2;
                if (minimumCost == min3)
                    direction[i][j] = 3;
            }
        }
        
        //Reconstruct to get disparity map for left and right images
        int p = w - 1, q = w - 1;
        while (p != 0 && q != 0) {
            if (direction[p][q] == 1) {
                disp[row][p] = abs(p - q);
                //dispRight[row][q] = abs(p - q);
                p--;
                q--;
            }
            else if (direction[p][q] == 2) {
                p--;
            }
            else if (direction[p][q] == 3) {
                q--;
            }
        }
        for (int i = 0; i < w; i++)
            delete[] cost[i];
        delete[] cost;

        for (int i = 0; i < w; i++)
            delete[] direction[i];
        delete[] direction;
    }
    ofstream Output_Image("Output.ppm");
    if (Output_Image.is_open())
    {
        Output_Image << "P3\n" << w << " " << h << " 255\n";
        for (int i = h - 1;i >= 0;i--) {
            for (int j = 0;j < w;j++) {
                Output_Image << (disp[i][j]) << ' ' << (disp[i][j]) << ' ' << (disp[i][j]) << "\n";
            }
        }
        Output_Image.close();
        //WinExec("cd ..", 1);
        //WinExec("magick \"./Output.ppm\" \"./Output.png\"", 1);
    }

    //Basic conversion to generate point cloud
    for (int i = 0; i < h ; i++) {
        for (int j = w-1; j >=0 ;j--) {

            float depth = baseline * focalLength / (disp[i][j] + dOffset);

            depth = (depth / 100.0 - 58.0) * 10.0f;

            if (depth < minDepth)
                minDepth = depth;
            else if (depth > maxDepth) {
                maxDepth = depth;
            }
            if (j / 2.0 > maxX)
                maxX = j / 2.0;
            if (i / 2.0 > maxY)
                maxY = i / 2.0;
            points.push_back(glm::vec3(j / 2.0, i / 2.0, depth));
        }
    }

    for (int i = 0; i < h; i++)
        delete[] disp[i];
    delete[] disp;

    //stbi_image_free(left_image);
    //stbi_image_free(right_image);
    
    //Generate vao and vbo
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    return;
}

void genCameraMatrices() {
    //Implement non corrected images later
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
    
    
    std::cout << "Enter focal length of camera" << "\n";
    std::cin >> focalLength;
    std::cout << "Enter offset between cameras" << "\n";
    std::cin >> dOffset;
    std::cout << "Enter baseline" << "\n";
    std::cin >> baseline;

    string imgPathLeft, imgPathRight;
    std::cout << "Enter relative path of left image" << "\n";
    std::cin >> imgPathLeft;

    std::cout << "Enter relative path of right image" << "\n";
    std::cin >> imgPathRight;

    std::cout << "\n\n\n";

    if (initialize() < 0)
        return -1;

    loadImagesPNG(&left_image, &right_image, imgPathLeft, imgPathRight, texture, w, h, comp);

    //findPoints();
    findPointsDynamic();
    
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
