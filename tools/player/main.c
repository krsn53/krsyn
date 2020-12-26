
#include <raylib.h>
#include <raygui.h>
#include <ricons.h>

#include <krsyn.h>
#include <ksio/io.h>

#define SAMPILNG_RATE               48000
#define VOLUME_LOG                  (1<<(KS_TIME_BITS-4))
#define POLYPHONY_BITS              8

#define SAMPLES_PER_UPDATE          4096
#define BUFFER_CHANNELS             2
#define BUFFER_LENGTH_PER_UPDATE    (SAMPLES_PER_UPDATE*BUFFER_CHANNELS)


typedef struct player_state{
    ks_score_data *score;
    ks_score_state* score_state;

    AudioStream     stream;
    i16*            buf;
} player_state;

static const int screenWidth = 300;
static const int screenHeight = 150;
static const u32 width = 18;
static const u32 height =18;
static const u32 margin = 1;
static const u32 step_x = width + margin;
static const u32 step_y = height + margin;
static const Rectangle base_pos = (Rectangle){ margin, margin, width, height };

void init(player_state* ps){
    ps->score = NULL;
    ps->score_state = ks_score_state_new(POLYPHONY_BITS);

    ks_score_state_add_volume_analizer(ps->score_state, SAMPILNG_RATE, VOLUME_LOG);

    ps->stream = InitAudioStream(SAMPILNG_RATE, sizeof(i16), BUFFER_CHANNELS);
    ps->buf = calloc(BUFFER_LENGTH_PER_UPDATE, sizeof(i16));
}

void deinit(player_state* ps){
    if(ps->score != NULL){
        ks_score_data_free(ps->score);
        ks_score_state_free(ps->score_state);
    }

    CloseAudioStream(ps->stream);
    free(ps->buf);
}

void update(void* ptr){
    player_state* ps = ptr;


    if(IsAudioStreamProcessed(ps->stream)){
        //hoge

        UpdateAudioStream(ps->stream, ps->buf, BUFFER_LENGTH_PER_UPDATE);
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    Rectangle sr = base_pos;
//        // previous
//        if(GuiLabelButton(sr, "#129#")){
//        }
//        sr.x += step_x;
    // play
    if(GuiLabelButton(sr, "#131#")){
        PlayAudioStream(ps->stream);
    }
    sr.x += step_x;
    // pause
    if(GuiLabelButton(sr, "#132#")){
        PauseAudioStream(ps->stream);
    }
    sr.x += step_x;
    // stop
    if(GuiLabelButton(sr, "#133#")){
        StopAudioStream(ps->stream);
    }
    sr.x += step_x;
//        // next
//        if(GuiLabelButton(sr, "#134#")){
//        }
//        sr.x += step_x;

    // playback now position

    //wave
    sr.x = base_pos.x;
    sr.y += step_y;
    {
        Rectangle wr ={sr.x, sr.y, screenWidth - margin*2, step_y*5 };
        DrawRectangleLinesEx(wr, 1, BLACK);
        Rectangle cr ={sr.x, sr.y + step_y*4, (screenWidth - margin * 2)/ (KS_NUM_CHANNELS+2.5), height};
        GuiSetStyle(LABEL, TEXT_ALIGNMENT, GUI_TEXT_ALIGN_CENTER);

        for(u32 i =0; i<KS_NUM_CHANNELS; i++){
            GuiLabel(cr, TextFormat("%d", i+1));
            cr.x += cr.width;
        }
        cr.x += cr.width*0.5;

        GuiLabel(cr, "L");
        cr.x += cr.width;
        GuiLabel(cr, "R");


        sr.y += wr.height + margin;
    }



    ClearBackground(RAYWHITE);




    EndDrawing();
    //----------------------------------------------------------------------------------
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{


    InitWindow(screenWidth, screenHeight, "Krsyn Player");
    InitAudioDevice();

    SetTargetFPS(60);

    player_state ps = { 0 };

    init(&ps);

    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        update(&ps);
    }

    deinit(&ps);

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    CloseAudioDevice();
    //--------------------------------------------------------------------------------------



    return 0;
}
