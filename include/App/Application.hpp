#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

#include "GL3DEngine.hpp"
#include "RubikSolverPocket/RubikSolver.h"

#include <fstream>
#include <glm/gtx/string_cast.hpp>

#define CAMERA_LIMIT_MIN -2.8f
#define CAMERA_LIMIT_MAX  1.f

namespace app {
	double mouseX = 0, mouseY = 0;
	bool moving = false;

	class ArcBallCamera {
	private:
		glm::vec3 cameraPosition = glm::vec3(3.f, 3.f, 3.f), cameraTarget = glm::vec3(0, 0, 0), cameraUpVector = glm::vec3(0, 1, 0);
		glm::vec3 cameraDirection, cameraRight;
		glm::mat4 view = glm::mat4(1.0f);
		float angleX = 0.f, angleY = 0.f, angleZ = 0.f, angleLastX = 0.f, angleLastY = 0.f, angleLastZ = 0.f;
		float esphereRadius = 1.5f;
		bool updated = false;
	public:
		ArcBallCamera() {
			cameraDirection = glm::normalize(cameraPosition - cameraTarget);
			cameraRight = glm::normalize(glm::cross(cameraUpVector, cameraDirection));
		}

		void rotateAxis_X(const float& xNew) {
			angleX = fmod(xNew, 360.f);
			if (angleLastX != angleX) {
				updated = false;
			}
		}

		void rotateAxis_Y(const float& yNew) {
			angleY = fmod(yNew, 360.f);
			if (angleLastY != angleY)
				updated = false;
		}

		void rotateAxis_Z(const float& zNew) {
			angleZ = fmod(zNew, 360.f);
			if (angleLastZ != angleZ)
				updated = false;
		}

		void animateCamera(const float& time) {
			float angleRot = glm::radians((cos(time) + 1) * 90.f);
			float r = esphereRadius * (cos(time) + 1);
			cameraPosition.x = r * cos(angleRot * time);
			cameraPosition.y = cos(time);
			cameraPosition.z = (-1) * r * sin(angleRot * time);
			updated = false;
		}

		const glm::vec3& getGlobalPosition() {
			return cameraPosition;
		}

		const glm::mat4& getView() {
			if (!updated) {
				glm::mat4 mOrbit = glm::inverse(glm::translate(glm::mat4(1.f), cameraPosition));
				mOrbit = glm::rotate(mOrbit, glm::radians(angleX), cameraUpVector);
				mOrbit = glm::rotate(mOrbit, glm::radians(angleY), cameraRight);
				mOrbit = glm::translate(mOrbit, cameraPosition);
				cameraPosition = glm::vec3(mOrbit * glm::vec4(cameraPosition, 1.0f));
				cameraDirection = glm::normalize(cameraPosition - cameraTarget);
				cameraRight = glm::normalize(glm::rotate(glm::mat4(1.f), glm::radians(angleZ), cameraDirection) * glm::vec4(glm::cross(cameraUpVector, cameraDirection),1.f));
				cameraUpVector = glm::normalize(glm::cross(cameraDirection, cameraRight));
				view = glm::lookAt(cameraPosition, cameraTarget, cameraUpVector);
				angleLastX = angleX;
				angleLastY = angleY;
				updated = true;
			}
			return view;
		}
	};

	class RubikCube3D {
	private:
		enum AnimationStatus { Paused, Running };
		enum MovementStatus { Solving, Shuffling, Fixing, Customizing, Idle };
		std::vector<eng::Cube3D> body;
		tool::Texture TEXTURE_STORAGE[6][3][3], TEX_HIDDEN;
		tool::Shader cubeShader;
		algo::Rubik solver;
		ArcBallCamera camera;
		std::vector<std::pair<eng::CentroidGroup, float>> solutionMovements;
		std::array<eng::Cube3D*, 9> currentGroup;
		std::size_t currentSMIndx = 0.f;
		AnimationStatus currentStatus;
		MovementStatus actionStatus;
		float animationSpeed = 5.f, rotationCount = 0.f;
		std::vector<char> solution, mix;
		bool mayusEnabled = false;
		std::shared_ptr<bool> globalProposalControl;

		glm::vec3 currentCenterVector, currentAxisRot;
		glm::mat4 projection = glm::mat4(1.0f);
		float cameraZoom = CAMERA_LIMIT_MIN / 2;

		bool fixerHeuristicRequired(std::vector<std::pair<char, int>>& wrongCenters) {
			wrongCenters.clear();
			bool needed = false;
			std::cout << "\nFixer Required\n";
			for (auto& cubie : body) {
				if (cubie.isCenter() && cubie.needFixOrientation()) {
					std::cout << "->" << cubie.getAsciiIndex() << ' ' << cubie.getRemainderAngle() << std::endl;
					wrongCenters.emplace_back(cubie.getAsciiIndex(), cubie.getRemainderAngle());
					needed = true;
				}
			}
			return needed;
		}

