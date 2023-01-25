#pragma once
#include "Render_Utils.h"

class Room {
private:
	Core::RenderContext roomContext;
	Core::RenderContext floorContext;

public:
	void init();
	void draw();
};