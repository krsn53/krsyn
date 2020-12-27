
#include <raylib.h>
#include <raygui.h>
#include <ricons.h>

#include <krsyn.h>
#include <ksio/io.h>
#include <ksio/midi.h>

#define SAMPILNG_RATE               48000
#define VOLUME_LOG                  (1<<(KS_TIME_BITS-4))
#define POLYPHONY_BITS              8

#define SAMPLES_PER_UPDATE          4096
#define BUFFER_CHANNELS             2
#define BUFFER_LENGTH_PER_UPDATE    (SAMPLES_PER_UPDATE*BUFFER_CHANNELS)


typedef struct player_state{
    ks_score_data *score;
    ks_score_state* score_state;

    enum{
        UNKNOWN,
        STOP,
        PLAY,
        PAUSE,
    } play_state;

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

static const ks_tone_list_data default_tone_list=
        #include "../test_tones/test.kstc"
;

bool load_score(player_state* ps, const char* file){
    bool ret = true;

    if(!FileExists(file)){
        ks_error("File \"%s\" not exists.", file);
        return false;
    }

    if(ps->score != NULL){
        ks_score_data_free(ps->score);
    }

    const char* ext = GetFileExtension(file);
    //midifile
    if(strcmp(ext,  "mid")  == 0 || strcmp(ext,"midi") == 0 ){
        ks_midi_file midi;
        ks_io* io = ks_io_new();
        ks_io_read_file(io, file);
        if(!ks_io_begin_deserialize(io, binary_big_endian, ks_prop_root(midi, ks_midi_file))){
            ks_error("Failed to load midi file");
            ret = false;
        }
        else {
             ps->score = ks_score_data_from_midi(&midi);
        }

        ks_io_free(io);
        ks_midi_tracks_free(midi.num_tracks, midi.tracks);
    }
    // krsyn score file
    else if(strcmp(ext, "kscb") == 0){
        ps->score = ks_score_data_new(0,0, 0);
        ks_io* io = ks_io_new();
        if(!ks_io_begin_deserialize(io, binary_little_endian, ks_prop_root(*ps->score, ks_score_data))){
            ks_error("Failed to load krsyn score binary file");
            ret = false;
        }
        ks_io_free(io);
    }
    else if(strcmp(ext, "kscc") == 0){
        ps->score = ks_score_data_new(0, 0, 0);
        ks_io* io = ks_io_new();
        if(!ks_io_begin_deserialize(io, clike, ks_prop_root(*ps->score, ks_score_data))){
            ks_error("Failed to load krsyn score clike file");
            ret = false;
        }
        ks_io_free(io);
    }
    else {
        ks_error("Invalid file type. Extention must be one of the following:\n\t\t*.mid *.midi *.kscb *kscc");
    }

    return ret;
}

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
    if(GuiToggle(sr, "#131#", ps->play_state == PLAY)){
        PlayAudioStream(ps->stream);
    }
    sr.x += step_x;
    // pause
    if(GuiToggle(sr, "#132#", ps->play_state == PAUSE)){
        PauseAudioStream(ps->stream);
    }
    sr.x += step_x;
    // stop
    if(GuiToggle(sr, "#133#", ps->play_state == STOP)){
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
        const float channel_width = (screenWidth - margin * 2)/ (KS_NUM_CHANNELS+2.5);

        {
            Rectangle or = {sr.x, sr.y+step_y*4, channel_width, step_y*4};

            const i16* channel_out = ks_effect_calc_volume(&ps->score_state->effects.data[0]);

            const float x_offset = or.width*0.1f;
            const float width = or.width * 0.8f;

            for(u32 i =0; i<KS_NUM_CHANNELS; i++){
                float height = (float)channel_out[i]/ ks_1(KS_OUTPUT_BITS)*step_y*5;
                DrawRectangleRec((Rectangle){or.x+ x_offset, or.y-height, width, height}, SKYBLUE);
                or.x += channel_width;
            }
        }

        {
            Rectangle cr ={sr.x, sr.y + step_y*4, channel_width, height};
            GuiSetStyle(LABEL, TEXT_ALIGNMENT, GUI_TEXT_ALIGN_CENTER);

            for(u32 i =0; i<KS_NUM_CHANNELS; i++){
                GuiLabel(cr, TextFormat("%d", i+1));
                cr.x += channel_width;
            }
            cr.x += channel_width*0.5;

            GuiLabel(cr, "L");
            cr.x += channel_width;
            GuiLabel(cr, "R");



        }
        sr.y += wr.height + margin;
    }



    ClearBackground(RAYWHITE);




    EndDrawing();
    //----------------------------------------------------------------------------------
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    InitWindow(screenWidth, screenHeight, "Krsyn Player");
    InitAudioDevice();

    SetTargetFPS(60);

    player_state ps = { 0 };

    init(&ps);

    if(argc == 2){
        load_score(&ps, argv[1]);
    }

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