		void findRubikSolution() {
			if (!solver.isSolved() && solutionMovements.empty()) {
				solution.clear();
				solver.solve(solution);
				tool::fixSequenceLogic(solution);
				solutionMovements = eng::parseSolverOutput(solution);
				std::cout << ">> Expected: ";
				for (auto& l : solution) {
					std::cout << l;
				}std::cout << std::endl;
				solver.printAll();
				actionStatus = Solving;
			}
		}

		void shuffleRubikRandom() {
			if (solver.isSolved() && solutionMovements.empty()) {
				mix.clear();
				tool::randomShuffle(mix, rand() % 10 + 20);
				solutionMovements = eng::parseSolverOutput(mix);
				std::cout << ">> Mixer movements: ";
				for (auto& l : mix) {
					cout << l;
				}std::cout << std::endl;
				solver.movSolver(mix);
				solver.printAll();
				actionStatus = Shuffling;
			}
		}

		void customizeMoves(const char& mov_code) {
			char fixed_mov = (mayusEnabled) ? std::toupper(mov_code) : mov_code;
			std::vector<char> movs = {fixed_mov};
			solutionMovements = eng::parseSolverOutput(movs);
			solver.movSolver(movs);
			solver.printAll();
			actionStatus = Customizing;
		}

		void execAnimations(float deltaTime) {
			if (!solutionMovements.empty()) {
				if (currentSMIndx < solutionMovements.size() && currentStatus == Paused) {
					std::size_t idx = 0;
					eng::CenterMap::iterator it = eng::MAP_CENTERS.find(solutionMovements[currentSMIndx].first);
					currentCenterVector = it->second;
					currentAxisRot = glm::abs(it->second);
					for (auto& cubie : body) {
						if (idx < 9 && cubie.belongingTo(solutionMovements[currentSMIndx].first)) {
							cubie.setAnimatedEnable(true, solutionMovements[currentSMIndx].second);
							currentGroup[idx++] = &cubie;
						}
					}
					if (solution.size())
						std::cout << solution[currentSMIndx];
					else if (mix.size())
						std::cout << mix[currentSMIndx];
					currentStatus = Running;
					rotationCount = 0;
				}
				else if (currentSMIndx < solutionMovements.size() && currentStatus == Running) {
					float entireAngle = abs(solutionMovements[currentSMIndx].second);
					float deltaAngle = entireAngle * (90.f / entireAngle) * animationSpeed * deltaTime;
					rotationCount += min(deltaAngle, abs(solutionMovements[currentSMIndx].second) - rotationCount);
					deltaAngle *= glm::sign(solutionMovements[currentSMIndx].second);
					for (auto& selectedCubie : currentGroup)
						selectedCubie->rotateAround(deltaAngle, currentCenterVector, currentAxisRot);
					if (rotationCount >= abs(solutionMovements[currentSMIndx].second)) {
						currentStatus = Paused;
						++currentSMIndx;
						for (auto& selectedCubie : currentGroup) {
							selectedCubie->updateMembership(currentAxisRot);
							selectedCubie->setAnimatedEnable(false, 0.f);
						}
					}
				}
				else {
					std::vector<std::pair<char, int>> wrongCentersSet;
					if (fixerHeuristicRequired(wrongCentersSet) && actionStatus == Solving) {
						std::cout << "\n[WARNING]: Some centers are not orientated correctly -> PROCEED TO FIX...\n";
						std::cout << "\nWRONG CENTERS\n";
						for (auto& cubie : wrongCentersSet) {
							std::cout << cubie.first << "->" << cubie.second << std::endl;
						}
						std::cout << "\n-------------\n";
						solutionMovements.clear();
						/*Comentar las 2 lineas siguientes si no quieres que se ejecute visualmente el fixer*/
						solution = tool::generateFixerSequence(wrongCentersSet);
						solutionMovements = eng::parseSolverOutput(solution);
						currentStatus = Paused;
						currentSMIndx = 0;
						actionStatus = Fixing;
					}
					else {
						actionStatus = Idle;
						currentStatus = Paused;
						solutionMovements.clear();
						currentSMIndx = 0;
						std::cout << std::endl;
					}
				}
			}
		}

