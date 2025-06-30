#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <limits>
#include <vector>
#include <chrono>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum VAO_ID {PointsVAO, LineVAO, NumVAOs};
enum Buffer_ID {PointPosBuffer, PointColorBuffer, LineBuffer, NumBuffers};
enum ShaderProgram_ID {PointProgram, LineProgram, NumPrograms};
enum Attrib_ID {PositionAttrib = 0, ColorAttrib = 1};

#define BUFFER_OFFSET(bytes) ((GLubyte*)NULL (bytes))

#define NUM_POINTS 3000

GLuint CreateShader(GLenum shadertype, std::string filename);
void SortPointsByPolarAngle(const GLfloat pointPos[][2], GLuint* hullVertIndices, GLuint numPoints);

int main() {
	auto seed = time(0);
	srand(seed);
	std::cout << "Seed: " << seed << std::endl;

	// Initialize GLFW.
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "ConvexHull", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	// GLAD: Load all OpenGL function pointers.
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	
	// Initialize positions and color for points.
	GLfloat pointPos[NUM_POINTS][2];
	GLuint hullVertIndices[NUM_POINTS];
	GLuint numHullVerts = 0;
	GLfloat pointColor[NUM_POINTS][4];

	for (int i = 0; i < NUM_POINTS; ++i) {
		for (int j = 0; j < 2; ++j) {
			// Screen space is from -1 to 1, so X and Y are set randomly between -0.975 and 0.975 to leave rendering space.
			pointPos[i][j] = ((float) rand() / RAND_MAX) * 1.95 - 0.975;
		}

		// Points default as white.
		pointColor[i][0] = 1.0f;
		pointColor[i][1] = 1.0f;
		pointColor[i][2] = 1.0f;
		pointColor[i][3] = 1.0f;

		hullVertIndices[i] = i;
	}

	// Graham's algorithm for convex hull.
	{
		auto graham_start = std::chrono::high_resolution_clock::now();

		// Step 1: Find a starting point (lowest y coordinate, and if multiple greatest x coordinate)
		GLuint start_index = 0;
		float lowest_y = pointPos[0][1];
		for (int i = 1; i < NUM_POINTS; ++i) {
			
			if (pointPos[i][1] > lowest_y) {
				continue;
			} else if (pointPos[i][1] < lowest_y) {
				lowest_y = pointPos[i][1];
				start_index = i;
			} else if (pointPos[i][1] == lowest_y && pointPos[i][0] > pointPos[start_index][0]) {
				start_index = i;
			}
		}

		hullVertIndices[0] = start_index;

		// Swapping 0 for start_index in place, so we can reuse this for selection sort later.
		hullVertIndices[start_index] = 0;

		// Step 2: Given the list of points, sort them by their polar angle relative to the starting point.
		auto sort_start = std::chrono::high_resolution_clock::now();
		SortPointsByPolarAngle(pointPos, hullVertIndices, NUM_POINTS);
		auto sort_end = std::chrono::high_resolution_clock::now();

		std::cout << "Duration to sort: " << std::chrono::duration_cast<std::chrono::microseconds>(sort_end - sort_start).count() << "us" << std::endl;

		// Step 3: Remove concave points, based on only allowing positive cross products.
		// 
		std::vector<GLuint> convex_hull_vec;

		// We assume the base point and the first non-base point are in the convex hull, at least until later.
		convex_hull_vec.push_back(hullVertIndices[0]);
		convex_hull_vec.push_back(hullVertIndices[1]);

		// Positive cross product -> only left turns.
		for (int i = 2; i < NUM_POINTS; ++i) {
			GLuint p = convex_hull_vec[convex_hull_vec.size() - 2];
			GLuint q = convex_hull_vec[convex_hull_vec.size() - 1];
			GLuint r = hullVertIndices[i];
			
			float cross_product =	   pointPos[p][0] * pointPos[q][1] - pointPos[p][1] * pointPos[q][0] 
									- (pointPos[p][0] * pointPos[r][1] - pointPos[r][0] * pointPos[p][1]) 
									+  pointPos[q][0] * pointPos[r][1] - pointPos[r][0] * pointPos[q][1];

			if (cross_product < 0.0f) {
				// Repeat this iteration, without the middle value that created a concave arc.
				convex_hull_vec.pop_back();
				if (convex_hull_vec.size() == 1) {
					// If the first non-base point was the bad value (not expected mathematically, but potentially possible with floating point error or the == case), 
					// we should push back "r" to replace it, so we always have at least 2 points in convex_hull_vec.
					convex_hull_vec.push_back(r);
				} else {
					--i;
				}
				continue;
			} else {
				convex_hull_vec.push_back(r);
			}
		}

		//// Write value from vector back into array.
		numHullVerts = convex_hull_vec.size();

		for (size_t i = 0; i < numHullVerts; ++i) {
			hullVertIndices[i] = convex_hull_vec[i];
		}

		auto graham_end = std::chrono::high_resolution_clock::now();
		std::cout << "Duration to calculate convex hull: " << std::chrono::duration_cast<std::chrono::microseconds>(graham_end - graham_start).count() << "us" << std::endl;
	}

	// Make all points in the convex hull green.
	for (int i = 0; i < numHullVerts; ++i) {
		auto index = hullVertIndices[i];

		pointColor[index][0] = 0.0f;
		pointColor[index][2] = 0.0f;
	}

	// Color the furthest apart points (calculated from the convex hull subset) red.
	{
		auto furthest_start = std::chrono::high_resolution_clock::now();

		GLuint furthestStart = 0;
		GLuint furthestEnd = 1;
		float max_dst = 0.0f;

		for (int i = 0; i < numHullVerts; ++i) {
			for (int j = i; j < numHullVerts; ++j) {
				float delta_y = pointPos[hullVertIndices[i]][1] - pointPos[hullVertIndices[j]][1];
				float delta_x = pointPos[hullVertIndices[i]][0] - pointPos[hullVertIndices[j]][0];
				float dst = sqrt(delta_x * delta_x + delta_y * delta_y);

				if (dst > max_dst) {
					max_dst = dst;
					furthestStart = i;
					furthestEnd = j;
				}
			}
		}

		auto furthest_end = std::chrono::high_resolution_clock::now();
		std::cout << "Duration to find the furthest points: " << std::chrono::duration_cast<std::chrono::microseconds>(furthest_end - furthest_start).count() << "us" << std::endl;

		pointColor[hullVertIndices[furthestStart]][0] = 1.0f;
		pointColor[hullVertIndices[furthestStart]][1] = 0.0f;
		pointColor[hullVertIndices[furthestEnd]][0] = 1.0f;
		pointColor[hullVertIndices[furthestEnd]][1] = 0.0f;
	}

	// Shader program creation.
	GLuint ShaderPrograms[NumPrograms];
	ShaderPrograms[PointProgram] = glCreateProgram();
	ShaderPrograms[LineProgram] = glCreateProgram();

	GLuint pointVertShader = CreateShader(GL_VERTEX_SHADER, "point_vert.shader");
	GLuint pointFragShader = CreateShader(GL_FRAGMENT_SHADER, "point_frag.shader");
	glAttachShader(ShaderPrograms[PointProgram], pointVertShader);
	glAttachShader(ShaderPrograms[PointProgram], pointFragShader);

	GLuint lineVertShader = CreateShader(GL_VERTEX_SHADER, "passthrough_vert.shader");
	GLuint lineFragShader = CreateShader(GL_FRAGMENT_SHADER, "line_frag.shader");
	glAttachShader(ShaderPrograms[LineProgram], lineVertShader);
	glAttachShader(ShaderPrograms[LineProgram], lineFragShader);

	for (GLuint program_idx = 0; program_idx < NumPrograms; ++program_idx) {
		glLinkProgram(ShaderPrograms[program_idx]);
		{
			int success;
			char infoLog[512];
			glGetProgramiv(ShaderPrograms[program_idx], GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(ShaderPrograms[program_idx], 512, NULL, infoLog);
				std::cout << "Error: Program link failed\n" << infoLog << std::endl;
			}
		}

		glValidateProgram(ShaderPrograms[program_idx]);
		{
			int success;
			char infoLog[512];
			glGetProgramiv(ShaderPrograms[program_idx], GL_VALIDATE_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(ShaderPrograms[program_idx], 512, NULL, infoLog);
				std::cout << "Error: Program validate failed\n" << infoLog << std::endl;
			}
		}
	}

	glDeleteShader(pointVertShader);
	glDeleteShader(pointFragShader);
	glDeleteShader(lineVertShader);
	glDeleteShader(lineFragShader);

	// VAO, VBO, EBO.
	GLuint VAOs[NumVAOs];
	GLuint Buffers[NumBuffers];
	glGenVertexArrays(NumVAOs, VAOs);
	glGenBuffers(NumBuffers, Buffers);

	// Points VAO.
	glBindVertexArray(VAOs[PointsVAO]);

	glBindBuffer(GL_ARRAY_BUFFER, Buffers[PointPosBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * NUM_POINTS * 2, pointPos, GL_STATIC_DRAW);
	glVertexAttribPointer(PositionAttrib, 2, GL_FLOAT, false, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(PositionAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, Buffers[PointColorBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * NUM_POINTS * 4, pointColor, GL_STATIC_DRAW);
	glVertexAttribPointer(ColorAttrib, 4, GL_FLOAT, false, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(ColorAttrib);

	// Line VAO.
	glBindVertexArray(VAOs[LineVAO]);

	glBindBuffer(GL_ARRAY_BUFFER, Buffers[PointPosBuffer]);
	// Point Pos was already loaded into the buffer earlier; no need to redo here.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[LineBuffer]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * NUM_POINTS, hullVertIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(PositionAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(PositionAttrib);

	// Render options.
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_LINE_WIDTH);

	// Render loop.
	while (!glfwWindowShouldClose(window)) {
		// Quit if desired.
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		int width, height;
		glfwGetWindowSize(window, &width, &height);
		glPointSize((width / 800) * 2);
		glLineWidth((width / 800) * 1);
		glViewport(0, 0, width, height);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw convex hull.
		glUseProgram(ShaderPrograms[LineProgram]);
		glBindVertexArray(VAOs[LineVAO]);
		glDrawElements(GL_LINE_LOOP, numHullVerts, GL_UNSIGNED_INT, NULL);

		// Draw Points.
		glUseProgram(ShaderPrograms[PointProgram]);
		glBindVertexArray(VAOs[PointsVAO]);
		glDrawArrays(GL_POINTS, 0, NUM_POINTS);


		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

GLuint CreateShader(GLenum shaderType, std::string filename) {
	GLuint shader = glCreateShader(shaderType);
	std::ifstream is(filename);

	if (is.fail()) {
		std::cout << "Error opening " << filename;
		return 0;
	}

	std::string source;
	std::string line;
	while (!is.eof()) {
		std::getline(is, line);
		source += line + '\n';
	}
	const GLchar* sourceChar(source.c_str());
	glShaderSource(shader, 1, &sourceChar, 0);
	glCompileShader(shader);

	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << "Error: Shader compilation failed for " << filename << "\n" << infoLog << std::endl;
	}

	return shader;
}

void SortPointsByPolarAngle(const GLfloat pointPos[][2], GLuint* hullVertIndices, GLuint numPoints) {
	const GLuint basePoint = hullVertIndices[0];

	// Comparison function for point indices, invariant to sorting algorithm.
	const static auto cmp = [&pointPos, basePoint](GLuint a, GLuint b) {
		// Negative reciprocal slope of A.
		float delta_y = pointPos[a][1] - pointPos[basePoint][1];
		float delta_x = pointPos[a][0] - pointPos[basePoint][0];

		float slope;
		if (delta_x == 0.0f) {
			slope = std::numeric_limits<float>::max();
		}
		else {
			slope = delta_y / delta_x;
		}

		float neg_recip_slope_a;
		if (slope == 0.0f) {
			neg_recip_slope_a = -std::numeric_limits<float>::max();
		}
		else {
			neg_recip_slope_a = -1.0f / slope;
		}

		// Negative reciprocal slope of B.
		delta_y = pointPos[b][1] - pointPos[basePoint][1];
		delta_x = pointPos[b][0] - pointPos[basePoint][0];

		if (delta_x == 0.0f) {
			slope = std::numeric_limits<float>::max();
		}
		else {
			slope = delta_y / delta_x;
		}

		float neg_recip_slope_b;
		if (slope == 0.0f) {
			neg_recip_slope_b = -std::numeric_limits<float>::max();
		}
		else {
			neg_recip_slope_b = -1.0f / slope;
		}

		return neg_recip_slope_a < neg_recip_slope_b;
		};

	// Implement sort as merge sort.
	int* temp = new int[numPoints];

	size_t stride = 1;
	while (stride < numPoints) {
		for (size_t i = 1; i < numPoints; i += 2 * stride) {
			size_t index = i;
			size_t firstHalf = i;
			size_t secondHalf = i + stride;

			while (firstHalf < i + stride && firstHalf < numPoints && secondHalf < i + 2 * stride && secondHalf < numPoints) {
				// Both halves have elements, compare them.
				if (cmp(hullVertIndices[firstHalf], hullVertIndices[secondHalf])) {
					temp[index++] = hullVertIndices[firstHalf++];
				}
				else {
					temp[index++] = hullVertIndices[secondHalf++];
				}
			}

			//// Dump rest of each half into temp.
			while (firstHalf < i + stride && firstHalf < numPoints) {
				temp[index++] = hullVertIndices[firstHalf++];
			}

			while (secondHalf < i + 2 * stride && secondHalf < numPoints) {
				temp[index++] = hullVertIndices[secondHalf++];
			}

			// Copy temp back to the original array.
			for (size_t j = i; j < i + 2 * stride && j < numPoints; ++j) {
				hullVertIndices[j] = temp[j];
			}
		}
		stride = stride * 2;
	}

	delete[] temp;
}