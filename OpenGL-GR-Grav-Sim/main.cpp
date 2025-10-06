#define M_PI        3.14159265358979323846264338327950288   /* pi */

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "formulae.h"
#include "celestial_body_class.h"
#include "integration.h"

#include <iostream>
#include <cmath>
#include <fstream>

#include <condition_variable>
#include <mutex>
#include <atomic>
#include <thread>

// Intialising main functions ~ Informs the compiler the functions exist pretty much
static void glfw_error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window);

const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 800;

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

void physics_thread(GLFWwindow* window, RK45_integration& integrator, buffer_box& bufbx) {
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
				bufbx.physicsStateUpdate(physics_result.state_y, physics_result.time_update);

				std::lock_guard<std::mutex> lock(mtx);
				bufbx.changeBuffers();
			}
		}
		else {
			if (physics_result.accepted) {
				std::cout << "[ACCEPTED]" << std::endl;
				bufbx.physicsStateUpdate(physics_result.state_y, physics_result.time_update);

				std::lock_guard<std::mutex> lock(mtx);
				bufbx.changeBuffers();
			}
		}

	}
}

int rendering_thread(GLFWwindow* window, int FPS, glm::vec4 background, bool show) {
	while (!glfwWindowShouldClose(window))
	{
		// Input processing
		process_input(window);
		glClearColor(background.x, background.y, background.z, background.w);
		glClear(GL_COLOR_BUFFER_BIT);

		// Imgui Frame Start
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Test GUI
		if (show) {
			ImGui::Begin("Hello World!");
			ImGui::Text("Tester Text");
			ImGui::End();
		}

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
	glfwMakeContextCurrent(window); // sets the context of the window to current on the thread
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // sets the framebuffer size callback GLFW will use to the one I programmed
	glfwSetErrorCallback(glfw_error_callback); // sets the error callback GLFW will use to the one I programmed

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls    

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Initialises GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialise GLAD" << std::endl;
		return -1;
	}

	ldvec3 pos1{0.0L, 0.0L, 0.0L};
	ldvec3 v1{0.0L, 0.0L, 0.0L};
	long double m1 = 1;

	ldvec3 pos2{ 0.8L, 0.0L, 0.0L };
	ldvec3 v2{ 0.0L, 1.5L * M_PI, 0.0L };
	long double m2 = 3.003e-6;


	celestial_body sun = celestial_body(pos1, v1, m1);
	celestial_body earth = celestial_body(pos2, v2, m2);

	RK45_integration integrator = RK45_integration(1e-8, 1e-10, 0.05);

	bool show = true;
	glm::vec4 background(0.5f, 0.5f, 0.5f, 1.0f);

	celestial_body b1 = celestial_body(pos1, v1, m1);
	celestial_body b2 = celestial_body(pos2, v2, m2);

	buffer_box bufbx = buffer_box(b1, b2);

	bufbx.debugBackBuffer();
	bufbx.debugFrontBuffer();

	//physics_thread(window, integrator, bufbx);

	bufbx.debugBackBuffer();
	bufbx.debugFrontBuffer();

	int FPS = 60;

	rendering_thread(window, FPS, background, show);

	// Threading
	std::thread p(physics_thread, window, integrator, bufbx);
	std::thread r(rendering_thread, window, FPS, background, show);
};

// Function definitions ~ kept below the main loop for formatting
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW ERROR %d: %s\n", error, description); // formatted print ~ using the format of a C error document it prints GLFW ERROR then its error number and then the description
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height); // Details how OpenGL should map its NDC (Normalised Device Coordinates) to the display
}

void process_input(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) // Terminate command
		glfwSetWindowShouldClose(window, true); // Causes while loop to break
}