	public:
		void init() {
			cubeShader.loadFromMemory(tool::defaultVertexShaderCode, tool::defaultFragmentShaderCode);
			globalProposalControl = std::make_shared<bool>(false);
			mouseX = mouseY = 0.f;
			currentStatus = Paused;
			actionStatus = Idle;
			std::array<bool, 6> centersFixFlags;
			std::ifstream settingsFile("res/faces.conf");
			if (settingsFile.is_open()) {
				std::string path;
				for (std::size_t idx = 0; idx < 6; ++idx) {
					std::getline(settingsFile, path);
					centersFixFlags[idx] = (path == "--REQUIRES-FIX--") ? true : false;
					for (std::size_t i = 0; i < 3; ++i) {
						for (std::size_t j = 0; j < 3; ++j) {
							std::getline(settingsFile, path);
							TEXTURE_STORAGE[idx][i][j].loadImageFromFile(path.c_str());
						}
					}
					if(idx != 5)
						std::getline(settingsFile, path);//separator
				}
				settingsFile.close();
			}
			else {
				std::cerr << "[ERROR]: File of settings does not exist.\n";
				exit(EXIT_FAILURE);
			}
			TEX_HIDDEN.loadImageFromFile("res/hidden.jpg");

			std::size_t cIdx = 0;
			for (float x = -1.f; x < 2.f; x += 1.f) {
				for (float y = -1.f; y < 2.f; y += 1.f) {
					for (float z = -1.f; z < 2.f; z += 1.f) {
						if (x != 0.f || y != 0.f || z != 0.f) {
							body.emplace_back(TEX_HIDDEN, TEX_HIDDEN, TEX_HIDDEN, TEX_HIDDEN, TEX_HIDDEN, TEX_HIDDEN);
							if (x == 1.f) {
								body.back().setRightTex(TEXTURE_STORAGE[0][int(y) + 1][int(z) + 1]);
								if (glm::abs(x) + glm::abs(y) + glm::abs(z) == 1.f) {
									body.back().setFixRequired(centersFixFlags[0]);
								}
							}
							else if (x == -1.f) {
								body.back().setLeftTex(TEXTURE_STORAGE[1][int(y) + 1][int(z) + 1]);
								if (glm::abs(x) + glm::abs(y) + glm::abs(z) == 1.f) {
									body.back().setFixRequired(centersFixFlags[1]);
								}
							}
							if (y == 1.f) {
								body.back().setUpTex(TEXTURE_STORAGE[2][int(x) + 1][int(z) + 1]);
								if (glm::abs(x) + glm::abs(y) + glm::abs(z) == 1.f) {
									body.back().setFixRequired(centersFixFlags[2]);
								}
							}
							else if (y == -1.f) {
								body.back().setDownTex(TEXTURE_STORAGE[3][int(x) + 1][int(z) + 1]);
								if (glm::abs(x) + glm::abs(y) + glm::abs(z) == 1.f) {
									body.back().setFixRequired(centersFixFlags[3]);
								}
							}
							if (z == 1.f) {
								body.back().setFrontTex(TEXTURE_STORAGE[4][int(x) + 1][int(y) + 1]);
								if (glm::abs(x) + glm::abs(y) + glm::abs(z) == 1.f) {
									body.back().setFixRequired(centersFixFlags[4]);
								}
							}
							else if (z == -1.f) {
								body.back().setBackTex(TEXTURE_STORAGE[5][int(x) + 1][int(y) + 1]);
								if (glm::abs(x) + glm::abs(y) + glm::abs(z) == 1.f) {
									body.back().setFixRequired(centersFixFlags[5]);
								}
							}
							body.back().translate(glm::vec3(x, y, z));
							body.back().setProposalEnabled(globalProposalControl);
						}
					}
				}
			}
		}

		void onUserController(GLFWwindow* window, const float& deltaTime) {
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);
			float cameraSpeed = 30 * deltaTime;
			camera.rotateAxis_X(0);
			camera.rotateAxis_Y(0);
			camera.rotateAxis_Z(0);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				*globalProposalControl = true;
			else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
				*globalProposalControl = false;
			if (glfwGetKey(window, GLFW_KEY_KP_1) == GLFW_PRESS) {
				mayusEnabled = true;
				std::cout << "[SPAM]: Uppercase movements enabled.\n";
			}
			else if(glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS) {
				mayusEnabled = false;
				std::cout << "[SPAM]: Lowercase movements enabled.\n";
			}
			if (actionStatus == Idle) {
				if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
					findRubikSolution();
				else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
					shuffleRubikRandom();
				else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
					customizeMoves('f');
				else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
					customizeMoves('b');
				else if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
					customizeMoves('l');
				else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
					customizeMoves('r');
				else if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
					customizeMoves('u');
				else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
					customizeMoves('d');
			}
			if (glfwGetKey(window, GLFW_KEY_KP_4) == GLFW_PRESS)
				camera.rotateAxis_X(-cameraSpeed);
			else if (glfwGetKey(window, GLFW_KEY_KP_6) == GLFW_PRESS)
				camera.rotateAxis_X(cameraSpeed);
			if (glfwGetKey(window, GLFW_KEY_KP_8) == GLFW_PRESS)
				camera.rotateAxis_Y(-cameraSpeed);
			else if (glfwGetKey(window, GLFW_KEY_KP_5) == GLFW_PRESS)
				camera.rotateAxis_Y(cameraSpeed);
			if (glfwGetKey(window, GLFW_KEY_KP_7) == GLFW_PRESS)
				camera.rotateAxis_Z(-cameraSpeed);
			else if (glfwGetKey(window, GLFW_KEY_KP_9) == GLFW_PRESS)
				camera.rotateAxis_Z(cameraSpeed);
			if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
				cameraZoom += cameraSpeed * 0.5;
			else if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
				cameraZoom -= cameraSpeed * 0.5;
			if (cameraZoom < CAMERA_LIMIT_MIN)
				cameraZoom = CAMERA_LIMIT_MIN;
			else if (cameraZoom > CAMERA_LIMIT_MAX)
				cameraZoom = CAMERA_LIMIT_MAX;
			if (actionStatus != Idle)
				execAnimations(deltaTime);
			if (actionStatus == Solving || actionStatus == Fixing)
				camera.animateCamera(glfwGetTime() * 0.45);
		}

