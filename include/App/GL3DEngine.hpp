#ifndef GL_3D_ENGINE_HPP_
#define GL_3D_ENGINE_HPP_

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <ctime>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Tools.hpp"

namespace eng {
  enum CentroidGroup { Front, Back, Left, Right, Up, Down, None };
  //                     0     1     2      3    4    5     6

  const std::unordered_map<char, CentroidGroup> CENTERS = { {'f', CentroidGroup::Left},//Orange
                                                            {'b', CentroidGroup::Right},//Red
                                                            {'r', CentroidGroup::Back},//Blue
                                                            {'l', CentroidGroup::Front},//Green
                                                            {'u', CentroidGroup::Down},//Yellow
                                                            {'d', CentroidGroup::Up} };//White
  typedef std::unordered_map<CentroidGroup, glm::vec3> CenterMap;

  CenterMap MAP_CENTERS = { {CentroidGroup::Front, glm::vec3( 0.f,  0.f,  1.f)},
                            {CentroidGroup::Back,  glm::vec3( 0.f,  0.f, -1.f)},
                            {CentroidGroup::Left,  glm::vec3(-1.f,  0.f,  0.f)},
                            {CentroidGroup::Right, glm::vec3( 1.f,  0.f,  0.f)},
                            {CentroidGroup::Up,    glm::vec3( 0.f,  1.f,  0.f)},
                            {CentroidGroup::Down,  glm::vec3( 0.f, -1.f,  0.f)} };
	void initOpenGL() {
    srand(time(0));
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 16);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	}

	GLFWwindow* get3DWindow(const uint32_t& SCR_WIDTH, const uint32_t& SCR_HEIGHT, char* title) {
		GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, title, 0, 0);
		if (window == nullptr) {
			std::cerr << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
		glfwMakeContextCurrent(window);
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			std::cerr << "Failed to initialize GLAD" << std::endl;
			exit(EXIT_FAILURE);
		}
		glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
		glDepthFunc(GL_LESS);
		return window;
	}

  std::vector<std::pair<CentroidGroup, float>> parseSolverOutput(std::vector<char>& solution) {
    std::vector<std::pair<CentroidGroup, float>> parsedSolution;
    char lastMovementCode = 'a';
    for (const char& movement : solution) {
      std::pair<CentroidGroup, float> parsedMovement = {CENTERS.find(std::tolower(movement))->second, ((std::isupper(movement)) ? -90.f : 90.f)};
      CenterMap::iterator centerO_ = MAP_CENTERS.find(parsedMovement.first);
      parsedMovement.second *= (centerO_->second[0] + centerO_->second[1] + centerO_->second[2]);
      if (!parsedSolution.empty() && movement == lastMovementCode)
        parsedSolution.back().second *= 2.f;
      else
        parsedSolution.emplace_back(parsedMovement);
      lastMovementCode = movement;
    }
    return parsedSolution;
  }

	class Cube3D {
	private:
    typedef uint32_t GL_Object3D;
    GL_Object3D VBO;
    GL_Object3D VAO;
		tool::Texture* TEX_F, *TEX_B, *TEX_L, *TEX_R, *TEX_U, *TEX_D;
    glm::mat4 model, animModel, mOrbit, proposal;
    std::array<CentroidGroup, 3> clusters;
    glm::vec3 currentMembership;
    float rotAngle = 0.f, goalAngle = 0.f;
    int currentOrientation;
    bool animated = false, fixOrientationRequired = false;
    std::shared_ptr<bool> proposalEnabled;

    GLfloat VBD[288] = {
      //BACK
         -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
          0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
          0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
          0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
         -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f,
         -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
      //FRONT
         -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
          0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f,
          0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
          0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
         -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  0.0f,  1.0f,
         -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
      //LEFT
         -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f,
         -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f,
         -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f,
         -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f,
         -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f,
         -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f,
      //RIGHT
          0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,
          0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
          0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f,
          0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f,
          0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f,
          0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,
      //DOWN
         -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f,
          0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
          0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f,
          0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f,
         -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f,
         -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f,
      //UP
         -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f,
          0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
          0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
          0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
         -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f,
         -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f
    };

    void destroyDrawable(GL_Object3D& VBO, GL_Object3D& VAO);
    void drawSingleFace(const std::size_t& begin, const std::size_t& count);
    void updateCenters(const glm::vec3& centers);
    
	public:
		Cube3D(tool::Texture& refT_R, tool::Texture& refT_L, tool::Texture& refT_U,
           tool::Texture& refT_D, tool::Texture& refT_F, tool::Texture& refT_B);
    Cube3D(){
      TEX_F = TEX_B = TEX_L = TEX_R = TEX_U = TEX_D = nullptr;
      VBO = 0;
      VAO = 0;
      clusters.fill(CentroidGroup::None);
      model = mOrbit = proposal = glm::mat4(1.0f);
      currentOrientation = 0;
    }

    ~Cube3D();

    void setFrontTex(tool::Texture& refT) {
      TEX_F = &refT;
    }

    void setBackTex(tool::Texture& refT) {
      TEX_B = &refT;
    }

    void setLeftTex(tool::Texture& refT) {
      TEX_L = &refT;
    }

    void setRightTex(tool::Texture& refT) {
      TEX_R = &refT;
    }

    void setUpTex(tool::Texture& refT) {
      TEX_U = &refT;
    }

    void setDownTex(tool::Texture& refT) {
      TEX_D = &refT;
    }

    int getRemainderAngle() {
      std::cout << "Current Ori: " << currentOrientation << std::endl;
      return 360 - currentOrientation;
    }

    char getAsciiIndex() {
      if (currentMembership.x > 0.f)
        return 'b';
      if (currentMembership.x < 0.f)
        return 'f';
      if (currentMembership.y > 0.f)
        return 'd';
      if (currentMembership.y < 0.f)
        return 'u';
      if (currentMembership.z > 0.f)
        return 'l';
      if (currentMembership.z < 0.f)
        return 'r';
      return 'n';
    }

    void setProposalEnabled(std::shared_ptr<bool>& globalEnabled) {
      proposalEnabled = globalEnabled;
    }

    void draw(tool::Shader& shader);
    void setFixRequired(bool enabled);
    void translate(const glm::vec3& position);
    void rotateAround(const float& angle, const glm::vec3& center, const glm::vec3& axis);
    std::string getPrintableModel();
    bool belongingTo(const CentroidGroup& center);
    bool needFixOrientation();
    bool isCenter();
    void updateMembership(const glm::vec3& axisRotation);
    const glm::vec3& getCurrentMembership();
    void setAnimatedEnable(bool enabled, const float& goal);
	};

  Cube3D::Cube3D(tool::Texture& refT_R, tool::Texture& refT_L, tool::Texture& refT_U,
                 tool::Texture& refT_D, tool::Texture& refT_F, tool::Texture& refT_B) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VBD), VBD, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    TEX_F = &refT_F; TEX_B = &refT_B;
    TEX_L = &refT_L; TEX_R = &refT_R;
    TEX_U = &refT_U; TEX_D = &refT_D;
    glBindVertexArray(0);
    model = mOrbit = proposal = glm::mat4(1.0f);
    clusters.fill(CentroidGroup::None);
    currentOrientation = 0;
  }

  Cube3D::~Cube3D() {
    destroyDrawable(VBO, VAO);
  }

  void Cube3D::destroyDrawable(GL_Object3D& VBO, GL_Object3D& VAO) {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
  }

  void Cube3D::drawSingleFace(const std::size_t& begin, const std::size_t& count) {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, begin, count);
  }

  float getTimedScale(float time) {
    time = time + time / float(rand() % 5 + 8);
    return std::min(glm::cos(time), 2.f) + 1.5f;
  }

  void Cube3D::draw(tool::Shader& shader) {
    if(animated)
      shader.setMat4("model", animModel);
    else
      shader.setMat4("model", model);
    if (proposalEnabled != nullptr && *proposalEnabled == true) {
      float time = glfwGetTime();
      time = std::fmod(time, 5.5);
      proposal = glm::scale(glm::mat4(1.f), glm::vec3(getTimedScale(time),
                                                      getTimedScale(time), 
                                                      getTimedScale(time)));
      float variant = glm::cos(time + 0.42) * 0.7;
      proposal = glm::translate(proposal, glm::vec3(currentMembership.x * variant, 
                                                    currentMembership.y * variant, 
                                                    currentMembership.z * variant));
    }
    else {
      proposal = glm::mat4(1.f);
    }
    shader.setMat4("animation", proposal);
    TEX_B->apply();
    shader.use();
    drawSingleFace(0, 6);
    TEX_F->apply();
    shader.use();
    drawSingleFace(6, 6);
    TEX_L->apply();
    shader.use();
    drawSingleFace(12, 6);
    TEX_R->apply();
    shader.use();
    drawSingleFace(18, 6);
    TEX_D->apply();
    shader.use();
    drawSingleFace(24, 6);
    TEX_U->apply();
    shader.use();
    drawSingleFace(30, 6);
  }

  void Cube3D::translate(const glm::vec3& position) {
    model = glm::translate(glm::mat4(1.f), position);
    animModel = model;
    currentMembership = position;
    updateCenters(position);
  }

  std::string Cube3D::getPrintableModel() {
    return glm::to_string(model);
  }

  bool Cube3D::belongingTo(const CentroidGroup& center) {
    for(const CentroidGroup& axis : clusters)
      if (axis == center)
        return true;
    return false;
  }

  void Cube3D::rotateAround(const float& angle, const glm::vec3& center, const glm::vec3& axis) {
    rotAngle += angle;
    rotAngle = glm::sign(rotAngle) * min(glm::abs(goalAngle), glm::abs(rotAngle));
    mOrbit = glm::inverse(glm::translate(glm::mat4(1.f), center));
    mOrbit = glm::rotate(mOrbit, glm::radians(rotAngle), axis);
    mOrbit = glm::translate(mOrbit, center);
    animModel = mOrbit * model;
  }

  bool Cube3D::isCenter() {
    return (glm::abs(currentMembership.x) + glm::abs(currentMembership.y) + glm::abs(currentMembership.z)) == 1.f;
  }

  void Cube3D::updateMembership(const glm::vec3& axisRotation) {
    if (!isCenter()) {
      currentMembership = glm::vec3(mOrbit * glm::vec4(currentMembership, 1.f));
      currentMembership = glm::vec3(int(currentMembership.x + glm::sign(currentMembership.x) * 0.5f),
        int(currentMembership.y + glm::sign(currentMembership.y) * 0.5f),
        int(currentMembership.z + glm::sign(currentMembership.z) * 0.5f));
      updateCenters(currentMembership);
    }
    else {
      if (fixOrientationRequired) {
        currentOrientation = tool::Mod(currentOrientation + int(goalAngle), 360);
        if (currentOrientation == 360)
          currentOrientation = 0;
      }
    }
    model = animModel;
    rotAngle = 0.f;
  }

  void Cube3D::updateCenters(const glm::vec3& centers) {
    clusters[0] = (centers[0] > 0) ? CentroidGroup::Right : ((centers[0] < 0) ? CentroidGroup::Left  : CentroidGroup::None);
    clusters[1] = (centers[1] > 0) ? CentroidGroup::Up    : ((centers[1] < 0) ? CentroidGroup::Down  : CentroidGroup::None);
    clusters[2] = (centers[2] > 0) ? CentroidGroup::Front : ((centers[2] < 0) ? CentroidGroup::Back  : CentroidGroup::None);
  }

  const glm::vec3& Cube3D::getCurrentMembership() {
    return currentMembership;
  }

  void Cube3D::setFixRequired(bool enabled) {
    fixOrientationRequired = enabled;
  }

  bool Cube3D::needFixOrientation() {
    return (currentOrientation != 0) && fixOrientationRequired;
  }

  void Cube3D::setAnimatedEnable(bool enabled, const float& goal) {
    animated = enabled;
    goalAngle = goal;
  }

  class SkyBox3D {
  private:
    typedef uint32_t GL_Object3D;

    GLfloat VBD[108] = {       
      -1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,

      -1.0f, -1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,

      -1.0f,  1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      -1.0f,  1.0f, -1.0f,

      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
       1.0f, -1.0f,  1.0f
    };
    GL_Object3D VBO;
    GL_Object3D VAO;
    tool::Shader skyboxShader;
    tool::Texture* SKYBOX_TEXTURE;
  public:
    ~SkyBox3D() {
      glDeleteVertexArrays(1, &VAO);
      glDeleteBuffers(1, &VBO);
    }

    void init(tool::Texture& texture) {
      glGenVertexArrays(1, &VAO);
      glGenBuffers(1, &VBO);
      glBindVertexArray(VAO);
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(VBD), VBD, GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
      skyboxShader.loadFromMemory(tool::defaultVertexSkyBoxShaderCode, tool::defaultFragmentSkyBoxShaderCode);
      SKYBOX_TEXTURE = &texture;
    }

    void draw(const glm::mat4& view, const glm::mat4& projection) {
      glDisable(GL_DEPTH_TEST);
      skyboxShader.use();
      skyboxShader.setMat4("view", view);
      skyboxShader.setMat4("projection", projection);
      glBindVertexArray(VAO);
      SKYBOX_TEXTURE->skyboxModeApply();
      glDrawArrays(GL_TRIANGLES, 0, 36);
      //glBindVertexArray(0);
      glEnable(GL_DEPTH_TEST);
    }
  };

}

#endif//GL_3D_ENGINE_HPP_