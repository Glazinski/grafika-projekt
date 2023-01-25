#pragma once
#include "Render_Utils.h"

class Bed {
private:
	Core::RenderContext frameContext;
	Core::RenderContext blanketContext;
	Core::RenderContext mattressContext;
	Core::RenderContext legsContext;

public:
	void init();
	void draw();
};