		void draw(float deltaTime) {
			cubeShader.setMat4("view", camera.getView());
			projection = glm::ortho(-5.f - cameraZoom, 5.f + cameraZoom, -5.f - cameraZoom, 5.f + cameraZoom, -1000.f, 1000.f);
			cubeShader.setMat4("projection", projection);
			float variant = glm::cos(glfwGetTime()) * 0.3 + 0.65;
			cubeShader.setVec3("lightColor", variant, variant, variant);
			//cubeShader.setVec3("lightColor", -1, -1, -1);
			cubeShader.setVec3("lightPos", camera.getGlobalPosition());
			cubeShader.setVec3("viewPos", camera.getGlobalPosition());
			for (auto& cubie : body) {
				cubie.draw(cubeShader);
			}
		}

		const glm::mat4& getProjectionMatrix() {
			return projection;
		}

		const glm::mat4& getViewMatrix() {
			return camera.getView();
		}
	};

	void mouseEventManager(GLFWwindow* window, int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			glfwGetCursorPos(window, &mouseX, &mouseY);
			moving = true;
		}
		else {
			moving = false;
		}
	}

	class GL3D_WindowApplication {
	private:
		GLFWwindow* screen;
		RubikCube3D cubeMaster;
		eng::SkyBox3D background;
		tool::Texture skyboxTexture;
		int32_t screenPosX, screenPosY, lastScreenPosX, lastScreenPosY;
	public:
		GL3D_WindowApplication(const uint32_t& w, const uint32_t& h, char* title);
		void start();
	};

	GL3D_WindowApplication::GL3D_WindowApplication(const uint32_t& w, const uint32_t& h, char* title) {
		screen = eng::get3DWindow(w, h, title);
		glfwSetMouseButtonCallback(screen, mouseEventManager);
		/*skyboxTexture.loadSkyBoxFromFile({ "res/Skybox/right.jpg","res/Skybox/left.jpg" ,"res/Skybox/top.jpg" ,
																			"res/Skybox/bottom.jpg" ,"res/Skybox/front.jpg" ,"res/Skybox/back.jpg" });*/
		//skyboxTexture.loadSkyBoxFromFile({ "res/red.jpg","res/green.jpg" ,"res/blue.jpg" ,
		//																	"res/orange.jpg" ,"res/yellow.jpg" ,"res/white.jpg" });
		//background.init(skyboxTexture);
		cubeMaster.init();
		glfwGetWindowPos(screen, &lastScreenPosX, &lastScreenPosY);
		system("cls");
	}

	void GL3D_WindowApplication::start() {
		float lastFrame = 0.f;
		float deltaTime = 0.f;
		glfwSetWindowOpacity(screen, 1.f);
		while (!glfwWindowShouldClose(screen)) {
			glClearColor(0.1f, 0.15f, 0.4f, 1.0f);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			float currentFrame = glfwGetTime();
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;
			glfwGetWindowPos(screen, &screenPosX, &screenPosY);
			if (screenPosX == lastScreenPosX && screenPosY == lastScreenPosY) {
				cubeMaster.onUserController(screen, deltaTime);
			}
			else {
				lastScreenPosX = screenPosX;
				lastScreenPosY = screenPosY;
			}
			//background.draw(cubeMaster.getViewMatrix(), cubeMaster.getProjectionMatrix());
			cubeMaster.draw(deltaTime);
			glfwSwapBuffers(screen);
			glfwPollEvents();
		}
		glfwTerminate();
	}
}

#endif//APPLICATION_HPP_