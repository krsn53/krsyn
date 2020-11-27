#include "raygui_impl.h"

#include "raylib.h"
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "raygui.h"

#define GUI_FILE_DIALOG_IMPLEMENTATION
#include "gui_file_dialog.h"

static Vector2 dragBeginPos = {-1.0f, 1.0f};
static Vector2 prePos = {-1.0f, 1.0f};


int GetLineWidth(){
    return GuiGetStyle(DEFAULT,TEXT_SIZE) + GuiGetStyle(SLIDER, BORDER_WIDTH)*2;
}

Color GetTextColorDefault(){
    return Fade(GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)), guiAlpha);
}

static inline int UpdateProperty(Rectangle rec, int value, int min_value, int max_value, int step, GuiControlState* state, bool is_enum){
    if ((*state != GUI_STATE_DISABLED) && !guiLocked){
        if(IsKeyDown(KEY_LEFT_CONTROL)) {
            step *= 10;
        }

        Vector2 mouse = GetMousePosition();
        if(dragBeginPos.x < 0 && CheckCollisionPointRec(mouse, rec)){
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                if(is_enum){
                    value+=step;
                } else {
                    dragBeginPos = mouse;
                    prePos = dragBeginPos;
                    *state = GUI_STATE_PRESSED;
                    DisableCursor();
                }
            } if(is_enum && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
                value += (max_value - min_value + 1) - step;
            }
            else {
                *state = GUI_STATE_FOCUSED;
            }

            float wheel = GetMouseWheelMove();
            if(wheel != 0.0f){
                value -= wheel*step;
            }

            if(IsKeyPressed(KEY_LEFT)){
                value -= step;
            }

            if(IsKeyPressed(KEY_RIGHT)){
                value += step;

            }
        }

        if(!is_enum){
            if(CheckCollisionPointRec(dragBeginPos, rec) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
                value += (mouse.x - prePos.x)* step;
                prePos = mouse;

                *state = GUI_STATE_PRESSED;
            }

            if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
                dragBeginPos = (Vector2) { -1.0f, -1.0f};
                prePos = (Vector2) {-1.0f, -1.0f};
                EnableCursor();
            }
        }

        if(is_enum){
            value =(value - min_value)%(max_value - min_value + 1) +min_value;
        } else {
            value = RANGE(value, min_value, max_value);
        }
    }

    return value;
}


int PropertyInt(Rectangle rec, const char* innerText, int value, int min_value, int max_value, int step){
    rec.width = MAX(rec.width, GuiGetStyle(DEFAULT,TEXT_SIZE)*3 + GuiGetStyle(SLIDER, BORDER_WIDTH)*3*2);
    rec.height = MAX(rec.height, GetLineWidth());


    Rectangle progress = { rec.x + GuiGetStyle(SLIDER, BORDER_WIDTH),
                           rec.y + GuiGetStyle(SLIDER, BORDER_WIDTH) + GuiGetStyle(SLIDER, SLIDER_PADDING), 0,
                           rec.height - 2*GuiGetStyle(SLIDER, BORDER_WIDTH) - 2*GuiGetStyle(SLIDER, SLIDER_PADDING) };

    GuiControlState state = guiState;
    value  = UpdateProperty(rec, value, min_value, max_value, step, &state, false);

    if (state != GUI_STATE_DISABLED) progress.width = ((float)value/(float)(max_value - min_value)*(float)(rec.width - 2*GuiGetStyle(SLIDER, BORDER_WIDTH)));


    GuiDrawRectangle(rec, GuiGetStyle(SLIDER, BORDER_WIDTH),
                     Fade(GetColor(GuiGetStyle(SLIDER, BORDER + (state*3))), guiAlpha),
                     Fade(GetColor(GuiGetStyle(SLIDER, BASE_COLOR_NORMAL)), guiAlpha));

    if ((state == GUI_STATE_NORMAL) || (state == GUI_STATE_PRESSED)) GuiDrawRectangle(progress, 0, BLANK, Fade(GetColor(GuiGetStyle(SLIDER, BASE_COLOR_PRESSED)), guiAlpha));
        else if (state == GUI_STATE_FOCUSED) GuiDrawRectangle(progress, 0, BLANK, Fade(GetColor(GuiGetStyle(SLIDER, TEXT_COLOR_FOCUSED)), guiAlpha));

    GuiAlignedLabel(innerText, rec, GUI_TEXT_ALIGN_CENTER);

    return value;
}

int PropertyIntImage(Rectangle rec, Texture2D tex, int value, int min_value, int max_value, int step){
    rec.width = MAX(rec.width, GuiGetStyle(DEFAULT,TEXT_SIZE)*3 + GuiGetStyle(SLIDER, BORDER_WIDTH)*3*2);
    rec.height = MAX(rec.height, GetLineWidth());

    GuiControlState state = guiState;
    value  = UpdateProperty(rec, value, min_value, max_value, step, &state, true);

    Rectangle src ={0.0f, 0.0f, tex.width / ((max_value - min_value + 1) / step), tex.height};
    src.x = (value - min_value) / step * tex.width / ((max_value - min_value + 1) / step);

    src.width = MIN(rec.width, src.width);
    src.height = MIN(rec.height, src.height);

    Color tex_color;
    if ((state == GUI_STATE_NORMAL)|| (state == GUI_STATE_PRESSED)) tex_color = WHITE;
    else tex_color = GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_FOCUSED));

    Rectangle dest = (Rectangle){rec.x + rec.width/2.0f, rec.y + rec.height/2.0f, src.width, src.height};
    DrawTexturePro(tex, src, dest,
                   (Vector2){src.width/2.0f, src.height/2.0f}, 0.0f, Fade(tex_color ,guiAlpha));
    if(state == GUI_STATE_DISABLED){
        DrawRectangleRec(rec, (Color){200,200,200, 128});
    }

    GuiDrawRectangle(rec, GuiGetStyle(SLIDER, BORDER_WIDTH),
                     Fade(GetColor(GuiGetStyle(SLIDER, BORDER + (state*3))), guiAlpha),
                     BLANK);

    return value;
}

void GuiAlignedLabel(const char* text, Rectangle rec, GuiTextAlignment align){
    Color color;
    if ((guiState == GUI_STATE_NORMAL)|| (guiState== GUI_STATE_PRESSED)) color = GetTextColorDefault();
    else color = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED));
    GuiDrawText(text, rec, align, color);
}
