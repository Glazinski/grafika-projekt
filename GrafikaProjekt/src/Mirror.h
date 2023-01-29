#pragma once
#include "Render_Utils.h"

class Mirror {
private:
	Core::RenderContext frameContext;
	Core::RenderContext glassContext;

public:
	void init();
	void draw();
};