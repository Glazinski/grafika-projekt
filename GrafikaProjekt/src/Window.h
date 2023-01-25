#pragma once
#include "Render_Utils.h"

class Window {
private:
	Core::RenderContext windowContext1;
	Core::RenderContext windowContext2;
	Core::RenderContext windowContext3;

public:
	void init();
	void draw();
};