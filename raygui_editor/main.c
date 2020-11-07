#include "raylib.h"
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "raygui.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define RANGE(value, min, max) MIN(max, MAX(min, value));

#define SAMPLING_RATE               48000
#define MAX_SAMPLES_PER_UPDATE      4096
#define NUM_CHANNELS                1

#include "../krsyn.h"

int GetLineWidth();
Color GetTextColorDefault();
void GuiAlignedLabel(const char* text, Rectangle rec, GuiTextAlignment align);
int PropertyInt(Rectangle rec, const char* innerText, int value, int min_value, int max_value, int step);
int PropertyIntImage(Rectangle rec, Texture2D tex, int value, int min_value, int max_value, int step);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    ks_synth_binary synth_bin;
    ks_synth synth;
    ks_synth_note note = { 0 };
    int8_t noteon_number = -1;

    ks_synth_binary_set_default(&synth_bin);

    InitAudioDevice();

    AudioStream audiostream = InitAudioStream(SAMPLING_RATE, 16, NUM_CHANNELS);
    int16_t *buf = malloc(sizeof(int16_t)*MAX_SAMPLES_PER_UPDATE* NUM_CHANNELS);
    PlayAudioStream(audiostream);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "krsyn raygui editor");
    SetTargetFPS(60);


    //--------------------------------------------------------------------------------------


    int margin = GuiGetStyle(DEFAULT, BORDER_WIDTH)*5;
    int base_width = 100;
    Rectangle base_pos = (Rectangle) { margin, margin, base_width,  GetLineWidth() };

    const char* text;

    const float line_width = GetLineWidth();
    const float step_x = base_width + margin;
    const float step = line_width+ margin;

    Texture2D algorithm_images = LoadTexture("resources/images/algorithms.png");
    Texture2D lfo_wave_images= LoadTexture("resources/images/lfo_waves.png");
    Texture2D keyscale_left_images = LoadTexture("resources/images/keyscale_curves_l.png");
    Texture2D keyscale_right_images =  LoadTexture("resources/images/keyscale_curves_r.png");

    SetTextureFilter(algorithm_images, FILTER_BILINEAR);
    SetTextureFilter(lfo_wave_images, FILTER_BILINEAR);
    SetTextureFilter(keyscale_left_images, FILTER_BILINEAR);
    SetTextureFilter(keyscale_right_images, FILTER_BILINEAR);
    
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        if(IsAudioStreamProcessed(audiostream)){
            ks_synth_render(&synth, &note, ks_1(KS_LFO_DEPTH_BITS), buf, MAX_SAMPLES_PER_UPDATE);
            UpdateAudioStream(audiostream, buf, MAX_SAMPLES_PER_UPDATE*NUM_CHANNELS);
        }
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            Rectangle x_pos = base_pos;
            Rectangle pos = base_pos;
            Rectangle pos2;

            {
                pos2 = pos;
                pos2.height *= 2;

            }

            x_pos.x += step_x*2;
            pos.x = x_pos.x;
            pos.y += step / 2.0;


            // common params
            {

                // algorithm
                {
                    pos2 = pos;
                    pos2.height = step * 4;
                    GuiAlignedLabel("Algorithm", pos2, GUI_TEXT_ALIGN_RIGHT);
                    pos2.x += step_x;
                    synth_bin.algorithm = PropertyIntImage(pos2, algorithm_images, synth_bin.algorithm, 0, 10, 1);
                }
                pos.x = x_pos.x;
                pos.y += pos2.height + margin;
                // feedback_level
                {
                    GuiAlignedLabel("Feedback", pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    text = FormatText("%.3f PI", 2.0f *calc_feedback_level(synth_bin.feedback_level) / ks_1(KS_FEEDBACK_LEVEL_BITS));
                    synth_bin.feedback_level = PropertyInt(pos, text, synth_bin.feedback_level, 0, 255, 1);
                }
                pos.x = x_pos.x;
                pos.y += step;
                // lfo_wave_type
                {
                    pos2 = pos;
                    pos2.height = pos.height*2;
                    GuiAlignedLabel("LFO Wave", pos2, GUI_TEXT_ALIGN_RIGHT);
                    pos2.x += step_x;


                    synth_bin.lfo_wave_type = PropertyIntImage(pos2, lfo_wave_images, synth_bin.lfo_wave_type, 0, KS_LFO_NUM_WAVES-1, 1);

                }
                pos.x = x_pos.x;
                pos.y += pos2.height + margin;
                // lfo_freq
                {
                    GuiAlignedLabel("LFO Freqency", pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    float hz = calc_lfo_freq(synth_bin.lfo_freq) / (float)ks_1(KS_FREQUENCY_BITS);
                    if(hz >= 1.0f){
                        text = FormatText("%.1f Hz", hz);
                    }
                    else if(hz< 1.0f && hz>= 0.01f){
                        text = FormatText("%.1f mHz", hz*1000.0f);
                    }
                    else {
                        text = FormatText("%.3f mHz", hz*1000.0f);
                    }


                    synth_bin.lfo_freq = PropertyInt(pos, text, synth_bin.lfo_freq, 0, 255, 1);
                }
                pos.x = x_pos.x;
                pos.y += step;
                // lfo_det
                {
                    GuiAlignedLabel("LFO Initial Phase", pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    text = FormatText("%.1f deg", 360.0f *calc_lfo_det(synth_bin.lfo_det) / (float)ks_1(KS_PHASE_MAX_BITS));
                    synth_bin.lfo_det = PropertyInt(pos, text, synth_bin.lfo_det, 0, 255, 1);
                }
                pos.x = x_pos.x;
                pos.y += step;
                // lfo_fms_depth
                {
                    GuiAlignedLabel("LFO FMS", pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    text = FormatText("%.3f %%", 100.0f *calc_lfo_fms_depth(synth_bin.lfo_fms_depth) / (float)ks_1(KS_LFO_DEPTH_BITS));
                    synth_bin.lfo_fms_depth= PropertyInt(pos, text, synth_bin.lfo_fms_depth, 0, 240, 1);
                }
                pos.x = x_pos.x;
                pos.y += step;
            }

            // wave
            {
                Rectangle wave_rec ={x_pos.x + step_x * 2 + margin, x_pos.y, step_x*3 - margin, pos.y - x_pos.y };
                DrawRectangleRec(wave_rec, DARKGRAY);

                float env_width =wave_rec.width*0.6f / 4.0f;
                float env_x = wave_rec.width*0.2f / 4.0f + wave_rec.x;
                float env_y = wave_rec.y + wave_rec.height;
                float env_step = wave_rec.width / 4.0f;
                float env_mul = wave_rec.height / ks_1(KS_ENVELOPE_BITS);

                for(unsigned i=0; i<KS_NUM_OPERATORS; i++){
                    float w = env_mul*note.envelope_now_amps[i];
                    Rectangle rec ={env_x + env_step*i, env_y - w, env_width, w};
                    DrawRectangleRec(rec, BLUE);
                    const char *text;
                    if(note.envelope_states[i] == KS_ENVELOPE_RELEASED){
                        text = "Released";
                    }
                    else if(note.envelope_states[i] == KS_ENVELOPE_OFF) {
                        text = "Off";
                    }
                    else{
                        text = FormatText(note.envelope_now_points[i] == 0 ?
                                              "Attack" : note.envelope_now_points[i] == 1 ?
                                                  "Decay" : "Sustain %d", note.envelope_now_points[i]-1);
                    }

                    DrawText(text, rec.x+ 1, env_y -  11, 10, YELLOW);
                }

                int samp = SAMPLING_RATE  * 2/ 440.0f;
                float dx = wave_rec.width / samp;
                float x = 0.0f;
                float y =  wave_rec.y + wave_rec.height/2.0f ;
                float amp = wave_rec.height /2.0f / INT16_MAX;

                for(int i=0; i<samp; i++){
                    float base_x = wave_rec.x + x;

                    DrawLineV((Vector2){base_x, y - buf[i]*amp}, (Vector2){base_x+dx, y - buf[i+1]*amp}, GREEN);
                    x+= dx;
                }
            }


            GuiGroupBox((Rectangle){pos.x, base_pos.y, step_x*2, pos.y - base_pos.y}, "Common Params");
            pos.y += step / 2.0f;
            x_pos.y = pos.y;

            // operators
            {
                pos.y += step /2.0f ;

                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    pos.x += step_x;
                    GuiAlignedLabel(FormatText("Operator %d", i+1), pos, GUI_TEXT_ALIGN_CENTER);
                }
                pos.x = x_pos.x; pos.y += step;

                // fixed frequency
                GuiAlignedLabel("Fixed Freqency", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    synth_bin.phase_coarses[i].str.fixed_frequency = GuiCheckBox((Rectangle){pos.x, pos.y, pos.height, pos.height}, "", synth_bin.phase_coarses[i].str.fixed_frequency);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;

                // phase coarse
                GuiAlignedLabel("Phase Coarse", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    if(synth_bin.phase_coarses[i].str.fixed_frequency){
                        text = FormatText("%.1f Hz", note_freq[synth_bin.phase_coarses[i].str.value] / (float)ks_1(KS_FREQUENCY_BITS));
                    }
                    else {
                        text = FormatText("%.1f", calc_phase_coarses(synth_bin.phase_coarses[i].str.value) / 2.0f);
                    }
                    synth_bin.phase_coarses[i].str.value = PropertyInt(pos, text, synth_bin.phase_coarses[i].str.value, 0, 127, 1);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;

                // phase fine
                GuiAlignedLabel("Phase Detune", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.3f", calc_phase_fines(synth_bin.phase_fines[i]) / (float)ks_v(2, KS_PHASE_FINE_BITS));
                    synth_bin.phase_fines[i] = PropertyInt(pos, text, synth_bin.phase_fines[i], 0, 255, 1);
                    pos.x += step_x;
                }
                 pos.x = x_pos.x; pos.y += step;

                // phase det
                 GuiAlignedLabel("Inital Phase", pos, GUI_TEXT_ALIGN_RIGHT);
                 pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.1f deg", 360.0*calc_phase_dets(synth_bin.phase_dets[i]) / (float)ks_1(KS_PHASE_MAX_BITS));
                    synth_bin.phase_dets[i] = PropertyInt(pos, text, synth_bin.phase_dets[i], 0, 255, 1);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;


                // envelope points and times
                for(unsigned e =0; e< KS_ENVELOPE_NUM_POINTS; e++){
                    text = FormatText( e == 0 ? "Attack" : e == 1 ? "Decay" : "Sustain %d", e-1);
                    GuiAlignedLabel(text, pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    pos2 = pos;
                    pos2.width /= 2.0f;
                    for(unsigned i=0; i< KS_NUM_OPERATORS; i++) {
                        text = FormatText("%.3f", calc_envelope_points(synth_bin.envelope_points[e][i]) / (float)ks_1(KS_ENVELOPE_BITS));
                        synth_bin.envelope_points[e][i] = PropertyInt(pos2, text, synth_bin.envelope_points[e][i], 0, 255, 1);
                        pos2.x += step_x / 2.0f;

                        float sec = krsyn_calc_envelope_times(synth_bin.envelope_times[e][i]) / (float)ks_1(16);
                        if(sec >= 1.0f){
                            text = FormatText("%.1f s", sec);
                        }
                        else if(sec< 1.0f && sec>= 0.01f){
                            text = FormatText("%.1f ms", sec*1000.0f);
                        }
                        else {
                            text = FormatText("%.3f ms", sec*1000.0f);
                        }

                        synth_bin.envelope_times[e][i] = PropertyInt(pos2, text, synth_bin.envelope_times[e][i], 0, 255, 1);
                        pos2.x += step_x / 2.0f;
                    }
                    pos.x = x_pos.x; pos.y += step;
                }

                // envelope release time
                GuiAlignedLabel("Release Time", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    float sec = krsyn_calc_envelope_times(synth_bin.envelope_release_times[i]) / (float)ks_1(16);
                    if(sec >= 1.0f){
                        text = FormatText("%.1f s", sec);
                    }
                    else if(sec< 1.0f && sec>= 0.01f){
                        text = FormatText("%.1f ms", sec*1000.0f);
                    }
                    else {
                        text = FormatText("%.3f ms", sec*1000.0f);
                    }

                    synth_bin.envelope_release_times[i] = PropertyInt(pos, text, synth_bin.envelope_release_times[i], 0, 255, 1);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;

                // rate scale
               GuiAlignedLabel("Rate Scale", pos, GUI_TEXT_ALIGN_RIGHT);
               pos.x += step_x;
               for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                   text = FormatText("%.1f %%", 100.0*calc_ratescales(synth_bin.ratescales[i]) / (float)ks_1(KS_RATESCALE_BITS));
                   synth_bin.phase_dets[i] = PropertyInt(pos, text, synth_bin.ratescales[i], 0, 255, 1);
                   pos.x += step_x;
               }
               pos.x = x_pos.x; pos.y += step;

                // velocity sensitive
                GuiAlignedLabel("Velocity Sensitive", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.1f %%", 100*calc_velocity_sens(synth_bin.velocity_sens[i]) / (float)ks_1(KS_VELOCITY_SENS_BITS));
                    synth_bin.velocity_sens[i] = PropertyInt(pos, text, synth_bin.velocity_sens[i], 0, 255, 1);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;

                // keyscale curve type
                pos2 = pos;
                pos2.height = pos.height*4;
                GuiAlignedLabel("Keyscale Curve", pos2, GUI_TEXT_ALIGN_RIGHT);
                pos2.width = (base_width - margin)/ 2.0;
                pos2.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    synth_bin.keyscale_curve_types[i].str.left = PropertyIntImage(pos2, keyscale_left_images, synth_bin.keyscale_curve_types[i].str.left, 0, KS_KEYSCALE_CURVE_NUM_TYPES-1, 1);
                    pos2.x += pos2.width  +margin;
                    synth_bin.keyscale_curve_types[i].str.right = PropertyIntImage(pos2, keyscale_right_images, synth_bin.keyscale_curve_types[i].str.right, 0, KS_KEYSCALE_CURVE_NUM_TYPES-1, 1);
                    pos2.x +=  pos2.width +margin;
                }
                pos.x = x_pos.x; pos.y += (pos2.height + margin);

                // keyscale low depth
                GuiAlignedLabel("Keyscale Depth", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;

                pos2 = pos;
                pos2.width /=2.0f;

                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.1f %%", 100*calc_keyscale_low_depths(synth_bin.keyscale_low_depths[i]) / (float)ks_1(KS_KEYSCALE_DEPTH_BITS));
                    synth_bin.keyscale_low_depths[i] = PropertyInt(pos2, text, synth_bin.keyscale_low_depths[i], 0, 255, 1);

                    pos2.x += step_x/2.0f;

                    text = FormatText("%.1f %%", 100*calc_keyscale_high_depths(synth_bin.keyscale_high_depths[i]) / (float)ks_1(KS_KEYSCALE_DEPTH_BITS));
                    synth_bin.keyscale_high_depths[i] = PropertyInt(pos2, text, synth_bin.keyscale_high_depths[i], 0, 255, 1);
                    pos2.x += step_x / 2.0f;
                }
                pos.x = x_pos.x; pos.y += step;


                // keyscale mid point
                GuiAlignedLabel("Keyscale Mid Key", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%d", calc_keyscale_mid_points(synth_bin.keyscale_mid_points[i]));
                    synth_bin.keyscale_mid_points[i] = PropertyInt(pos, text, synth_bin.keyscale_mid_points[i], 0, 127, 1);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;

                // lfo ams depth
                GuiAlignedLabel("LFO AMS", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.1f %%", 100*calc_lfo_ams_depths(synth_bin.lfo_ams_depths[i]) / (float)ks_1(KS_LFO_DEPTH_BITS));
                    synth_bin.lfo_ams_depths[i] = PropertyInt(pos, text, synth_bin.lfo_ams_depths[i], 0, 255, 1);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;


            }
             GuiGroupBox((Rectangle){pos.x, x_pos.y, step_x*5, pos.y - x_pos.y}, "Operator Params");

             pos.y+= margin;

            x_pos.y = pos.y;
            pos.x = x_pos.x;
            // keyboard
            {
                int oct = 8;
                Rectangle bounds = {pos.x, pos.y, step_x*5, step*3.0f};
                Vector2 white = { bounds.width / (oct*7), bounds.height };
                Vector2 black = { white.x * 0.8, white.y*0.6 };

                Rectangle noteon_rec;

                int8_t noteon = -1;
                int8_t velocity = 0;

                float x = pos.x;
                float y = pos.y;
                for(int o=0; o< oct; o++){
                    const int8_t whites[] = {0, 2, 4, 5, 7, 9, 11};
                    const int8_t blacks[] = {1, 3, 6, 8, 10};
                    float x2 = x;
                    const int8_t offset = 60-(oct / 2)* 12 + o*12;
                    Rectangle recs[12];

                    for(int k=0; k<7; k++){


                        Rectangle rec = recs[whites[k]] = (Rectangle){x2, y, white.x, white.y};
                        int8_t n = whites[k]+ offset;

                        if(n == noteon_number){
                            noteon_rec = rec;
                            DrawRectangleRec(rec, SKYBLUE);
                            DrawRectangleLinesEx(rec, 1, GRAY);
                        }
                        else {
                            DrawRectangleRec(rec, RAYWHITE);
                            DrawRectangleLinesEx(rec, 1, GRAY);
                        }

                        x2 += white.x;
                    }

                    x2 = x + white.x * 0.6;
                    for(int k=0; k<5; k++){



                        Rectangle rec = recs[blacks[k]] = (Rectangle){x2, y, black.x, black.y};
                        int8_t n = blacks[k]+ offset;


                        if(n == noteon_number){
                            noteon_rec = rec;
                            DrawRectangleRec(rec, DARKBLUE);
                        } else {
                            DrawRectangleRec(rec, DARKGRAY);
                        }


                        x2 += (k != 1 && k != 4) ? white.x : white.x*2;
                    }

                    for(int i=0; i<7; i++){
                        int8_t n = offset + whites[i];
                        Vector2 mouse = GetMousePosition();
                        if(CheckCollisionPointRec(mouse, recs[whites[i]]) ){
                            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && noteon_number != n){
                                    noteon = n;
                                    velocity = (mouse.y - recs[whites[i]].y)*127 / recs[whites[i]].height;
                            }
                        }
                    }
                    for(int i=0; i<5; i++){
                        int8_t n = offset + blacks[i];
                        Vector2 mouse = GetMousePosition();
                        if(CheckCollisionPointRec(mouse, recs[blacks[i]]) ){
                            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && noteon_number != n){
                                    noteon = n;
                                    velocity = (mouse.y - recs[blacks[i]].y)*127 / recs[blacks[i]].height;
                            }
                        }
                    }

                    x += white.x*7;
                }



                if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && noteon_number != -1){
                    ks_synth_note_off(&note);
                    noteon_number = -1;
                }

                if(noteon != -1){
                    ks_synth_set(&synth, SAMPLING_RATE, &synth_bin);
                    ks_synth_note_on(&note, &synth, SAMPLING_RATE, noteon, velocity);
                    noteon_number = noteon;
                }
            }
		EndDrawing();
		//----------------------------------------------------------------------------------
    }

    UnloadTexture(algorithm_images);
    UnloadTexture(lfo_wave_images);
    UnloadTexture(keyscale_left_images);
    UnloadTexture(keyscale_right_images);

    CloseWindow();
    CloseAudioDevice();

    free(buf);

    return 0;
}


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

    GuiDrawText(innerText, rec, GUI_TEXT_ALIGN_CENTER,
             Fade(GetColor(GuiGetStyle(SLIDER, BORDER)), guiAlpha));

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
    DrawTexturePro(tex, src, (Rectangle){rec.x + rec.width/2.0f, rec.y + rec.height/2.0f, src.width, src.height}, (Vector2){src.width/2.0f, src.height/2.0f}, 0.0f, Fade(tex_color ,guiAlpha));

    GuiDrawRectangle(rec, GuiGetStyle(SLIDER, BORDER_WIDTH),
                     Fade(GetColor(GuiGetStyle(SLIDER, BORDER + (state*3))), guiAlpha),
                     BLANK);

    return value;
}

void GuiAlignedLabel(const char* text, Rectangle rec, GuiTextAlignment align){
    GuiDrawText(text, rec, align, GetTextColorDefault());
}
