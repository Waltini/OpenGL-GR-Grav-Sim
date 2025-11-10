#define M_PI        3.14159265358979323846264338327950288   /* pi */

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glad/include/glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "integration.h"
#include "shaders_c.h"
#include "celestial_body_class.h"

#include <iostream>
#include <cmath>
#include <fstream>
#include <filesystem>

#include <queue>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>


int SCR_WIDTH = 600;
int SCR_HEIGHT = 800;

using dvec3 = glm::dvec3;
using dmat43 = glm::mat<4, 3, double>;

namespace fs = std::filesystem;

std::atomic<bool> pause = false;
std::atomic<bool> halted = false;

std::mutex mtx;
std::condition_variable P_cv;
std::condition_variable H_cv;

struct clsState {
	celestial_body* b1;
	celestial_body* b2;
};

struct render_object {
	dvec3 pos;
	dvec3 vel;
	double mass;
	int body_num;

	// Operations
	bool operator==(const render_object& other) const { // Equality Operator
		return pos == other.pos && vel == other.vel && mass == other.mass; // Compares each composite part ensuring all are similar
	}

	bool operator!=(const render_object& other) const { // Inequality Operator
		return !(*this == other); // Utilises the equality operator
	}
};

// Intialising main functions ~ Informs the compiler the functions exist pretty much
static void glfw_error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window, render_object& b1, render_object& b2);
void mouse_callback(GLFWwindow* window, int button, int action, int mods);

// Formatting Functions
enum infields {
	pos,
	vel,
	mass
};

void ImGui_Input_Fields(infields intp_type, render_object& edit_obj);

class buffer_box {
private:
	mathState backBuffer; // The back buffer. Used for the background mathematics and the buffer used by the physics thread
	clsState frontBuffer; // The front buffer. Used for displaying the position of the bodies and the buffer used by the rendering function within the main thread
	glm::vec2 mousePos; // The mouse buffer. Stores the location of the most recent mouse input
	int GUI_ID; // The GUI ID. Informs the rendering what gui (in context of the bodies) to display at a given moment

	void bufferSet(celestial_body& body1, celestial_body& body2) {
		// Constructor Function
		frontBuffer.b1 = &body1;
		frontBuffer.b2 = &body2;
		backBuffer.y[0] = body1.getPos();
		backBuffer.y[1] = body1.getVel();
		backBuffer.y[2] = body2.getPos();
		backBuffer.y[3] = body2.getVel();
		backBuffer.m1 = body1.getMass();
		backBuffer.m2 = body2.getMass();
		backBuffer.physics_time = 0.0;
	}
public:
	buffer_box(celestial_body& body1, celestial_body& body2) {
		bufferSet(body1, body2);
	}

	void changeBuffers() {
		// Copies Backbuffer into Frontbuffer
		dmat43 y = backBuffer.y;
		celestial_body& b1 = *frontBuffer.b1;
		celestial_body& b2 = *frontBuffer.b2;

		b1.setPos(y[0]);
		b1.setVel(y[1]);
		b2.setPos(y[2]);
		b2.setVel(y[3]);
	}

	mathState readBackBuffer() const { return backBuffer; } // returns the back buffer
	clsState readFrontBuffer() const { return frontBuffer; } // returns the back buffer

	void physicsStateUpdate(const dmat43 state) {
		backBuffer.y = state;
	} // Updates the backbuffer with a new state and new time

	void applyEdits(dvec3 pos1_edit, dvec3 vel1_edit, dvec3 pos2_edit, dvec3 vel2_edit, double m1_edit, double m2_edit) { // Update the back buffer and front buffer with a completely new state / simulation
		// Pack Matrix
		dmat43 dim_edits = { pos1_edit, vel1_edit, pos2_edit, vel2_edit };
		
		//Back Buffer Edits
		backBuffer.y = dim_edits; // Apply dimensional edits to the backbuffer
		backBuffer.m1 = m1_edit; // Apply the mass edits of b1 to the backbuffer
		backBuffer.m2 = m2_edit; // Apply the mass edits of b2 to the backbuffer

		//Front Buffer Edits
		celestial_body& b1 = *frontBuffer.b1; // Dereference b1
		celestial_body& b2 = *frontBuffer.b2; // Dereference b2

		b1.setMass(m1_edit);
		b2.setMass(m2_edit);

		b1.setPos(pos1_edit);
		b1.setVel(vel1_edit);

		b2.setPos(pos2_edit);
		b2.setVel(vel2_edit);
	}

	// Debug Functions
	void debugBackBuffer() const {
		dmat43 y = backBuffer.y;

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
		std::cout << std::setprecision(20) << "bckbuf m1 = " << backBuffer.m1 << "\n";
		std::cout << std::setprecision(20) << "bckbuf m2 = " << backBuffer.m2 << "\n";
	}

	void debugFrontBuffer() const {
		celestial_body& b1 = *frontBuffer.b1;
		celestial_body& b2 = *frontBuffer.b2;

		b1.print();
		b2.print();
	}

	glm::vec2 getMousePos() const { return mousePos; }
	void setMousePos(const glm::vec2 position) { mousePos = position; }
};

