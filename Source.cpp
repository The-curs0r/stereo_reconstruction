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

float focalLength = 3997.684;//Initialize
float prinPointX = 1176.728;//Initialize
float prinPointY = 1011.728;//Initialize
float dOffset = 131.111;//Initialize
float baseline = 193.001;//Initialize

vector<glm::vec3> points;
GLuint vao;
GLuint vbo;

//glm::mat4 mv_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
//glm::mat4 proj_matrix = glm::ortho(0.0f, 435.0f, 0.0f, 300.0f,-50.0f, 50.0f);

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
    
    //glPointSize(2.0f);

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
    glm::vec3** leftImage = new glm::vec3*[h];
    for (int i = 0; i < h; i++)
        leftImage[i] = new glm::vec3[w];

    glm::vec3** rightImage = new glm::vec3 * [h];
    for (int i = 0; i < h; i++)
        rightImage[i] = new glm::vec3[w];

    int** disp = new int* [h];
    for (int i = 0; i < h; i++)
        disp[i] = new int[w];
    
    for (int i = h-1;i >=0;i--)
    {
        for (int j = 0;j < w;j++) {
            glm::vec3 leftImgPixel = glm::vec3((float)*left_image ,(float)*(left_image + 1) ,(float)*(left_image + 2));
            glm::vec3 rightImgPixel = glm::vec3((float)*right_image, (float)*(right_image + 1), (float)*(right_image + 2));
            leftImage[i][j] = leftImgPixel;
            rightImage[i][j] = rightImgPixel;
            left_image += 3;
            right_image += 3;
            if (comp == 4)
            {
                left_image += 1;
                right_image += 1;
            }
            //std::cout << leftImage[i][j][0] << " " << leftImage[i][j][1] << " " << leftImage[i][j][2] << "\n";
        }
        //std::cout << "\n";
    }

    const int dispAbs = 75,windowSizeX=9, windowSizeY = 9;
    int maxDisp = INT_MIN;
    ofstream Output_Image("Output.ppm");
    if (Output_Image.is_open())
    {
        Output_Image << "P3\n" << w << " " << h << " 255\n";
    }
    for (int i = 0;i < h;i++) {
        for (int j = 0;j < w;j++) {

            double disparities[2 * dispAbs - 1] = { FLT_MIN };

            for (int d = -dispAbs+1;d <= 0;d++) {
                //Calc window cost
                double windowCost = 0.0f;
                double flfr = 0.0f;
                double fl2 = 0.0f;
                double fr2 = 0.0f;
                for (int k = -windowSizeX;k <= windowSizeX;k++) {
                    for (int l = -windowSizeY;l <= windowSizeY;l++) {
                        if (j + k +d >= w || i + l >= h|| i + l<0||j+k<0 || j+k+d < 0 || j+k>=w)
                            continue;
                        double intensityAtPixelLeft = (0.2126 * leftImage[i + l][j + k][0] + 0.7152 * leftImage[i + l][j + k][1] + 0.0722 * leftImage[i + l][j + k][2]);
                        double intensityAtPixelRight = (0.2126 * rightImage[i + l ][j + k + d][0] + 0.7152 * rightImage[i + l ][j + k +d][1] + 0.0722 * rightImage[i + l ][j + k+d][2]);
                        windowCost += pow(intensityAtPixelLeft - intensityAtPixelRight, 2);
                        /*flfr += intensityAtPixelLeft * intensityAtPixelRight;
                        fl2 += intensityAtPixelLeft * intensityAtPixelLeft;
                        fr2 += intensityAtPixelRight * intensityAtPixelRight;*/
                    }
                }
                //windowCost = flfr / (sqrt(fl2*fr2));
                //std::cout << flfr<<" "<<fl2<<" "<< fr2 << " ";
                disparities[d - 1 + dispAbs] = windowCost;
            }
            //std::cout << "\n";

            int bestDisp=0;
            double bestWindowCost = disparities[dispAbs-1];
            for (int d = -dispAbs + 1;d < dispAbs;d++) {
                //std::cout << disparities[d - 1 + dispAbs] << " ";
                if (disparities[d - 1 + dispAbs] > bestWindowCost)
                {
                    bestDisp = -d;
                    bestWindowCost = disparities[d - 1 + dispAbs];
                }
            }
            disp[i][j] = bestDisp;
            if (disp[i][j] > maxDisp)
                maxDisp = disp[i][j];
            //if (disp[i][j])
            {
                //Output_Image << 255 - (int)(255 * (bestWindowCost)) << ' ' << 255 - (int)(255 * (bestWindowCost)) << ' ' << 255 - (int)(255 * (bestWindowCost)) << "\n";
                //Output_Image << 255-(int)(255 * (bestWindowCost*10-(int) (bestWindowCost * 10))) << ' ' << 255-(int)(255 * (bestWindowCost * 10 - (int)(bestWindowCost * 10))) << ' ' <<255- (int)(255 * (bestWindowCost * 10 - (int)(bestWindowCost * 10))) << "\n";
                //std::cout << bestWindowCost << "\n";
            }
        }
    }
    for (int i = h-1;i >=0 ;i--) {
        for (int j = 0;j < w;j++) {
            Output_Image << (int)(255*(float)disp[i][j]/(float)maxDisp) << ' ' << (int)(255 * (float)disp[i][j] / (float)maxDisp) << ' ' << (int)(255 * (float)disp[i][j] / (float)maxDisp) << "\n";

        }
    }

    for (int i = 0; i < h; i++)
        delete[] leftImage[i];
    delete[] leftImage;
    for (int i = 0; i < h; i++)
        delete[] rightImage[i];
    delete[] rightImage;

    for (int i = 0;i < h;i++) {
        for (int j = 0;j < w;j++) {
            Output_Image << "#" << disp[i][j]<<"\n";
            //Ray rayCamOne, rayCamTwo;
            //rayCamOne.origin = glm::vec3(cameraOne[0][2], cameraOne[1][2], focalLength);
            //rayCamTwo.origin = glm::vec3(cameraTwo[0][2], cameraTwo[1][2], focalLength);
            //rayCamOne.direction = glm::normalize(glm::vec3(j, i, focalLength) - glm::vec3(0.0f));
            //rayCamTwo.direction = glm::normalize(glm::vec3(j + disp[i][j], i, focalLength) - glm::vec3(0.0f));
            float depth = baseline * focalLength / (disp[i][j] + dOffset);
            //std::cout << disp[i][j] << "\n";
            points.push_back(glm::vec3(j/2.0, i/2.0, (depth/100.0-58.0)*10.0f));
        }
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    Output_Image.close();
    WinExec("cd ..", 1);
    WinExec("magick \"./Output.ppm\" \"./Output.png\"", 1);
    return ;
}

