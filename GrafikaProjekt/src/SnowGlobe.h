#pragma once
#include "Render_Utils.h"

class SnowGlobe {
private:
	Core::RenderContext snowGlobeContext;

public:
	void init();
	void draw();
};