// Initial State
// Body 1 Characteristics
dvec3 pos1{ 0.0, 0.0, 0.0 };
dvec3 v1{ 0.0, 0.0, 0.0 };
double m1 = 1;

// Body 2 Characteristics
dvec3 pos2{ 1.0, 0.0, 0.0 };
dvec3 v2{ 0.0, 2.0 * M_PI, 0.0 };
double m2 = 3.003e-6;

// Creates the celestial body objects
celestial_body body1(pos1, v1, m1);
celestial_body body2(pos2, v2, m2);

buffer_box bufbx = buffer_box(body1, body2);

void physics_thread(GLFWwindow* window, RK45_integration& integrator, double sim_speed) {
	double ct, lt = 0.0, accum_t = 0.0;
	double physics_dt = 0.033;
	dmat43 mat{ dvec3{ 0.0 }, dvec3{ 0.0 }, dvec3{ 0.0 }, dvec3{ 0.0 } };
	integrate_result result(mat, 0.0, 0, 0, 0, 0.0);

	int count = 0, accepts = 0, rejects = 0;

	while (!glfwWindowShouldClose(window)) {
		halted = true; // Momentarily updates the halted variable so it may be caught if the ImGUI pause function has started
		H_cv.notify_one(); // Notifies the rendering function IF IT IS WAITING, if it's not waitng nothing happens and this just goes onto the next step

		double lock_time_start = glfwGetTime();

		std::unique_lock<std::mutex> lock_unique(mtx);
		P_cv.wait(lock_unique, [] { return !pause; }); // Mutex unqiue_lock checks if the physics thread has been set to pause
		lock_unique.unlock(); // Unlocks unique_lock

		double lock_time_end = glfwGetTime();
		double lock_duration = lock_time_end - lock_time_start;

		halted = false; // After checking that it doesn't need to halted it then sets the halted variable to false

		mathState BackBuffer = bufbx.readBackBuffer(); // Reads the current backbuffer

		ct = glfwGetTime();
		double delta = ct - lt - lock_duration; // change in time since last
		lt = ct;
		accum_t += delta;

		while (accum_t >= physics_dt) {
			accum_t -= physics_dt;
			result = integrator.step(BackBuffer, physics_dt); // Steps through the physics given the current state within the backbuffer

			bufbx.physicsStateUpdate(result.state_y);

			std::lock_guard<std::mutex> lock(mtx);
			bufbx.changeBuffers();
		}

		count += result.count;
		accepts += result.accepts;
		rejects += result.rejects;

		//std::cout << "accept ratio = " << accepts / count << std::endl;
		//std::cout << "reject ratio = " << rejects / count << std::endl;

		std::this_thread::sleep_for(std::chrono::milliseconds(33));
	}
}

void render(GLFWwindow* window, int FPS, glm::vec4 background, bool show, const char* glsl_version) {
	int GUI_ID = 0;

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
	}

	fs::path vertexPath = fs::path("assets") / "shader.vs";
	fs::path fragmentPath = fs::path("assets") / "shader.fs";
	Shader myShader(vertexPath.string().c_str(), fragmentPath.string().c_str()); // Points my shader class to my vertex and fragment shader files

	float vertices[] = { // Creates a cube
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

	// Graphical Buffers and Arrays
	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Render Objects
	render_object body1, body2, body1_edit, body2_edit;
	body1.body_num = 1; body1_edit.body_num = 1;
	body2.body_num = 2; body2_edit.body_num = 2;

	bool edit_f = false; // editing flag to indicate if the user is editing properties

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
	glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

	// view matrix, representative of the current position of the where the point of view originates and in what direction
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f)); // Sets the camera back three units

	while (!glfwWindowShouldClose(window))
	{
		// Front Buffer Snapshot
		clsState snapshot = bufbx.readFrontBuffer(); // grabs the pointers for the celestial body class objects
		celestial_body b1 = *snapshot.b1; // De-referenced Body 1
		celestial_body b2 = *snapshot.b2; // De-referenced Body 2

		// Snapshot segmenting
		body1.pos = b1.getPos();
		body1.vel = b1.getVel();
		body1.mass = b1.getMass();
		body2.pos = b2.getPos();
		body2.vel = b2.getVel();
		body2.mass = b2.getMass();
		if (!edit_f) {
			body1_edit.pos = body1.pos;
			body1_edit.vel = body1.vel;
			body1_edit.mass = body1.mass;
			body2_edit.pos = body2.pos;
			body2_edit.vel = body2.vel;
			body2_edit.mass = body2.mass;
		}

		// Input processing
		process_input(window, body1, body2);
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
				if (edit_f) {
					edit_f = false;
				}
				P_cv.notify_one();
			}
			ImGui_Input_Fields(pos, body1_edit);
			ImGui_Input_Fields(vel, body1_edit);
			ImGui_Input_Fields(mass, body1_edit);
			ImGui_Input_Fields(pos, body2_edit);
			ImGui_Input_Fields(vel, body2_edit);
			ImGui_Input_Fields(mass, body2_edit);
			if (body1_edit != body1 || body2_edit != body2) {
				edit_f = true;
			}

			if (edit_f) {
				pause = true;
				P_cv.notify_one();

				bufbx.applyEdits(body1_edit.pos, body1_edit.vel, body2_edit.pos, body2_edit.vel, body1_edit.mass, body2_edit.mass);
			}

			ImGui::End();
		}

		glBindVertexArray(VAO);

		// projection matrix, responsible for dictating what is in view / what can be seen
		glm::mat4 projection;
		projection = glm::perspective(glm::radians(45.0f), (float)SCR_HEIGHT / (float)SCR_WIDTH, 0.1f, 100.0f); // Grabs the current screen height and width to scale display accordingly. This is responsible for converting world coordinates into NDC

		myShader.use();
		// uniform assigning
		myShader.setMat4("view", view); // sets the calculated view matrix to the uniform variable view matrix called within the vertex shader
		myShader.setMat4("projection", projection); // sets the calculated projection matrix to the uniform variable projection matrix called within the vertex shader

		// Body 1
		glm::mat4 model_1 = glm::mat4(1.0f);
		glm::vec3 pos1 = (glm::vec3)body1.pos; // grabs the current position of Body 1
		model_1 = glm::translate(model_1, pos1); // translates model matrix using the current position of Body 1
		myShader.setMat4("model", model_1); // sets the calculated model matrix for Body 1 to the model matrix called within the vertex shader

		glDrawArrays(GL_TRIANGLES, 0, 36);  // Updates the draw array with the current values for Body 1 set in the shader

		// Body 2
		glm::mat4 model_2 = glm::mat4(1.0f);
		glm::vec3 pos2 = (glm::vec3)body2.pos; // grabs the current position of Body 2
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

	RK45_integration integrator(1e-8, 1e-10, 0.05);

	bool show = true;
	glm::vec4 background(0.5f, 0.5f, 0.5f, 1.0f);

	int FPS = 60;

	integrator.setDebug(false);

	// Threading
	std::thread p(physics_thread, window, std::ref(integrator), 1.0f);

	render(window, FPS, background, show, glsl_version);

	p.join();

	// As soon as the window is set to close, the while loop is passed and then Imgui and glfw is terminated
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
};



