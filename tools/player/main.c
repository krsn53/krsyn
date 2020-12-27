
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
    const ks_tone_list_data *tones_data;
    const ks_tone_list      *tones;
    enum{
        STOP,
        PLAY,
        PAUSE,
    } play_state;

    AudioStream     stream;
    i32*            buf;
} player_state;

static const int screenWidth = 250;
static const int screenHeight = 120;
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

    if(ret) {
        ks_score_state_set_default(ps->score_state, ps->tones, SAMPILNG_RATE, ps->score->resolution);
        SetWindowTitle( GetFileName(file) );
    }



    return ret;
}

void init(player_state* ps){
    ps->score = NULL;
    ps->score_state = ks_score_state_new(POLYPHONY_BITS);
    ps->tones_data = &default_tone_list;
    ps->tones = ks_tone_list_new_from_data(SAMPILNG_RATE, ps->tones_data);

    ks_score_state_add_volume_analizer(ps->score_state, SAMPILNG_RATE, VOLUME_LOG);

    ps->stream = InitAudioStream(SAMPILNG_RATE, 32, BUFFER_CHANNELS);
    ps->buf = calloc(BUFFER_LENGTH_PER_UPDATE, sizeof(i32));

    PlayAudioStream(ps->stream);
}

void deinit(player_state* ps){
    if(ps->score != NULL){
        ks_score_data_free(ps->score);
        ks_score_state_free(ps->score_state);
    }

    if(ps->tones_data != &default_tone_list){
        ks_tone_list_data_free((ks_tone_list_data*)ps->tones_data);
    }
    ks_tone_list_free((ks_tone_list*)ps->tones);

    CloseAudioStream(ps->stream);
    free(ps->buf);
}

void update(void* ptr){
    player_state* ps = ptr;


    if(IsAudioStreamProcessed(ps->stream)){
        if(ps->play_state == PLAY && ps->score != NULL){
            if(ps->score_state->passed_tick < 0) {
                ps->play_state = STOP;
                ks_score_state_set_default(ps->score_state, ps->tones, SAMPILNG_RATE, ps->score->resolution);
                ks_effect_volume_analizer_clear(ps->score_state->effects.data);
            } else {
                ks_score_data_render(ps->score, SAMPILNG_RATE, ps->score_state, ps->tones, ps->buf, BUFFER_LENGTH_PER_UPDATE);
            }
        } else {
            memset(ps->buf, 0, sizeof(i32)*BUFFER_LENGTH_PER_UPDATE);

        }

        float * f = calloc(BUFFER_LENGTH_PER_UPDATE, sizeof(float));
        for(int i=0; i< BUFFER_LENGTH_PER_UPDATE; i++){
            f[i]  = ps->buf[i] / (float)INT16_MAX;
        }
        UpdateAudioStream(ps->stream, f, BUFFER_LENGTH_PER_UPDATE);
        free(f);
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
        ps->play_state = PLAY;
    }
    sr.x += step_x;
    // pause
    if(GuiToggle(sr, "#132#", ps->play_state == PAUSE)){
        ps->play_state = PAUSE;
    }
    sr.x += step_x;
    // stop
    if(GuiToggle(sr, "#133#", false)){
        ps->play_state = STOP;
        ks_score_state_set_default(ps->score_state, ps->tones, SAMPILNG_RATE, ps->score->resolution);
        ks_effect_volume_analizer_clear(ps->score_state->effects.data);
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

        const float channel_width = (screenWidth - margin * 2)/ (KS_NUM_CHANNELS+2.5);

        {
            Rectangle or = {sr.x+margin, sr.y+margin + step_y*4, channel_width, step_y*4};

            const u32* channel_out = ks_effect_calc_volume(&ps->score_state->effects.data[0]);

            const float x_offset = or.width*0.1f;
            const float wid = or.width * 0.8f;

            for(u32 i =0; i<KS_NUM_CHANNELS; i++){
                float hei = (float)channel_out[i]/ ks_1(KS_OUTPUT_BITS)*step_y*5;
                DrawRectangleRec((Rectangle){or.x+ x_offset, or.y-hei-margin*2, wid, hei}, (Color){ 100, 220, 255, 255 });
                or.x += channel_width;
            }

            or.x += channel_width*0.5f;
            float hei = (float)channel_out[KS_NUM_CHANNELS]/ ks_1(KS_OUTPUT_BITS)*step_y*5;
            DrawRectangleRec((Rectangle){or.x+ x_offset, or.y-hei-margin*2, wid, hei}, (Color){ 80, 180, 255, 255 });
            or.x += channel_width;
            hei = (float)channel_out[KS_NUM_CHANNELS+1]/ ks_1(KS_OUTPUT_BITS)*step_y*5;
            DrawRectangleRec((Rectangle){or.x+ x_offset, or.y-hei-margin*2, wid, hei}, (Color){ 80, 180, 255, 255 });
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

            cr.x += channel_width;

            int p = 0;
            for(int i=0; i<ks_1(POLYPHONY_BITS); i++)
            {
                if(ks_score_note_is_enabled(ps->score_state->notes + i)){
                            p++;
                }
            }

           GuiLabel(cr, FormatText("%d", p));
        }
        DrawRectangleLinesEx(wr, 1, GRAY);

        sr.y += wr.height + margin;
    }


    if(IsFileDropped()){
        int count = 0;
        char** f = GetDroppedFiles(&count);
        if(count == 1){
            load_score(ps, *f);
        }
        ClearDroppedFiles();
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
