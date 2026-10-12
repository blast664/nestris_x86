#include <cstring>

#include "nestris_x86.hpp"

int main(const int argc, const char** argv)
{
  bool full_screen = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--fullscreen") == 0 || std::strcmp(argv[i], "-f") == 0) {
      full_screen = true;
    }
  }

  nestris_x86::NestrisX86 nestetris{};
	if (nestetris.Construct(256, 225, 4, 4, full_screen))
  {
    nestetris.Start();
  }
	return 0;
}