// Function definitions ~ kept below the main loop for formatting
static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW ERROR %d: %s\n", error, description); // formatted print ~ using the format of a C error document it prints GLFW ERROR then its error number and then the description
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glfwGetWindowSize(window, &SCR_HEIGHT, &SCR_WIDTH);
	glViewport(0, 0, width, height); // Details how OpenGL should map its NDC (Normalised Device Coordinates) to the display
}

void process_input(GLFWwindow* window, render_object& b1, render_object& b2) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) // Terminate command
		glfwSetWindowShouldClose(window, true); // Causes while loop to break
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
		double xpos, ypos;

		glfwGetCursorPos(window, &xpos, &ypos); // grabs the position of cursour

		bufbx.setMousePos((glm::vec2)(xpos, ypos));
	}
}

void ImGui_Input_Fields(infields intp_type, render_object& edit_obj) {
	ImGui::PushItemWidth(150);

	switch (intp_type) {
	case pos:
	case vel: {
		glm::dvec3* vec = (intp_type == pos) ? &edit_obj.pos : &edit_obj.vel; // selects which vector to use
		const char* label_name = (intp_type == pos) ? "Position" : "Velocity"; // selects the display label for the vector input fields

		char label_x[64], label_y[64], label_z[64];
		snprintf(label_x, sizeof(label_x), "##b%i_%s_x", edit_obj.body_num, label_name); // reformats ID label_x to contain the relevant information
		snprintf(label_y, sizeof(label_y), "##b%i_%s_y", edit_obj.body_num, label_name); // reformats ID label_y to contain the relevant information
		snprintf(label_z, sizeof(label_z), "##b%i_%s_z", edit_obj.body_num, label_name); // reformats ID label_z to contain the relevant information

		ImGui::Text("%s", label_name); // display label
		ImGui::SameLine(); ImGui::InputDouble(label_x, &vec->x, 0.01, 1.0, "%e"); // input field for the x component of the vector
		ImGui::SameLine(); ImGui::InputDouble(label_y, &vec->y, 0.01, 1.0, "%e"); // input field for the y component of the vector
		ImGui::SameLine(); ImGui::InputDouble(label_z, &vec->z, 0.01, 1.0, "%e"); // input field for the z component of the vector
		break;
	}

	case mass: {
		char label_m[64];
		snprintf(label_m, sizeof(label_m), "##b%i_mass", edit_obj.body_num); // reformats the ID label mass to contain the relevant information
		ImGui::Text("Mass");
		double last_mass = edit_obj.mass;
		ImGui::SameLine(); ImGui::InputDouble(label_m, &edit_obj.mass, 0.01, 1.0, "%e"); // input field for the mass
		if (edit_obj.mass <= 0) {
			edit_obj.mass = last_mass;
		}
		break;
	}

	default:
		break;
	}

	ImGui::PopItemWidth();
}