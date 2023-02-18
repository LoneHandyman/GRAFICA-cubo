#include "RubikSolverPocket/RubikSolver.h"
#include "App/Application.hpp"
#include <sstream>

int main() {
  eng::initOpenGL();
  app::GL3D_WindowApplication api(SCREEN_SIZE_X, SCREEN_SIZE_Y, "RUSOSI");
  api.start();
	return 0;
}