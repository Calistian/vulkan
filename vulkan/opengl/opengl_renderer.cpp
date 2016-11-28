#include <gl/glew.h>
#include "opengl_renderer.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

using namespace opengl;
using namespace std;

void opengl_renderer::init(GLFWwindow* window) {}

struct model_opengl_data
{
	GLuint vao;
	GLuint vertex_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;
};

static GLuint create_shader(const string& path, GLenum type)
{
	auto shader = glCreateShader(type);

	ifstream file(path, ios::ate);
	vector<char> source(file.tellg());
	GLint source_size = size(source);
	file.seekg(0);
	file.read(data(source), source_size);
	file.close();

	auto source_data = data(source);

	glShaderSource(shader, 1, &source_data, &source_size);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	GLint log_length;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
	if(status != GL_TRUE || log_length > 1)
	{
		vector<char> log(log_length + 1);
		glGetShaderInfoLog(shader, size(log) - 1, nullptr, data(log));
		cerr << "Failed to compile shader " << path << endl;
		cerr << data(log) << endl;
		throw runtime_error("Failed to compile shader");
	}

	return shader;
}

GLuint create_program(const std::string& vs, const std::string& fs)
{
	auto vertex_shader = create_shader(vs, GL_VERTEX_SHADER);
	auto fragment_shader = create_shader(fs, GL_FRAGMENT_SHADER);

	auto program = glCreateProgram();

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	GLint log_length;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
	if(status != GL_TRUE || log_length > 1)
	{
		vector<char> log(log_length + 1);
		glGetProgramInfoLog(program, size(log) - 1, nullptr, data(log));
		cerr << "Failed to link program" << endl;
		cerr << data(log) << endl;
		throw runtime_error("Failed to link program");
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}

void opengl_renderer::init_scene(scene& sc)
{
	for(auto& obj : sc.objects)
	{
		model_opengl_data model_data;
		if (obj.model->user_data.empty())
		{
			glGenVertexArrays(1, &model_data.vao);
			glBindVertexArray(model_data.vao);

			glGenBuffers(1, &model_data.vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, model_data.vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(obj.model->vertices[0]) * size(obj.model->vertices), data(obj.model->vertices), GL_STATIC_DRAW);

			glGenBuffers(1, &model_data.normal_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, model_data.normal_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(obj.model->normals[0]) * size(obj.model->normals), data(obj.model->normals), GL_STATIC_DRAW);

			glGenBuffers(1, &model_data.index_buffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model_data.index_buffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(obj.model->indices[0]) * size(obj.model->indices), data(obj.model->indices), GL_STATIC_DRAW);

			obj.model->user_data = model_data;
		}

		obj.user_data = create_program(obj.vertex_shader.filename, obj.fragment_shader.filename);
	}
}

void opengl_renderer::render(const scene& sc)
{
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for(auto& obj : sc.objects)
	{
		auto program = any_cast<GLuint>(obj.user_data);

		auto ubo_model = glGetUniformLocation(program, "model");
		auto ubo_view = glGetUniformLocation(program, "view");
		auto ubo_proj = glGetUniformLocation(program, "proj");
		auto ubo_material_ambiant = glGetUniformLocation(program, "material.ambiant");
		auto ubo_material_diffuse = glGetUniformLocation(program, "material.diffuse");
		auto ubo_material_specular = glGetUniformLocation(program, "material.specular");
		auto ubo_material_hardness = glGetUniformLocation(program, "material.hardness");
		auto ubo_point_pos = glGetUniformLocation(program, "point.pos");
		auto ubo_point_ambiant = glGetUniformLocation(program, "point.pos");
		auto ubo_point_diffuse = glGetUniformLocation(program, "point.diffuse");
		auto ubo_point_specular = glGetUniformLocation(program, "point.specular");
		auto ubo_point_attenuation = glGetUniformLocation(program, "point.attenuation");
		auto ubo_eye = glGetUniformLocation(program, "eye");

		glUseProgram(program);

		glUniformMatrix4fv(ubo_model, 1, GL_FALSE, &obj.trans[0][0]);
		glUniformMatrix4fv(ubo_view, 1, GL_FALSE, &sc.view[0][0]);
		glUniformMatrix4fv(ubo_proj, 1, GL_FALSE, &sc.projection[0][0]);
		glUniform4fv(ubo_material_ambiant, 1, &obj.material.ambiant[0]);
		glUniform4fv(ubo_material_diffuse, 1, &obj.material.diffuse[0]);
		glUniform4fv(ubo_material_specular, 1, &obj.material.specular[0]);
		glUniform4fv(ubo_material_hardness, 1, &obj.material.hardness[0]);
		glUniform4fv(ubo_point_pos, 1, &sc.point.pos[0]);
		glUniform4fv(ubo_point_ambiant, 1, &sc.point.ambiant[0]);
		glUniform4fv(ubo_point_diffuse, 1, &sc.point.diffuse[0]);
		glUniform4fv(ubo_point_specular, 1, &sc.point.specular[0]);
		glUniform4fv(ubo_point_attenuation, 1, &sc.point.attenuation[0]);
		glUniform4fv(ubo_eye, 1, &sc.eye[0]);

		auto model_data = any_cast<model_opengl_data>(obj.model->user_data);

		glBindVertexArray(model_data.vao);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, model_data.vertex_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, model_data.normal_buffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model_data.index_buffer);

		glDrawElements(GL_TRIANGLES, size(obj.model->indices), GL_UNSIGNED_INT, nullptr);

		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		glUseProgram(0);
	}
}

void opengl_renderer::cleanup(scene& sc)
{
	
}
