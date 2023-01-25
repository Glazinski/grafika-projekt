#pragma once
#include "Render_Utils.h"

class Doors {
private:
	Core::RenderContext frameContext;
	Core::RenderContext locksetHandleContext;
	Core::RenderContext panelContext;

public:
	void init();
	void draw();
};