
#include <raylib.h>
#include <raygui_impl.h>

#include <krsyn.h>
#include <ksio/io.h>
#include <ksio/midi.h>

#define SAMPLING_RATE               48000
#define VOLUME_LOG                  (1<<(KS_TIME_BITS-4))
#define POLYPHONY_BITS              8

#define SAMPLES_PER_UPDATE          4096
#define BUFFER_CHANNELS             2
#define BUFFER_LENGTH_PER_UPDATE    (SAMPLES_PER_UPDATE*BUFFER_CHANNELS)
#define BUFFER_LENGTH_PER_EXPORT    (BUFFER_LENGTH_PER_UPDATE << 4)

typedef struct player_state{
    ks_score_data *score;
    ks_score_state* score_state;
    const ks_tone_list_data *tones_data;
    const ks_tone_list      *tones;
    enum{
        STOP,
        PLAY,
        PAUSE,
        ERROR,
        INFO,
        EXPORT
    } player_state;

    const char*     message;
    char            score_file[256];
    char            export_filename[64];

    AudioStream     stream;
    i32*            buf;

    u32             tick;
    double          time;

    u32             volume;
} player_state;

static const int screenWidth = 260;
static const int screenHeight = 100;
static const u32 width = 18;
static const u32 height =18;
static const u32 margin = 1;
static const u32 step_x = width + margin;
static const u32 step_y = height + margin;
static const Rectangle base_pos = (Rectangle){ margin, margin, width, height };

static const ks_tone_list_data default_tone_list=
        #include "../test_tones/test.kstc"
;

bool load_file(player_state* ps, const char* file){
    int state = 0;

    if(!FileExists(file)){
        ks_error("File \"%s\" not exists.", file);
        return false;
    }

    const char* ext = GetFileExtension(file);
    if(ext == NULL) ext = "";
    //midifile
    if(strcmp(ext,  "mid")  == 0 || strcmp(ext,"midi") == 0 ){
        ks_midi_file midi;
        ks_io* io = ks_io_new();
        ks_io_read_file(io, file);
        if(!ks_io_begin_deserialize(io, binary_big_endian, ks_prop_root(midi, ks_midi_file))){
            ks_error("Failed to load midi file");
            ps->message = "Failed to load midi file";
            ps->player_state = ERROR;
        }
        else {
             ks_score_data_free(ps->score);
             ps->score = ks_score_data_from_midi(&midi);
             state = 1;
        }

        ks_io_free(io);
        ks_midi_tracks_free(midi.num_tracks, midi.tracks);
    }
    // krsyn score file
    else if(strcmp(ext, "kscb") == 0){
        ks_score_data* score = ks_score_data_new(0,0, 0);
        ks_io* io = ks_io_new();
        ks_io_read_file(io, file);
        if(!ks_io_begin_deserialize(io, binary_little_endian, ks_prop_root(*ps->score, ks_score_data))){
            ks_error("Failed to load krsyn score binary file");
            ps->message = "Failed to load score file";
            ps->player_state = ERROR;
            ks_score_data_free(score);
        }
        else {
            ks_score_data_free(ps->score);
            ps->score = score;
            state = 1;
        }
        ks_io_free(io);
    }
    else if(strcmp(ext, "kscc") == 0){
        ks_score_data* score = ks_score_data_new(0,0, 0);
        ks_io* io = ks_io_new();
        ks_io_read_file(io, file);
        if(!ks_io_begin_deserialize(io, clike, ks_prop_root(*ps->score, ks_score_data))){
            ks_error("Failed to load krsyn score clike file");
            ps->message = "Failed to load score file";
            ps->player_state = ERROR;
             ks_score_data_free(score);

        }
        else{
            ks_score_data_free(ps->score);
            ps->score = score;
            state = 1;
        }
        ks_io_free(io);
    }
    else if(strcmp(ext, "kstb") == 0){
        if(ps->tones_data != &default_tone_list){
            ks_tone_list_data_free((ks_tone_list_data*)ps->tones_data);
        }
        ks_tone_list_data* dat = ks_tone_list_data_new();
        ks_io* io = ks_io_new();
        ks_io_read_file(io, file);
        if(!ks_io_begin_deserialize(io, binary_little_endian, ks_prop_root(*dat, ks_tone_list_data))){
            ks_error("Failed to load krsyn tone list binary file");
            ps->message = "Failed to load tone list file";
            ps->player_state = ERROR;
            ks_tone_list_data_free(dat);
        } else {
            state = 2;
            ps->tones_data = dat;
        }
        ks_io_free(io);
    }
    else if(strcmp(ext, "kstc") == 0){
        if(ps->tones_data != &default_tone_list){
            ks_tone_list_data_free((ks_tone_list_data*)ps->tones_data);
        }
        ks_tone_list_data* dat = ks_tone_list_data_new();
        ks_io* io = ks_io_new();
        ks_io_read_file(io, file);
        if(!ks_io_begin_deserialize(io, clike, ks_prop_root(*dat, ks_tone_list_data))){
            ks_error("Failed to load krsyn tone list clike file");
            ps->message = "Failed to load tone list file";
            ps->player_state = ERROR;
            ks_tone_list_data_free(dat);
        } else {
            state = 2;
            ps->tones_data = dat;
        }
        ks_io_free(io);
    }
    else {
        ks_error("Invalid file type. Extention must be one of the following:\n\t\t*.mid *.midi *.kscb *kscc *.kstb *.kstc");
        ps->message = "Invalid file type";
        ps->player_state = ERROR;
    }

    if(state == 1) {
        ks_score_data_calc_score_length(ps->score, SAMPLING_RATE);
        ks_score_state_set_default(ps->score_state, ps->tones, SAMPLING_RATE, ps->score->resolution);
        SetWindowTitle( GetFileName(file) );
        strcpy(ps->score_file, file);
    }
    else if(state == 2){
        ks_tone_list_free((ks_tone_list*)ps->tones);
        ps->tones = ks_tone_list_new_from_data(SAMPLING_RATE, ps->tones_data);
    }

    return state != 0;
}

