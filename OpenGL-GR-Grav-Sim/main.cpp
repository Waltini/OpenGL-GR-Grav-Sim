#define M_PI        3.14159265358979323846264338327950288   /* pi */

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL_timer.h>
#include "celestial_body_class.h"
#include "integration.h"
#include "shaders_c.h"

#include <iostream>
#include <cmath>
#include <fstream>

#include <condition_variable>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>

// Intialising main functions ~ Informs the compiler the functions exist pretty much
static void glfw_error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window);

int SCR_WIDTH = 600;
int SCR_HEIGHT = 800;

const unsigned int PixelsPerUnitLength = 150;

using ldvec3 = glm::tvec3<long double>;
using ldmat43 = glm::mat<4, 3, long double>;

std::atomic<bool> pause = false;
std::atomic<bool> halted = false;

std::mutex mtx;
std::condition_variable P_cv;
std::condition_variable H_cv;

struct clsState {
	celestial_body* b1;
	celestial_body* b2;
};

class buffer_box {
private:
	mathState backbuffer;
	clsState frontbuffer;

	void bufferSet(celestial_body& body1, celestial_body& body2) {
		// Constructor Function
		frontbuffer.b1 = &body1;
		frontbuffer.b2 = &body2;
		backbuffer.y[0] = body1.getPos();
		backbuffer.y[1] = body1.getVel();
		backbuffer.y[2] = body2.getPos();
		backbuffer.y[3] = body2.getVel();
		backbuffer.m1 = body1.getMass();
		backbuffer.m2 = body2.getMass();
		backbuffer.physics_time = 0.0L;
	}
public:
	buffer_box(celestial_body& body1, celestial_body& body2) {
		bufferSet(body1, body2);
	}

	void changeBuffers() {
		// Copies Backbuffer into Frontbuffer
		ldmat43 y = backbuffer.y;
		celestial_body& b1 = *frontbuffer.b1;
		celestial_body& b2 = *frontbuffer.b2;

		b1.setPos(y[0]);
		b1.setVel(y[1]);
		b2.setPos(y[2]);
		b2.setVel(y[3]);
	}

	mathState readBackBuffer() const { return backbuffer; } // returns the back buffer
	clsState readFrontBuffer() const { return frontbuffer; } // returns the back buffer

	void physicsStateUpdate(const ldmat43 state, const long double physics_time) {
		backbuffer.y = state;
		backbuffer.physics_time = physics_time;
	} // Updates the backbuffer with a new state and new time

	void applyEdits(ldmat43 dim_edits, long double m1_edit, long double m2_edit) { // Update the back buffer and front buffer with a completely new state / simulation
		//Back Buffer Edits
		backbuffer.y = dim_edits; // Apply dimensional edits to the backbuffer
		backbuffer.m1 = m1_edit; // Apply the mass edits of b1 to the backbuffer
		backbuffer.m2 = m2_edit; // Apply the mass edits of b2 to the backbuffer

		//Front Buffer Edits
		celestial_body& b1 = *frontbuffer.b1; // Dereference b1
		celestial_body& b2 = *frontbuffer.b2; // Dereference b2

		b1.setMass(m1_edit);
		b2.setMass(m2_edit);

		b1.setPos(dim_edits[0]);
		b1.setVel(dim_edits[1]);

		b2.setPos(dim_edits[2]);
		b2.setVel(dim_edits[3]);
	}

	// Debug Functions
	void debugBackBuffer() const {
		ldmat43 y = backbuffer.y;

		for (int i = 0; i < 4; i = i + 2) {
			std::cout << "bckbuf pos = ";
			for (int j = 0; j < 3; j++) {
				std::cout << std::setprecision(20) << y[i][j] << ", ";
			}
			std::cout << "\n";
			std::cout << "bckbuf vel = ";
			for (int j = 0; j < 3; j++) {
				std::cout << std::setprecision(20) << y[i + 1][j] << ", ";
			}
			std::cout << "\n";
		}
		std::cout << std::setprecision(20) << "bckbuf m1 = " << backbuffer.m1 << "\n";
		std::cout << std::setprecision(20) << "bckbuf m2 = " << backbuffer.m2 << "\n";
	}

