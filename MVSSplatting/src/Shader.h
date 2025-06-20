#ifndef SHADER_H
#define SHADER_H


#include <glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

class Shader
{
private:
	unsigned int ID;
	mutable std::unordered_map<std::string, GLint> uniformLocationMap;

public:
	Shader() {
		ID = 0;
	}
	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	bool init(const char* vertexPath, const char* fragmentPath = nullptr, const char* geometryPath = nullptr)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::string geometryCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		std::ifstream gShaderFile;
		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			vShaderFile.open(vertexPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();

			// fragment shader
			if (fragmentPath == "") {
				fragmentPath = nullptr;
			}
			if (fragmentPath != nullptr) {
				fShaderFile.open(fragmentPath);
				fShaderStream << fShaderFile.rdbuf();
				fShaderFile.close();
				fragmentCode = fShaderStream.str();
			}

			// if geometry shader path is present, also load a geometry shader
			if (geometryPath != nullptr)
			{
				gShaderFile.open(geometryPath);
				std::stringstream gShaderStream;
				gShaderStream << gShaderFile.rdbuf();
				gShaderFile.close();
				geometryCode = gShaderStream.str();
			}
		}
		catch (std::ifstream::failure)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
			return false;
		}
		const char* vShaderCode = vertexCode.c_str();
		// 2. compile shaders
		unsigned int vertex;
		// vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		if (!checkCompileErrors(vertex, "VERTEX")) {
			return false;
		}
		
		// fragment Shader
		unsigned int fragment;
		if (fragmentPath != nullptr) {
			const char* fShaderCode = fragmentCode.c_str();
			fragment = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragment, 1, &fShaderCode, NULL);
			glCompileShader(fragment);
			if (!checkCompileErrors(fragment, "FRAGMENT")) {
				return false;
			}
		}

		// if geometry shader is given, compile geometry shader
		unsigned int geometry;
		if (geometryPath != nullptr)
		{
			const char* gShaderCode = geometryCode.c_str();
			geometry = glCreateShader(GL_GEOMETRY_SHADER);
			glShaderSource(geometry, 1, &gShaderCode, NULL);
			glCompileShader(geometry);
			if (!checkCompileErrors(geometry, "GEOMETRY")) {
				return false;
			}
		}
		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		if (fragmentPath != nullptr) 
			glAttachShader(ID, fragment);
		if (geometryPath != nullptr)
			glAttachShader(ID, geometry);

		if (fragmentPath != nullptr) {
			glLinkProgram(ID);
			if (!checkCompileErrors(ID, "PROGRAM")) {
				return false;
			}
		}
		
		// delete the shaders as they're linked into our program now and no longer necessery
		glDeleteShader(vertex);
		if (fragmentPath != nullptr)
			glDeleteShader(fragment);
		if (geometryPath != nullptr)
			glDeleteShader(geometry);
		return true;

	}
	// activate the shader
	// ------------------------------------------------------------------------
	void use()
	{
		glUseProgram(ID);
	}
	// utility uniform functions
	// ------------------------------------------------------------------------
	GLint getUniformLocation(const std::string& name) const {
		if (uniformLocationMap.find(name) != uniformLocationMap.end()) {
			return uniformLocationMap[name];
		}
		GLint location = glGetUniformLocation(ID, name.c_str());
		uniformLocationMap[name] = location;
		return location;
	}


	void setBool(const std::string& name, bool value) const
	{
		glUniform1i(getUniformLocation(name), (int)value);
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string& name, int value) const
	{
		glUniform1i(getUniformLocation(name), value);
	}
	// ------------------------------------------------------------------------
	void setFloat(const std::string& name, float value) const
	{
		glUniform1f(getUniformLocation(name), value);
	}
	// ------------------------------------------------------------------------
	void setVec2(const std::string& name, const glm::vec2& value) const
	{
		glUniform2fv(getUniformLocation(name), 1, &value[0]);
	}
	void setVec2(const std::string& name, float x, float y) const
	{
		glUniform2f(getUniformLocation(name), x, y);
	}
	// ------------------------------------------------------------------------
	void setVec3(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(getUniformLocation(name), 1, &value[0]);
	}
	void setVec3(const std::string& name, float x, float y, float z) const
	{
		glUniform3f(getUniformLocation(name), x, y, z);
	}
	// ------------------------------------------------------------------------
	void setVec4(const std::string& name, const glm::vec4& value) const
	{
		glUniform4fv(getUniformLocation(name), 1, &value[0]);
	}
	void setVec4(const std::string& name, float x, float y, float z, float w)
	{
		glUniform4f(getUniformLocation(name), x, y, z, w);
	}
	// ------------------------------------------------------------------------
	void setMat2(const std::string& name, const glm::mat2& mat) const
	{
		glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat3(const std::string& name, const glm::mat3& mat) const
	{
		glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat4(const std::string& name, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
	}

	bool InitTransformFeedShader(int nrVaryings, const char* varyings[]) {
		glTransformFeedbackVaryings(ID, nrVaryings, varyings, GL_INTERLEAVED_ATTRIBS);
		glLinkProgram(ID);
		if (!checkCompileErrors(ID, "PROGRAM")) {
			return false;
		}
		return true;
	}


private:
	// utility function for checking shader compilation/linking errors.
	// ------------------------------------------------------------------------
	bool checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
				return false;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
				return false;
			}
		}
		return true;
	}
};


#endif