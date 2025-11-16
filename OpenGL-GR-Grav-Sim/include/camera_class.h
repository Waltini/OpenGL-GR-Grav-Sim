#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H
#define M_PI        3.14159265358979323846264338327950288   /* pi */

#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <iomanip>
#include <cmath>

enum directions {
	forward,
	backward,
	left,
	right
};

class camera{
private:
	bool intsFlag = true;
	float yaw;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
	float pitch;
	float lastX;
	float lastY;
	float fov = 45.0f;

	bool lookingFlag = false;

	// camera
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	const glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	// characteristics
	float cameraSpeed = 0.0f;
	float sensitivity = 0.1f;
	float speedScalar = 2.5f;

	inline void calcCamSpeed(double deltaTime) {
		cameraSpeed = speedScalar * deltaTime;
	}

public:
	// Constructor
	camera(float ya, float p, float lX, float lY, float fv)
		: yaw(ya), pitch(p), lastX(lX), lastY(lY), fov(fv) {
	}

	const inline void look(glm::mat4 &view) {
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	}

	const inline void project(glm::mat4& projection, float height, float width) {
		projection = glm::perspective(glm::radians(fov), height / width, 0.1f, 100.0f);  // Grabs the current screen height and width to scale display accordingly. This is responsible for converting world coordinates into NDC
	}

	inline void move(directions direction, const double deltaTime) {
		calcCamSpeed(deltaTime);
		switch (direction) {
		case forward: {
			cameraPos += cameraSpeed * cameraFront;
			break;
		}

		case backward: {
			cameraPos -= cameraSpeed * cameraFront;
			break;
		}

		case left: {
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
			break;
		}

		case right: {
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
			break;
		}

		default:
			break;
		}
	}

	inline void setLooking(const bool flag) { lookingFlag = flag; }

	inline void looking(const double xpos, const double ypos) {
		if (intsFlag)
		{
			lastX = xpos;
			lastY = ypos;
			intsFlag = false;
		}
		if (lookingFlag) {
			float xoffset = xpos - lastX;
			float yoffset = lastY - ypos;

			xoffset *= sensitivity;
			yoffset *= sensitivity;

			yaw += xoffset;
			pitch += yoffset;

			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;

			glm::vec3 direction;
			direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			direction.y = sin(glm::radians(pitch));
			direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(direction);
		}
		lastX = xpos;
		lastY = ypos;
	}

	inline void settings() {
		ImGui::SliderFloat("FOV", &fov, 15.0f, 70.0f, "%.1f");
		ImGui::SliderFloat("Sensitivity", &sensitivity, 0.01f, 1.0f, "%.2f");
		ImGui::SliderFloat("Camera Speed", &speedScalar, 0.5f, 2.5f, "%.2f");
	}

};

#endif