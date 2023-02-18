#ifndef TOOLS_HPP_
#define TOOLS_HPP_

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "MemShader.hpp"
#include "Tools/imageLoader.h"

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#define SCREEN_SIZE_X 900.f
#define SCREEN_SIZE_Y 800.f

namespace tool {

	int32_t Mod(int32_t num, int32_t mod) {
		return (num < 0) ? (num - ((num / mod) - 1) * mod) : num - (num / mod) * mod;
	}

	class Texture {
	private:
		GLuint textureID;
	public:
		void loadImageFromFile(const char* fileName);
		void loadSkyBoxFromFile(const std::vector<std::string>& fileNames);
		void apply();
		void skyboxModeApply();
	};

	void Texture::loadSkyBoxFromFile(const std::vector<std::string>& fileNames) {
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
		int32_t width, height, nrChannels;
		for (uint32_t i = 0; i < fileNames.size(); i++) {
			uint8_t* data = stbi_load(fileNames[i].c_str(), &width, &height, &nrChannels, 0);
			if (data) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				stbi_image_free(data);
				std::cout << "Cubemap texture: " << fileNames[i] << " loaded successfully." << std::endl;
			}
			else {
				std::cerr << "Cubemap texture failed to load at path: " << fileNames[i] << std::endl;
				stbi_image_free(data);
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	void Texture::loadImageFromFile(const char* fileName) {
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		int32_t width, height, nrChannels;
		stbi_set_flip_vertically_on_load(true);
		uint8_t* data = stbi_load(fileName, &width, &height, &nrChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else {
			std::cerr << "Failed to load texture" << std::endl;
			stbi_image_free(data);
			return;
		}
		std::cout << fileName << " loaded successfully.\n";
	}

	void Texture::apply() {
		glBindTexture(GL_TEXTURE_2D, textureID);
	}

	void Texture::skyboxModeApply() {
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	}

	void randomShuffle(std::vector<char>& steps, const int& iterations) {
		steps.clear();
		char indexes[6] = { 'R','L','F','B','U','D' };
		std::map<char, std::array<char, 10>> movements = {{'R',{'L','l','F','f','B','b','U','u','D','d'}},
																											{'L',{'R','r','F','f','B','b','U','u','D','d'}},
																											{'F',{'R','r','L','l','B','b','U','u','D','d'}},
																											{'B',{'R','r','L','l','F','f','U','u','D','d'}},
																											{'U',{'R','r','L','l','F','f','B','b','D','d'}},
																											{'D',{'R','r','L','l','F','f','B','b','U','u'}} };
		char movIdx = indexes[rand() % 6];
		for (int iter = 0; iter < iterations; ++iter)
			steps.push_back(movIdx = movements[std::toupper(movIdx)][rand() % 10]);
	}

	void fixSequenceLogic(std::vector<char>& sequence) {
		if (!sequence.empty()) {
			char lastMovement = sequence[0];
			uint8_t matching = 1;
			std::vector<char> fixedSequence = {lastMovement};
			for (std::size_t idx = 1; idx < sequence.size(); ++idx) {
				if (sequence[idx] != lastMovement) {
					matching = 1;
					if (std::tolower(sequence[idx]) == std::tolower(lastMovement)) {
						fixedSequence.pop_back();
						if (fixedSequence.empty())
							lastMovement = 'z';
						else
							lastMovement = fixedSequence.back();
					}
					else {
						fixedSequence.push_back(sequence[idx]);
						lastMovement = fixedSequence.back();
					}
				}
				else {
					++matching;
					if (matching == 3) {
						fixedSequence.pop_back();
						fixedSequence.pop_back();
						fixedSequence.push_back((std::isupper(sequence[idx])) ? std::tolower(sequence[idx]) : std::toupper(sequence[idx]));
						lastMovement = fixedSequence.back();
						matching = 1;
					}
					else {
						fixedSequence.push_back(sequence[idx]);
						lastMovement = fixedSequence.back();
					}
				}
			}
			sequence = fixedSequence;
		}
	}

	void concatenateToSequence(std::vector<char>& sequence, const std::string& pattern, const uint8_t& times) {
		for (uint8_t i = 1; i <= times; ++i)
			for (auto c : pattern)
				sequence.push_back(c);
	}

	std::vector<char> generateFixerSequence(std::vector<std::pair<char, int>>& wrongCenters) {
		std::map<char, std::string> fixerMovements90 = { {'r',"fBrFbD"},{'l',"fBLFbd"},{'f',"lRfLrD"},
																										 {'b',"LrBlRd"},{'u',"LrulRB"},{'d',"fBdFbL"} };
			
		std::map<char, std::string> fixerMovements180 = { {'r',"BRbR"},{'l',"FlFl"},{'f',"RFrF"},
																											{'b',"LBlB"},{'u',"LUlU"},{'d',"RDrD"} };
		std::vector<char> fixerSequence;
		for (auto& item : wrongCenters) {
			if (item.second == 90.f) {//90
				concatenateToSequence(fixerSequence, fixerMovements90[item.first], 15);
			}
			else if (item.second == 180.f) {//180
				concatenateToSequence(fixerSequence, fixerMovements180[item.first], 5);
			}
			else {//180 + 90
				concatenateToSequence(fixerSequence, fixerMovements180[item.first], 5);
				concatenateToSequence(fixerSequence, fixerMovements90[item.first], 15);
			}
		}
		return fixerSequence;
	}
	//RUrU
	//LrulRB
}
#endif //TOOLS_HPP_