void stop(player_state* ps){
    ps->player_state = STOP;
    ks_score_state_set_default(ps->score_state, ps->tones, SAMPLING_RATE, ps->score->resolution);
    ks_effect_volume_analizer_clear(ps->score_state->effects.data);
    ps->time = 0;
    ps->tick = 0;
}

void init(player_state* ps){
    ps->score = ks_score_data_new(96, 0, NULL);
    ps->score_state = ks_score_state_new(POLYPHONY_BITS);
    ps->tones_data = &default_tone_list;
    ps->tones = ks_tone_list_new_from_data(SAMPLING_RATE, ps->tones_data);

    ks_score_state_add_volume_analizer(ps->score_state, SAMPLING_RATE, VOLUME_LOG);

    ps->stream = InitAudioStream(SAMPLING_RATE, 32, BUFFER_CHANNELS);
    ps->buf = calloc(BUFFER_LENGTH_PER_UPDATE, sizeof(i32));

    ps->volume = 100;

    PlayAudioStream(ps->stream);
}

void deinit(player_state* ps){
    ks_score_data_free(ps->score);
    ks_score_state_free(ps->score_state);

    if(ps->tones_data != &default_tone_list && ps->tones_data != NULL){
        ks_tone_list_data_free((ks_tone_list_data*)ps->tones_data);
    }
    ks_tone_list_free((ks_tone_list*)ps->tones);

    CloseAudioStream(ps->stream);
    free(ps->buf);
}