void loadImagesPNG() {

    string imageToLoad = "./Images/L_3.png";
    left_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb_alpha);
    if (left_image == nullptr) {
        throw(string("Failed to load snow texture"));
    }
    imageToLoad = "./Images/R_3.png";
    right_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb_alpha);
    if (right_image == nullptr) {
        throw(string("Failed to load snow texture"));
    }
    //std::cout << (float)*left_image << " " << (float)*(left_image+1) << " " << (float)*(left_image + 2) << " " << (float)*(left_image + 3)<<"\n";
    return;
}

void loadImagesJPG() {

    string imageToLoad = "./Images/R_1.jpg";
    left_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb);
    if (left_image == nullptr) {
        throw(string("Failed to load snow texture"));
    }
    imageToLoad = "./Images/L_1.jpg";
    right_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb);
    if (right_image == nullptr) {
        throw(string("Failed to load snow texture"));
    }
    //std::cout << (float)*left_image << " " << (float)*(left_image+1) << " " << (float)*(left_image + 2) << " " << (float)*(left_image + 3)<<"\n";
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
        
        /*glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);*/
        //glDrawArrays(GL_POINTS,0,points.size());

        

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);   

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(points), &points[0], GL_STATIC_DRAW);
        glDrawArrays(GL_POINTS, 0, points.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
	return 0;
}