	void debugFrontBuffer() const {
		celestial_body& b1 = *frontbuffer.b1;
		celestial_body& b2 = *frontbuffer.b2;

		b1.print();
		b2.print();
	}
};

int physics_thread(GLFWwindow* window, RK45_integration& integrator, buffer_box& bufbx, float sim_speed) {
	while (!glfwWindowShouldClose(window)) {
		halted = true;
		H_cv.notify_one();

		std::unique_lock<std::mutex> lock(mtx);
		P_cv.wait(lock, [] { return !pause; });
		lock.unlock();

		halted = false;

		mathState BackBuffer = bufbx.readBackBuffer();

		debug_values physics_step = integrator.step(BackBuffer);
		result_values physics_result = physics_step.result;

		if (integrator.getDebug()) {
			if (physics_result.accepted) {
				std::cout << "[ACCEPTED]" << std::endl;
				std::cout << "[attempt_h] " << physics_step.attempt_h << std::endl;
				bufbx.physicsStateUpdate(physics_result.state_y, physics_result.time_update);

				std::lock_guard<std::mutex> lock(mtx);
				bufbx.changeBuffers();
			}
		}
		else {
			if (physics_result.accepted) {
				std::cout << "[ACCEPTED]" << std::endl;
				std::cout << "[attempt_h] " << physics_step.attempt_h << std::endl;
				bufbx.physicsStateUpdate(physics_result.state_y, physics_result.time_update);

				std::lock_guard<std::mutex> lock(mtx);
				bufbx.changeBuffers();
			}
		}
		
	}
	return 0;
}

int rendering_thread(GLFWwindow* window, int FPS, glm::vec4 background, bool show, buffer_box& bufbx, const char* glsl_version) {
	// GLFW Set
	glfwMakeContextCurrent(window); // sets the context of the window to current on the thread

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls    

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Initialises GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialise GLAD" << std::endl;
		return -1;
	}

	Shader myShader("shader.vs", "shader.fs");

	float vertices[] = {
	-0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,

	-0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f, -0.5f,  0.5f,

	-0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,

	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,

	-0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f, -0.5f, -0.5f,

	-0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f, -0.5f
	};

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	while (!glfwWindowShouldClose(window))
	{
		// Input processing
		process_input(window);
		glClearColor(background.x, background.y, background.z, background.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Imgui Frame Start
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Test GUI
		if (show) {
			ImGui::Begin("Hello World!");
			if (ImGui::Button("Pause")) {
				pause = true;
				P_cv.notify_one();
			}
			if (ImGui::Button("Unpause")) {
				pause = false;
				P_cv.notify_one();
			}
			ImGui::End();
		}

		clsState snapshot = bufbx.readFrontBuffer(); // grabs the pointers for the celestial body class objects
		celestial_body b1 = *snapshot.b1; // De-referenced Body 1
		celestial_body b2 = *snapshot.b2; // De-referenced Body 2

		glBindVertexArray(VAO);

		// view matrix, representative of the current position of the where the point of view originates and in what direction
		glm::mat4 view = glm::mat4(1.0f);
		view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f)); // Sets the camera back three units

		// projection matrix, responsible for dictating what is in view / what can be seen
		glm::mat4 projection;
		projection = glm::perspective(glm::radians(45.0f), (float)SCR_HEIGHT / (float)SCR_WIDTH, 0.1f, 100.0f); // Grabs the current screen height and width to scale display accordingly. This is responsible for converting world coordinates into NDC

		myShader.use();
		// uniform assigning
		myShader.setMat4("view", view); // sets the calculated view matrix to the uniform variable view matrix called within the vertex shader
		myShader.setMat4("projection", projection); // sets the calculated projection matrix to the uniform variable projection matrix called within the vertex shader

		// Body 1
		glm::mat4 model_1 = glm::mat4(1.0f);
		b1.print();
		glm::vec3 pos1 = (glm::vec3)b1.getPos(); // grabs the current position of Body 1
		model_1 = glm::translate(model_1, pos1); // translates model matrix using the current position of Body 1
		myShader.setMat4("model", model_1); // sets the calculated model matrix for Body 1 to the model matrix called within the vertex shader

		glDrawArrays(GL_TRIANGLES, 0, 36);  // Updates the draw array with the current values for Body 1 set in the shader

		// Body 2
		glm::mat4 model_2 = glm::mat4(1.0f);
		glm::vec3 pos2 = (glm::vec3)b2.getPos(); // grabs the current position of Body 2
		model_2 = glm::translate(model_2, pos2); // translates model matrix using the current position of Body 2
		myShader.setMat4("model", model_2); // sets the calculated model matrix for Body 2 to the model matrix called within the vertex shader

		glDrawArrays(GL_TRIANGLES, 0, 36);  // Updates the draw array with the current values for Body 2 set in the shader

		// Imgui Render
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Buffers are swapped and events are checked and called
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// As soon as the window is set to close, the while loop is passed and then Imgui and glfw is terminated
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}


