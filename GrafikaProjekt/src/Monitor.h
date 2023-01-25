#pragma once
#include "Render_Utils.h"

class Monitor {
private:
	Core::RenderContext monitorContext;

public:
	void init();
	void draw();
};