void update(void* ptr){
    player_state* ps = ptr;

    if(IsAudioStreamProcessed(ps->stream)){
        if(ps->player_state == PLAY && ps->score != NULL && ps->tones_data != NULL){
            if(ps->score_state->passed_tick < 0) {
                stop(ps);
            } else {
                ks_score_data_render(ps->score, SAMPLING_RATE, ps->score_state, ps->tones, ps->buf, BUFFER_LENGTH_PER_UPDATE);
            }
        } else {
            memset(ps->buf, 0, sizeof(i32)*BUFFER_LENGTH_PER_UPDATE);

        }

        float * f = calloc(BUFFER_LENGTH_PER_UPDATE, sizeof(float));
        float volume = ps->volume * 0.01f;
        for(int i=0; i< BUFFER_LENGTH_PER_UPDATE; i++){
            f[i]  = ps->buf[i] * volume / (float)INT16_MAX;
        }
        UpdateAudioStream(ps->stream, f, BUFFER_LENGTH_PER_UPDATE);
        free(f);
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if(ps->player_state >= ERROR){
        GuiDisable();
    }

    if(ps->score->length == 0 && ps->player_state < ERROR) {
        GuiDisable();
    }

    const Color border_color = GetColor(GuiGetStyle(DEFAULT, GuiGetState() == GUI_STATE_DISABLED ? BORDER_COLOR_DISABLED : BORDER_COLOR_NORMAL));

    Rectangle sr = base_pos;



//        // previous
//        if(GuiLabelButton(sr, "#129#")){
//        }
//        sr.x += step_x;
    // play
    if(GuiToggle(sr, "#131#", ps->player_state == PLAY)){
        ps->player_state = PLAY;
    }
    sr.x += step_x;
    // pause
    if(GuiToggle(sr, "#132#", ps->player_state == PAUSE)){
        ps->player_state = PAUSE;
    }
    sr.x += step_x;
    // stop
    if(GuiToggle(sr, "#133#", false)){
        stop(ps);
    }
    sr.x += step_x;

    //export

    if(GuiToggle(sr, "#7#", false)){
        ps->player_state = EXPORT;
        strcpy(ps->export_filename, FormatText("%s.wav", GetFileNameWithoutExt(ps->score_file)));
    }
    sr.x += step_x;
//        // next
//        if(GuiLabelButton(sr, "#134#")){
//        }
//        sr.x += step_x;

    // time
    sr.width = (int)((screenWidth - sr.x) * 0.66f) - margin;
    const u64 delta_frame = (ps->score_state->current_tick - ps->tick) * ps->score_state->frames_per_event;
    const double delta_time = (double)delta_frame / SAMPLING_RATE;
    ps->tick = ps->score_state->current_tick;
    ps->time += delta_time;

    {
        const int now_time = (int)ps->time;
        const int now_min = now_time / 60;
        const int now_sec = now_time % 60;

        const int song_length = (int)ps->score->score_length ;
        const int song_min = song_length / 60;
        const int song_sec = song_length % 60;

        float now_seek = MIN(ps->time,  ps->score->score_length);
        float seek = GuiSliderBar(sr, "", "", now_seek, 0.0f, ps->score->score_length);
        if(ps->score->length != 0 && seek != now_seek){
            ks_score_state_set_default(ps->score_state, ps->tones, SAMPLING_RATE, ps->score->resolution);

            double time = 0;
            u64 tick = 0;

            u32 i=0;
            for(; i< ps->score->length; i++){
                tick = tick+ ps->score->data[i].delta;
                const u64 delta_frame = (u64)ps->score->data[i].delta* ps->score_state->frames_per_event;
                const double delta_time =  (double)delta_frame / SAMPLING_RATE;
                time = time + delta_time;

                if(time >= seek)break;
                // event run
                if(ps->score->data[i].status >= 0xb0){
                     ps->score_state->current_event = i;
                    ks_score_data_event_run(ps->score, SAMPLING_RATE, ps->score_state, ps->tones);
                }
            }

            const float remaining_time = time - seek;
            const u32 remaining_tick = remaining_time * SAMPLING_RATE / ps->score_state->frames_per_event;
            const u32 passed_tick = ps->score->data[i].delta - remaining_tick;

            ps->time = seek;
            ps->tick = tick - remaining_tick;
            ps->score_state->current_tick = ps->tick;
            ps->score_state->passed_tick = passed_tick;
            ps->score_state->current_event = i-1;
        }
        GuiLabel(sr, FormatText("%02d:%02d / %02d:%02d", now_min, now_sec, song_min, song_sec));
    }


    sr.x += sr.width + margin;
    sr.width = (screenWidth - sr.x) - margin;

    // volume
    ps->volume = PropertyInt(sr, FormatText("#122#%d %%", ps->volume), ps->volume, 0, 300, 1);

    sr.x = base_pos.x;
    sr.width = base_pos.width;
    sr.y += step_y;

    // output level
    {
        float base_height =  screenHeight - sr.y;
        float base_width = screenWidth - margin*2;

        Rectangle wr ={sr.x, sr.y, base_width, base_height };

        const float channel_width = (base_width - margin * 2)/ (KS_NUM_CHANNELS+2.5);

        {
            Rectangle or = {sr.x+margin, sr.y+margin + base_height - step_y, channel_width, base_height - step_y};

            const u32* channel_out = ks_effect_calc_volume(&ps->score_state->effects.data[0]);

            const Color channel_color = GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_PRESSED));
            const Color output_color = GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED));
            const float x_offset = or.width*0.1f;
            const float wid = or.width * 0.8f;



            for(u32 i =0; i<KS_NUM_CHANNELS; i++){
                float hei = (float)channel_out[i]/ ks_1(KS_OUTPUT_BITS)*base_height*2;
                DrawRectangleRec((Rectangle){or.x+ x_offset, or.y-hei-margin*2, wid, hei},channel_color);
                or.x += channel_width;
            }

            or.x += channel_width*0.5f;
            float hei = (float)channel_out[KS_NUM_CHANNELS]/ ks_1(KS_OUTPUT_BITS)*base_height;
            DrawRectangleRec((Rectangle){or.x+ x_offset, or.y-hei-margin*2, wid, hei}, output_color);
            or.x += channel_width;
            hei = (float)channel_out[KS_NUM_CHANNELS+1]/ ks_1(KS_OUTPUT_BITS)*base_height;
            DrawRectangleRec((Rectangle){or.x+ x_offset, or.y-hei-margin*2, wid, hei}, output_color);
        }

        {
            Rectangle cr ={sr.x, sr.y + base_height - step_y, channel_width, height};
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

        }
        DrawRectangleLinesEx(wr, 1, border_color);
        const int underline_y = wr.y + base_height - step_y;
        const int separateline_x = wr.x + margin + channel_width*16.25;
        DrawLine(wr.x + margin, underline_y, wr.x + wr.width - margin, underline_y, border_color);
        DrawLine(separateline_x, wr.y + margin, separateline_x, wr.y + wr.height - margin, border_color);

        sr.y += wr.height + margin;
    }

    if(ps->score->length == 0 && ps->player_state < ERROR) {
        GuiEnable();
    }


    if(IsFileDropped()){
        int count = 0;
        char** f = GetDroppedFiles(&count);
        if(count == 1){
            if(ps->player_state == PLAY || ps->player_state == PAUSE){
                stop(ps);
            }
            load_file(ps, *f);
        }
        ClearDroppedFiles();
    }

    if(ps->player_state == ERROR || ps->player_state == INFO){
        GuiEnable();
        float margin = screenHeight * 0.1;
        Rectangle rec = {margin, margin, screenWidth - margin*2, screenHeight - margin*2};

        DrawRectangleRec(rec, ps->player_state == ERROR ? (Color){255, 192, 192, 200} : (Color){192, 255, 192, 200});
        DrawRectangleLinesEx(rec, 1, (Color){0, 0, 0, 200});

        if(GuiLabelButton(rec,  ps->message)){
            stop(ps);
        };

        Rectangle er = {rec.x + margin*2, rec.y + margin*2, rec.width *0.3f, step_y};
        GuiLabel(er, ps->player_state == ERROR ?  "Error : " : "Info : ");
    }
    else if(ps->player_state == EXPORT){
        GuiEnable();
        float margin = screenHeight * 0.1;
        Rectangle rec = {margin, margin, screenWidth - margin*2, screenHeight - margin*2};

        DrawRectangleRec(rec, (Color){192, 192, 255, 200});
        DrawRectangleLinesEx(rec, 1, (Color){0, 0, 0, 200});

        const Rectangle ex_base = {rec.x + margin, rec.y + margin, rec.width / 2 - margin*2, base_pos.height};
        Rectangle ex = ex_base;
        ex.height = 24;

        GuiAlignedLabel("Export File Name", ex, GUI_TEXT_ALIGN_RIGHT);

        ex.x += rec.width / 2;
        GuiTextBox(ex, ps->export_filename, GuiGetStyle(TEXTBOX, TEXT_SIZE), true);

        ex.x = ex_base.x;
        ex.y += ex.height + margin;

        if(GuiButton(ex, "OK")){
            u32 len = (ps->score->score_length+1)*SAMPLING_RATE*BUFFER_CHANNELS;
            i32* tmp_buf = calloc(BUFFER_LENGTH_PER_EXPORT, sizeof(i32));
            i16* export_buf = calloc(BUFFER_LENGTH_PER_EXPORT, sizeof(i16));

            char file_path[256];

            const int score_path_len = strlen(ps->score_file);
            const int score_file_len = strlen(GetFileName(ps->score_file));
            const int score_dir_len = score_path_len - score_file_len;
            int export_file_len = strlen(ps->export_filename);

            if(export_file_len == 0){
                strcpy(ps->export_filename, "output.wav");
            }
            else if(strcmp(ps->export_filename + export_file_len - 4, ".wav") != 0){
                strncpy(ps->export_filename + export_file_len - 4, ".wav", 4);
                export_file_len += 4;
            }

            strncpy(file_path, ps->score_file, score_dir_len);
            strcpy(file_path + score_dir_len, ps->export_filename);

            int buf_size = sizeof(i16) * len;

            struct wave_header
            {
              u32 riff_id;
              u32 chunk_size;
              u32 format;
              u32 format_id;
              u32 fmt_chunk_byte_num;
              u16 tone_format;
              u16 channel_num;
              u32 sampling_freq;
              u32 mean_byte_num_per_sec;
              u16 block_size;
              u16 sample_bits;
              u32 sub_chunk_id;
              u32 sub_chunk_size;
            } out = {
                .riff_id = 0x46464952,
                .chunk_size = sizeof(out) + buf_size - 8,
                .format = 0x45564157,
                .format_id = 0x20746D66,
                .fmt_chunk_byte_num = 16,
                .tone_format = 1,
                .channel_num = 2,
                .sampling_freq = SAMPLING_RATE,
                .mean_byte_num_per_sec = SAMPLING_RATE*4,
                .block_size = 4,
                .sample_bits = 16,
                .sub_chunk_id = 0x61746164,
                .sub_chunk_size = sizeof(out) + buf_size - 126,
            };

            FILE* f = fopen(file_path, "wb");
            fwrite(&out, 1, sizeof(out), f);

            ks_score_state_set_default(ps->score_state, ps->tones, SAMPLING_RATE, ps->score->resolution);
            u32 seek = 0;
            while(seek < len){
               u32 write_len = MIN(len - seek, BUFFER_LENGTH_PER_EXPORT);
               ks_score_data_render(ps->score, SAMPLING_RATE, ps->score_state, ps->tones, tmp_buf, write_len);
               for(u32 i = 0; i< write_len; i++){
                   export_buf[i] = tmp_buf[i] >> 1;
               }
               seek+=write_len;
               fwrite(export_buf, 1, write_len*sizeof(i16), f);

               BeginDrawing();
               ClearBackground(RAYWHITE);

               Rectangle screc = {0, 0, screenWidth, screenHeight};
               float p = (float)seek / len;
               GuiProgressBar(screc, "", "", p, 0.0f, 1.0f);
               GuiLabel(screc, FormatText("%d / %d (%.1f%%)", seek, len, p*100.0));

               EndDrawing();
            }

            fclose(f);

            free(tmp_buf);
            free(export_buf);

            ps->message = "Export successfully";
            ps->player_state = INFO;
        }

        ex.x += rec.width / 2;

        if(GuiButton(ex, "Cancel")){
            stop(ps);
        }
    }

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
        load_file(&ps, argv[1]);
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