int main(int, char**)
{
	// Intialising GLFW
	if (!glfwInit())
	{
		return -1;
	}

	// Context
	const char* glsl_version = "#version 330 core";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Informs GLFW I'm using OpenGL Version 3.x
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Informs GLFW I'm using OpenGL Version x.3
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	// Window Creation
	GLFWwindow* window = glfwCreateWindow(SCR_HEIGHT, SCR_WIDTH, "Hello World!!!", NULL, NULL);
	if (!window)
	{
		std::cout << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// GLFW Set
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // sets the framebuffer size callback GLFW will use to the one I programmed
	glfwSetErrorCallback(glfw_error_callback); // sets the error callback GLFW will use to the one I programmed

	// Body 1 Characteristics
	ldvec3 pos1{ 0.0L, 0.0L, 0.0L };
	ldvec3 v1{ 0.0L, 0.0L, 0.0L };
	long double m1 = 1;

	// Body 2 Characteristics
	ldvec3 pos2{ 1.0L, 0.0L, 0.0L };
	ldvec3 v2{ 0.0L, 2.0L * M_PI, 0.0L };
	long double m2 = 1;

	celestial_body sun(pos1, v1, m1);
	celestial_body earth(pos2, v2, m2);

	RK45_integration integrator(1e-8, 1e-10, 0.05);

	bool show = true;
	glm::vec4 background(0.5f, 0.5f, 0.5f, 1.0f);

	buffer_box bufbx = buffer_box(sun, earth);

	bufbx.debugBackBuffer();
	bufbx.debugFrontBuffer();

	//physics_thread(window, integrator, bufbx);

	bufbx.debugBackBuffer();
	bufbx.debugFrontBuffer();

	int FPS = 60;

	//rendering_thread(window, FPS, background, show, bufbx);

	integrator.setDebug(true);

	// Threading
	std::thread p(physics_thread, window, std::ref(integrator), std::ref(bufbx), 1.0f);
	//std::thread r(rendering_thread, window, FPS, background, show, std::ref(bufbx), glsl_version);

	rendering_thread(window, FPS, background, show, bufbx, glsl_version);

	p.join();
	//r.join();

};



// Function definitions ~ kept below the main loop for formatting
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW ERROR %d: %s\n", error, description); // formatted print ~ using the format of a C error document it prints GLFW ERROR then its error number and then the description
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glfwGetWindowSize(window, &SCR_HEIGHT, &SCR_WIDTH);
	glViewport(0, 0, width, height); // Details how OpenGL should map its NDC (Normalised Device Coordinates) to the display
}

void process_input(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) // Terminate command
		glfwSetWindowShouldClose(window, true); // Causes while loop to break
}