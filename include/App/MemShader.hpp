#ifndef MEM_SHADER_HPP_
#define MEM_SHADER_HPP_

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace tool {

  const char* defaultVertexShaderCode = "#version 330 core\n"
                                        "layout(location = 0) in vec3 aPos;\n"
                                        "layout(location = 1) in vec2 aTexCoord;\n"
                                        "layout(location = 2) in vec3 aNormal;\n"
                                        "out vec3 FragPos;\n"
                                        "out vec3 Normal;\n"
                                        "out vec2 TexCoord;\n"
                                        "uniform mat4 model;\n"
                                        "uniform mat4 view;\n"
                                        "uniform mat4 animation;\n"
                                        "uniform mat4 projection;\n"
                                        "void main()\n"
                                        "{\n"
                                        "  FragPos = vec3(model * vec4(aPos, 1.0));\n"
                                        "  Normal = mat3(transpose(inverse(model))) * aNormal;\n"
                                        "  gl_Position = projection * view * animation * vec4(FragPos, 1.0f);\n"
                                        "  TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
                                        "}";

  const char* defaultFragmentShaderCode = "#version 330 core\n"
                                          "out vec4 FragColor;\n"
                                          "in vec2 TexCoord;\n"
                                          "in vec3 Normal;\n"
                                          "in vec3 FragPos;\n"
                                          "uniform vec3 lightPos;\n"
                                          "uniform vec3 viewPos;\n"
                                          "uniform vec3 lightColor;\n"
                                          "uniform sampler2D tex_face;\n"
                                          "void main()\n"
                                          "{\n"
                                          "  float ambientStrength = 0.3;\n"
                                          "  vec3 ambient = ambientStrength * vec3(1.0,1.0,1.0);\n"
                                          "  vec3 norm = normalize(Normal);\n"
                                          "  vec3 lightDir = normalize(lightPos - FragPos);\n"
                                          "  float diff = max(dot(norm, lightDir), 0.0);\n"
                                          "  vec3 diffuse = diff * lightColor * 0.52;\n"
                                          "  float specularStrength = 0.3;\n"
                                          "  vec3 viewDir = normalize(viewPos - FragPos);\n"
                                          "  vec3 reflectDir = reflect(-lightDir, norm);\n"
                                          "  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 3);\n"
                                          "  vec3 specular = specularStrength * spec * lightColor;\n"
                                          "  vec3 result = ambient + diffuse + specular;\n"
                                          "  FragColor = vec4(lightColor,0.0) - vec4(result, 1.0) * texture(tex_face, TexCoord);\n"
                                          "}";

  const char* defaultFragmentSkyBoxShaderCode = "#version 330 core\n"
                                                "out vec4 FragColor;\n"
                                                "in vec3 TexCoords;\n"
                                                "uniform samplerCube skybox;\n"
                                                "void main()\n"
                                                "{\n"
                                                "  FragColor = texture(skybox, TexCoords);\n"
                                                "}";

  const char* defaultVertexSkyBoxShaderCode = "#version 330 core\n"
                                              "layout(location = 0) in vec3 aPos;\n"
                                              "out vec3 TexCoords;\n"
                                              "uniform mat4 projection;\n"
                                              "uniform mat4 view;\n"
                                              "void main()\n"
                                              "{\n"
                                              "  TexCoords = aPos;\n"
                                              "  vec4 pos = projection * view * vec4(aPos, 1.0);\n"
                                              "  gl_Position = pos.xyww;\n"
                                              "}";

  class Shader {
  public:
    uint32_t ID;
    void loadFromFile(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr) {
      std::string vertexCode;
      std::string fragmentCode;
      std::string geometryCode;
      std::ifstream vShaderFile;
      std::ifstream fShaderFile;
      std::ifstream gShaderFile;
      vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        if (geometryPath != nullptr) {
          gShaderFile.open(geometryPath);
          std::stringstream gShaderStream;
          gShaderStream << gShaderFile.rdbuf();
          gShaderFile.close();
          geometryCode = gShaderStream.str();
        }
      }
      catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
      }
      loadFromMemory(vertexCode.c_str(), fragmentCode.c_str(), ((geometryPath != nullptr) ? geometryCode.c_str() : nullptr));
    }

    void loadFromMemory(const char* vShaderCode, const char* fShaderCode, const char* gShaderCode = nullptr) {
      uint32_t vertex, fragment;
      vertex = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(vertex, 1, &vShaderCode, NULL);
      glCompileShader(vertex);
      checkCompileErrors(vertex, "VERTEX");
      fragment = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(fragment, 1, &fShaderCode, NULL);
      glCompileShader(fragment);
      checkCompileErrors(fragment, "FRAGMENT");
      uint32_t geometry;
      if (gShaderCode != nullptr) {
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &gShaderCode, NULL);
        glCompileShader(geometry);
        checkCompileErrors(geometry, "GEOMETRY");
      }
      ID = glCreateProgram();
      glAttachShader(ID, vertex);
      glAttachShader(ID, fragment);
      if (gShaderCode != nullptr)
        glAttachShader(ID, geometry);
      glLinkProgram(ID);
      checkCompileErrors(ID, "PROGRAM");
      glDeleteShader(vertex);
      glDeleteShader(fragment);
      if (gShaderCode != nullptr)
        glDeleteShader(geometry);
    }

    void use() {
      glUseProgram(ID);
    }

    void setBool(const std::string& name, bool value) const {
      glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
      glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
      glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string& name, const glm::vec2& value) const
    {
      glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string& name, float x, float y) const
    {
      glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
      glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
      glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
      glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string& name, float x, float y, float z, float w)
    {
      glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
      glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
      glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
      glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

  private:
    void checkCompileErrors(GLuint shader, std::string type)
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
        }
      }
      else
      {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
          glGetProgramInfoLog(shader, 1024, NULL, infoLog);
          std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
      }
    }
  };
}

#endif//MEM_SHADER_HPP_