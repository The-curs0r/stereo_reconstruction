#include <GLFW/glfw3.h>
extern GLFWwindow* window;
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace std;

#include "control.hpp"

glm::mat4 viewMatrix;
glm::mat4 projMatrix;

glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

float horizontal = 3.14f;
float vertical = 0.0f;
float FoV = 45.0f;
float speed = 500.0;
float mouseSpeed = 0.005f;

glm::vec3 center;
glm::vec3 direction;
glm::vec3 up;

glm::mat4 getViewMatrix() {
	return viewMatrix;
}

glm::mat4 getProjMatrix() {
	return projMatrix;
}

glm::vec3 getFrontViewDirection() {
	return position + direction;
}

glm::vec3 getDirection() {
	return direction;
}

glm::vec3 getPosition() {
	return position;
}

void calcMatrixFromInp(GLFWwindow* window) {
	static double lastTime = glfwGetTime();
	double xpos, ypos;
	double current = glfwGetTime();

	float delta = (float)(current - lastTime);

	glfwGetCursorPos(window, &xpos, &ypos);
	glfwSetCursorPos(window, 1920 / 2, 1080 / 2);

	horizontal += mouseSpeed * float(1920 / 2 - xpos);
	vertical += mouseSpeed * float(1080 / 2 - ypos);

	direction = glm::normalize(glm::vec3(cos(vertical) * sin(horizontal), sin(vertical), cos(vertical) * cos(horizontal)));
	glm::vec3 right = glm::normalize(glm::vec3(sin(horizontal - 3.14f / 2.0f), 0, cos(horizontal - 3.14f / 2.0f)));
	up = glm::normalize(glm::cross(right, direction));

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		position += direction * delta * speed;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		position -= direction * delta * speed;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		position -= right * delta * speed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		position += right * delta * speed;
	}

	center = position + direction;
	viewMatrix = glm::lookAt(position, position + direction, up);
	lastTime = current;
	//projMatrix = glm::perspective(glm::radians(FoV), (float)(1920.0 / 1080.0), 0.1f, 10000.0f);
	projMatrix = glm::ortho(-300.0f,300.0f,-300.0f,300.0f,-300.0f,300.0f);
}