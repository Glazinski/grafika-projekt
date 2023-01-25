#pragma once
#include "Render_Utils.h"

class Table {
private:
	Core::RenderContext tableContext;

public:
	void init();
	void draw();
};