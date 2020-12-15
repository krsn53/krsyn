#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "raygui.h"
#undef RAYGUI_IMPLEMENTATION
#undef RAYGUI_SUPPORT_ICONS

#include "raygui_impl.h"

#define GUI_FILE_DIALOG_IMPLEMENTATION
#include "gui_file_dialog.h"



#include "raylib.h"
#include "raymath.h"


static Vector2 dragBeginPos = {-1.0f, 1.0f};


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
                value += (mouse.x - dragBeginPos.x)* step;
                SetMousePosition(dragBeginPos.x, dragBeginPos.y);

                *state = GUI_STATE_PRESSED;
            }

            if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
                dragBeginPos = (Vector2) { -1.0f, -1.0f};
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

void GuiListViewGetInfo(Rectangle bounds, int count, int scrollIndex, int *startIndex, int *endIndex){
    // Get items on the list
    int visibleItems = bounds.height/(GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_PADDING));
    if (visibleItems > count) visibleItems = count;

    *startIndex = scrollIndex;
    if ((*startIndex < 0) || (*startIndex > (count - visibleItems))) *startIndex = 0;
    *endIndex = *startIndex + visibleItems;
}

int GuiListViewEx2(Rectangle bounds, const char **text, int count, int *focus, int *scrollIndex, int active)
{
    GuiControlState state = guiState;
    int itemFocused = (focus == NULL)? -1 : *focus;
    int itemSelected = active;

    // Check if we need a scroll bar
    bool useScrollBar = false;
    if ((GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_PADDING))*count > bounds.height) useScrollBar = true;

    // Define base item rectangle [0]
    Rectangle itemBounds = { 0 };
    itemBounds.x = bounds.x + GuiGetStyle(LISTVIEW, LIST_ITEMS_PADDING);
    itemBounds.y = bounds.y + GuiGetStyle(LISTVIEW, LIST_ITEMS_PADDING) + GuiGetStyle(DEFAULT, BORDER_WIDTH);
    itemBounds.width = bounds.width - 2*GuiGetStyle(LISTVIEW, LIST_ITEMS_PADDING) - GuiGetStyle(DEFAULT, BORDER_WIDTH);
    itemBounds.height = GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT);
    if (useScrollBar) itemBounds.width -= GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH);

    // Get items on the list
    int visibleItems = bounds.height/(GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_PADDING));
    if (visibleItems > count) visibleItems = count;

    int startIndex = (scrollIndex == NULL)? 0 : *scrollIndex;
    if ((startIndex < 0) || (startIndex > (count - visibleItems))) startIndex = 0;
    int endIndex = startIndex + visibleItems;

    // Update control
    //--------------------------------------------------------------------
    if ((state != GUI_STATE_DISABLED) && !guiLocked)
    {
        Vector2 mousePoint = GetMousePosition();

        // Check mouse inside list view
        if (CheckCollisionPointRec(mousePoint, bounds))
        {
            state = GUI_STATE_FOCUSED;

            // Check focused and selected item
            for (int i = 0; i < visibleItems; i++)
            {
                if (CheckCollisionPointRec(mousePoint, itemBounds))
                {
                    itemFocused = startIndex + i;
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    {
                        if (itemSelected == (startIndex + i)) itemSelected = -1;
                        else itemSelected = startIndex + i;
                    }
                    break;
                }

                // Update item rectangle y position for next item
                itemBounds.y += (GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_PADDING));
            }

            if (useScrollBar)
            {
                int wheelMove = GetMouseWheelMove();
                startIndex -= wheelMove;

                if (startIndex < 0) startIndex = 0;
                else if (startIndex > (count - visibleItems)) startIndex = count - visibleItems;

                endIndex = startIndex + visibleItems;
                if (endIndex > count) endIndex = count;
            }
        }
        else itemFocused = -1;

        // Reset item rectangle y to [0]
        itemBounds.y = bounds.y + GuiGetStyle(LISTVIEW, LIST_ITEMS_PADDING) + GuiGetStyle(DEFAULT, BORDER_WIDTH);
    }
    //--------------------------------------------------------------------

    // Draw control
    //--------------------------------------------------------------------
    GuiDrawRectangle(bounds, GuiGetStyle(DEFAULT, BORDER_WIDTH), Fade(GetColor(GuiGetStyle(LISTVIEW, BORDER + state*3)), guiAlpha), GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));     // Draw background

    // Draw visible items
    for (int i = 0; ((i < visibleItems) && (text != NULL)); i++)
    {
        if (state == GUI_STATE_DISABLED)
        {
            if ((startIndex + i) == itemSelected) GuiDrawRectangle(itemBounds, GuiGetStyle(LISTVIEW, BORDER_WIDTH), Fade(GetColor(GuiGetStyle(LISTVIEW, BORDER_COLOR_DISABLED)), guiAlpha), Fade(GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_DISABLED)), guiAlpha));

            GuiDrawText(text[startIndex + i], GetTextBounds(DEFAULT, itemBounds), GuiGetStyle(LISTVIEW, TEXT_ALIGNMENT), Fade(GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_DISABLED)), guiAlpha));
        }
        else
        {
            if ((startIndex + i) == itemSelected)
            {
                // Draw item selected
                GuiDrawRectangle(itemBounds, GuiGetStyle(LISTVIEW, BORDER_WIDTH), Fade(GetColor(GuiGetStyle(LISTVIEW, BORDER_COLOR_PRESSED)), guiAlpha), Fade(GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_PRESSED)), guiAlpha));
                GuiDrawText(text[startIndex + i], GetTextBounds(DEFAULT, itemBounds), GuiGetStyle(LISTVIEW, TEXT_ALIGNMENT), Fade(GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_PRESSED)), guiAlpha));
            }
            else if ((startIndex + i) == itemFocused)
            {
                // Draw item focused
                GuiDrawRectangle(itemBounds, GuiGetStyle(LISTVIEW, BORDER_WIDTH), Fade(GetColor(GuiGetStyle(LISTVIEW, BORDER_COLOR_FOCUSED)), guiAlpha), Fade(GetColor(GuiGetStyle(LISTVIEW, BASE_COLOR_FOCUSED)), guiAlpha));
                GuiDrawText(text[startIndex + i], GetTextBounds(DEFAULT, itemBounds), GuiGetStyle(LISTVIEW, TEXT_ALIGNMENT), Fade(GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_FOCUSED)), guiAlpha));
            }
            else
            {
                // Draw item normal
                GuiDrawText(text[startIndex + i], GetTextBounds(DEFAULT, itemBounds), GuiGetStyle(LISTVIEW, TEXT_ALIGNMENT), Fade(GetColor(GuiGetStyle(LISTVIEW, TEXT_COLOR_NORMAL)), guiAlpha));
            }
        }

        // Update item rectangle y position for next item
        itemBounds.y += (GuiGetStyle(LISTVIEW, LIST_ITEMS_HEIGHT) + GuiGetStyle(LISTVIEW, LIST_ITEMS_PADDING));
    }

    if (useScrollBar)
    {
        Rectangle scrollBarBounds = {
            bounds.x + bounds.width - GuiGetStyle(LISTVIEW, BORDER_WIDTH) - GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH),
            bounds.y + GuiGetStyle(LISTVIEW, BORDER_WIDTH), (float)GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH),
            bounds.height - 2*GuiGetStyle(DEFAULT, BORDER_WIDTH)
        };

        // Calculate percentage of visible items and apply same percentage to scrollbar
        float percentVisible = (float)(endIndex - startIndex)/count;
        float sliderSize = bounds.height*percentVisible;

        int prevSliderSize = GuiGetStyle(SCROLLBAR, SCROLL_SLIDER_SIZE);   // Save default slider size
        int prevScrollSpeed = GuiGetStyle(SCROLLBAR, SCROLL_SPEED); // Save default scroll speed
        GuiSetStyle(SCROLLBAR, SCROLL_SLIDER_SIZE, sliderSize);            // Change slider size
        GuiSetStyle(SCROLLBAR, SCROLL_SPEED, count - visibleItems); // Change scroll speed

        startIndex = GuiScrollBar(scrollBarBounds, startIndex, 0, count - visibleItems);

        GuiSetStyle(SCROLLBAR, SCROLL_SPEED, prevScrollSpeed); // Reset scroll speed to default
        GuiSetStyle(SCROLLBAR, SCROLL_SLIDER_SIZE, prevSliderSize); // Reset slider size to default
    }
    //--------------------------------------------------------------------

    if (focus != NULL) *focus = itemFocused;
    if (scrollIndex != NULL) *scrollIndex = startIndex;

    return itemSelected;
}
