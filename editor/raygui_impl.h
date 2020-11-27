#pragma once

#include "raygui.h"
#include "gui_file_dialog.h"
#include "../krsyn.h"

#define RANGE(value, min, max) MIN(max, MAX(min, value));

int GetLineWidth();
Color GetTextColorDefault();
void GuiAlignedLabel(const char* text, Rectangle rec, GuiTextAlignment align);
int PropertyInt(Rectangle rec, const char* innerText, int value, int min_value, int max_value, int step);
int PropertyIntImage(Rectangle rec, Texture2D tex, int value, int min_value, int max_value, int step);
