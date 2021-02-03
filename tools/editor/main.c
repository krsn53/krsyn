#include "raygui_impl.h"
#ifdef PLATFORM_DESKTOP
#include "../rtmidi/rtmidi_c.h"
#endif
#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#define SCREEN_WIDTH 726
#define SCREEN_HEIGHT 552

#define SAMPLING_RATE               48000
#define SAMPLES_PER_UPDATE          4096
#define BUFFER_LENGTH_PER_UPDATE    (SAMPLES_PER_UPDATE*NUM_CHANNELS)
#define TIME_PER_UPDATE             ((double)SAMPLES_PER_UPDATE / SAMPLING_RATE)
#define NUM_CHANNELS                2
#define MIDIIN_RESOLUTION           2400
#define MIDIIN_POLYPHONY_BITS       6

#include "krsyn.h"
#include <ksio/vector.h>
#include <ksio/serial/clike.h>
#include <ksio/serial/binary.h>

//------------------------------------------------------------------------------------

#define EVENTS_PER_UPDATE           (unsigned)(MIDIIN_RESOLUTION / 0.5f * SAMPLES_PER_UPDATE / SAMPLING_RATE)
#define MIDIIN_MAX_EVENTS           (KS_NUM_CHANNELS*2*EVENTS_PER_UPDATE)
static const char* tone_list_ext=".kstb;.kstc";
static const char* synth_ext = ".ksyb;.ksyc";

static const int margin = 3;
static const int base_width = 100;
static const float line_width = 12;
#define step_x  (int)((float)base_width + margin)
#define step    (int)((float)line_width+ margin)
#define base_pos (Rectangle){ margin, margin, base_width,  line_width }

typedef struct editor_state{
    ks_tone_list_data tones_data;
    ks_tone_list *tones;
    int tone_list_scroll;
    int testbox_focus;
    i32 current_tone_index;
    ks_synth_data temp_synth;
    ks_synth synth;
    ks_synth_note note;

    ks_score_data score;
    ks_score_state* score_state;
    ks_score_event events[MIDIIN_MAX_EVENTS];
    const char* dialog_message;
    i8 noteon_number;
    bool dirty;
    GuiFileDialogState file_dialog_state, file_dialog_state_synth;
    enum{
        EDIT,
        CONFIRM_NEW,
        SAVE_DIALOG,
        LOAD_DIALOG,
        IMPORT_SYNTH_DIALOG,
        EXPORT_SYNTH_DIALOG,
        SETTINGS,
        ERROR_DIALOG,
        INFO_DIALOG,
    }display_mode;

    AudioStream audiostream;
    float *buf;
#ifdef PLATFORM_DESKTOP
    double  midi_clock;
    double  last_event_time;
    i32     last_event_tick;
    RtMidiInPtr midiin;
    u32 midiin_port;
    int midiin_list_scroll;
#endif

    Texture2D output_images;
    Texture2D operator_images;
    Texture2D wave_images;
    Texture2D mod_images;
    Texture2D keyscale_left_images;
    Texture2D keyscale_right_images;
} editor_state;

static void mark_dirty(bool* dirty, GuiFileDialogState* file_dialog_state){
    if(*dirty) return;
    *dirty = true;
    SetWindowTitle(FormatText("krsyn editor - %s%s",  file_dialog_state->fileNameText[0] == 0 ?
                       "noname" : file_dialog_state->fileNameText,  "*"));
}

static void unmark_dirty(bool* dirty, GuiFileDialogState* file_dialog_state){
    *dirty = false;
    SetWindowTitle(FormatText("krsyn editor - %s",  file_dialog_state->fileNameText[0] == 0 ?
                       "noname" : file_dialog_state->fileNameText));
}

bool save_load_synth(ks_synth_data* bin, editor_state* es, bool serialize){
    ks_io * io = ks_io_new();
    bool text_format = false;
    if(IsFileExtension(es->file_dialog_state_synth.fileNameText, ".ksyc")){
        text_format= true;
    } else {
        if(!IsFileExtension(es->file_dialog_state_synth.fileNameText, ".ksyb")){
            strcpy(es->file_dialog_state_synth.fileNameText + strlen(es->file_dialog_state_synth.fileNameText), ".ksyb");
        }
    }

    if(!serialize){
        ks_io_read_file(io, FormatText("%s/%s",es->file_dialog_state_synth.dirPathText, es->file_dialog_state_synth.fileNameText));
    }

    bool ret;
    if(text_format){
        ret = serialize ? ks_io_serialize_begin(io, clike, *bin, ks_synth_data) :
                          ks_io_deserialize_begin(io, clike, *bin, ks_synth_data) ;
    }
     else {
        ret = serialize ? ks_io_serialize_begin(io, binary_little_endian, *bin, ks_synth_data) :
                          ks_io_deserialize_begin(io, binary_little_endian, *bin, ks_synth_data) ;
    }

    if(serialize){
        SaveFileData(FormatText("%s/%s", es->file_dialog_state_synth.dirPathText, es->file_dialog_state_synth.fileNameText), io->str->data, io->str->length);
#ifdef PLATFORM_WEB
        emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')",es->file_dialog_state_synth.fileNameText, GetFileName(es->file_dialog_state_synth.fileNameText)));
#endif
    }

    ks_io_free(io);

    if(!serialize){
        mark_dirty(&es->dirty, &es->file_dialog_state);
    }

    return ret;
}

bool save_load_tone_list(ks_tone_list_data* bin, editor_state* es, bool serialize){
    ks_io * io = ks_io_new();
    bool text_format = false;
    if(IsFileExtension(es->file_dialog_state.fileNameText, ".kstc")){
        text_format= true;
    } else {
        if(!IsFileExtension(es->file_dialog_state.fileNameText, ".kstb")){
            strcpy(es->file_dialog_state.fileNameText + strlen(es->file_dialog_state.fileNameText), ".kstb");
        }
    }

    if(!serialize){
        ks_io_read_file(io, FormatText("%s/%s", es->file_dialog_state.dirPathText, es->file_dialog_state.fileNameText));
    }

    bool ret;
    if(text_format){
        ret = serialize ? ks_io_serialize_begin(io, clike, *bin, ks_tone_list_data) :
                          ks_io_deserialize_begin(io, clike, *bin, ks_tone_list_data) ;
    }
     else {
        ret = serialize ? ks_io_serialize_begin(io, binary_little_endian, *bin, ks_tone_list_data) :
                          ks_io_deserialize_begin(io, binary_little_endian, *bin, ks_tone_list_data) ;
    }

    if(serialize){
        SaveFileData(FormatText("%s/%s", es->file_dialog_state.dirPathText, es->file_dialog_state.fileNameText), io->str->data, io->str->length);
#ifdef PLATFORM_WEB
        emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')",es->file_dialog_state.fileNameText, GetFileName(es->file_dialog_state.fileNameText)));
#endif
    }

    ks_io_free(io);


    unmark_dirty(&es->dirty, &es->file_dialog_state);

    return ret;
}

static void program_number_increment(u8* msb, u8* lsb, u8* program, u8* note){
    if(*note < 0x7f){
        ++ *note;
        return;
    }

    if(*note != KS_NOTENUMBER_ALL){
        *note = 0;
    }

    if(*program < 0x7f){
        ++ *program;
        return;
    }

    *program = 0;
    if(*lsb < 0x7f){
        ++ *lsb;
        return;
    }

    *lsb = 0;
    ++ * msb;
}



static void update_tone_list(ks_tone_list ** ptr, ks_tone_list_data* dat, ks_score_state* score_state){
    if(*ptr != NULL)ks_tone_list_free(*ptr);
    *ptr = ks_tone_list_new_from_data(SAMPLING_RATE ,dat);
    u32 tmptick = score_state->current_tick;
    u32 tmpptick = score_state->passed_tick;
    ks_score_state_set_default(score_state, *ptr, SAMPLING_RATE, MIDIIN_RESOLUTION);
    score_state->current_tick = tmptick;
    score_state->passed_tick = tmpptick;
}

void EditorUpdate(void* ptr){
    editor_state* es = ptr;


    const char* text;
    // Update
    //----------------------------------------------------------------------------------
    if(IsAudioStreamProcessed(es->audiostream)){
#ifdef PLATFORM_DESKTOP
        bool res_midi_event = false;
        if(es->midiin != NULL){
            es->score.length = 0;

            while(es->score.data[es->score_state->current_event].delta != 0xffffffff){
                es->score.data[es->score.length] = es->score.data[es->score_state->current_event];
                es->score.length ++;
                es->score_state->current_event++;
            }

            es->score_state->current_event = 0;
            size_t size;
            do{
                unsigned char message[4];
                size = 4;
                double stamp = rtmidi_in_get_message(es->midiin, message, &size);

                if(size != 0){
                    es->midi_clock += stamp;

                    if(es->score.length >= (sizeof(es->events)/ sizeof(es->events[0])) - 1){ continue;}
                    if((message[0] == 0xff && message[1] != 0x51) || (message[0] >=0x80 && message[0] < 0xf0)){
                        double delta_d = es->midi_clock - es->last_event_time;
                        u32 delta = MAX(delta_d*MIDIIN_RESOLUTION*2, 0);

                        es->score.data[es->score.length].delta = delta;
                        es->score.data[es->score.length].status = message[0];
                        memcpy(es->score.data[es->score.length].data, message + 1, 3);
                        es->score.length ++;
                        es->last_event_time += delta_d;
                        es->last_event_tick += delta;
                    }

                    res_midi_event = true;
                }
            }while(size != 0);

            if(res_midi_event){
                i64 diff = (i64)es->score_state->current_tick - es->last_event_tick;

                if(diff > 0.1*MIDIIN_RESOLUTION*2){
                    es->last_event_time -= 0.1;
                }
                else if(diff < -0.1*MIDIIN_RESOLUTION*2){
                    es->last_event_time += 0.1;
                }

            }

            es->score.data[es->score.length].delta = 0xffffffff;
            es->score.data[es->score.length].status = 0xff;
            es->score.data[es->score.length].data[0] = 0x2f;
            es->score.length ++;

           i32 tmpbuf[BUFFER_LENGTH_PER_UPDATE];
            memset(tmpbuf, 0, sizeof(tmpbuf));
            if(es->midi_clock != 0)ks_score_data_render(&es->score, SAMPLING_RATE, es->score_state, es->tones, tmpbuf, BUFFER_LENGTH_PER_UPDATE);
            for(u32 i=0; i< BUFFER_LENGTH_PER_UPDATE; i++){
                es->buf[i] = tmpbuf[i] / (float)INT16_MAX;
            }

            if(es->midi_clock == 0){
                es->score_state->passed_tick = 0;
            }

        } else {
            memset(es->buf, 0, BUFFER_LENGTH_PER_UPDATE*sizeof(float));
        }
#else
        memset(es->buf, 0, BUFFER_LENGTH_PER_UPDATE*sizeof(float));
#endif

        i32 tmpbuf[BUFFER_LENGTH_PER_UPDATE];
        memset(tmpbuf, 0, sizeof(tmpbuf));
        ks_synth_render(&es->note, ks_1(KS_VOLUME_BITS), ks_1(KS_LFO_DEPTH_BITS), tmpbuf, BUFFER_LENGTH_PER_UPDATE);

        for(u32 i=0; i< BUFFER_LENGTH_PER_UPDATE; i++){
            es->buf[i] += tmpbuf[i] / (float)INT16_MAX;
        }
        UpdateAudioStream(es->audiostream, es->buf, BUFFER_LENGTH_PER_UPDATE);

    }

    if(es->display_mode== EDIT && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
        if(memcmp(&es->tones_data.data[es->current_tone_index].synth, &es->temp_synth, sizeof(ks_synth_data)) != 0 ){
            if(!es->dirty){
                mark_dirty(&es->dirty, &es->file_dialog_state);
                es->temp_synth = es->tones_data.data[es->current_tone_index].synth;
            }
        }
    }

    // Draw (parameters update)
    //----------------------------------------------------------------------------------
    BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        if(es->display_mode== EDIT) GuiEnable();
        else GuiDisable();

        Rectangle x_pos = base_pos;
        Rectangle pos = base_pos;
        Rectangle pos2;
        {
            pos2 = pos;
            pos2.width *= 0.85f;
            pos2.height *= 2;
            pos.y += step *2;

            float step_x2 = pos2.width + margin;
            if(GuiLabelButton(pos2, "#8#New")){
                es->display_mode= CONFIRM_NEW;
            }
            pos2.x += step_x2;
            if(GuiLabelButton(pos2, "#1#Open")){
#ifdef PLATFORM_WEB
                es->dialog_message = "Just drag and drop your .kstc or .krtb file";
                es->display_mode = INFO_DIALOG;

#else
                es->display_mode= LOAD_DIALOG;
                strcpy(es->file_dialog_state.titleText, "Load Tone List");
                es->file_dialog_state.fileDialogActiveState = DIALOG_ACTIVE;
#endif
            }
            pos2.x += step_x2;

            if(GuiLabelButton(pos2, "#2#Save")){
#ifdef PLATFORM_WEB
                strcpy(es->file_dialog_state.titleText, "Save Tone List");
                es->file_dialog_state.fileDialogActiveState = DIALOG_ACTIVE;
                es->display_mode= SAVE_DIALOG;
#else
                if(strcmp(es->file_dialog_state.fileNameText, "") == 0){
                    strcpy(es->file_dialog_state.titleText, "Save Tone List");
                    es->file_dialog_state.fileDialogActiveState = DIALOG_ACTIVE;
                    es->display_mode= SAVE_DIALOG;
                } else {
                    if(save_load_tone_list(&es->tones_data, es, true)){
                        es->temp_synth = es->tones_data.data[es->current_tone_index].synth;
                        update_tone_list(&es->tones, &es->tones_data, es->score_state);
                    }
                    else {
                        es->dialog_message = "Failed to save tone list";
                        es->display_mode = ERROR_DIALOG;
                    }
                }
#endif
            }
            pos2.x += step_x2;

            if(GuiLabelButton(pos2, "#3#Save As")){
                strcpy(es->file_dialog_state.titleText, "Save Tone List");
                es->file_dialog_state.fileDialogActiveState = DIALOG_ACTIVE;
                es->display_mode= SAVE_DIALOG;
            }
            pos2.x += step_x2;

            if(GuiLabelButton(pos2, "#6#LoadSynth")){
#ifdef PLATFORM_WEB
                es->dialog_message = "Just drag and drop your .ksyc or .kryb file";
                es->display_mode = INFO_DIALOG;
#else
                strcpy(es->file_dialog_state.titleText, "Load Synth");
                es->file_dialog_state_synth.fileDialogActiveState = DIALOG_ACTIVE;
                es->display_mode= IMPORT_SYNTH_DIALOG;
#endif
            }
            pos2.x += step_x2;
            if(GuiLabelButton(pos2, "#7#SaveSynth")){
                strcpy(es->file_dialog_state.titleText, "Save Synth");
                es->file_dialog_state_synth.fileDialogActiveState = DIALOG_ACTIVE;
                es->display_mode= EXPORT_SYNTH_DIALOG;
            }
            pos2.x += step_x2;

            if(GuiLabelButton(pos2, "#141#Settings")){
                es->display_mode= SETTINGS;
            }

            pos2.x += step_x2;

            if(GuiLabelButton(pos2, "#159#Exit")){
#ifdef PLATFORM_DESKTOP
                exit(0);
#else
                es->dialog_message = "This program cannot exit because it is a virus";
                es->display_mode = ERROR_DIALOG;

#endif
            }

            if(IsFileDropped()){
                int count;
                char** files =NULL;
                files = GetDroppedFiles(&count);

                if(count > 1){
                    es->dialog_message = "Please drag and drop only 1 file";
                    es->display_mode = ERROR_DIALOG;
                } else {
                    if(IsFileExtension(*files, tone_list_ext)){
                        strcpy(es->file_dialog_state.fileNameText, GetFileName(*files));
                        memset(es->file_dialog_state.dirPathText, 0 , sizeof(es->file_dialog_state.dirPathText));
                        strncpy(es->file_dialog_state.dirPathText, *files, strlen(*files) - strlen(es->file_dialog_state.fileNameText));

                        if(!save_load_tone_list(&es->tones_data , es, false)){
                            es->dialog_message = "Failed to load tone list";
                            es->display_mode = ERROR_DIALOG;
                        }
                    }
                    else if(IsFileExtension(*files, synth_ext)){
                        strcpy(es->file_dialog_state_synth.fileNameText, GetFileName(*files));
                        memset(es->file_dialog_state_synth.dirPathText, 0 , sizeof(es->file_dialog_state_synth.dirPathText));
                        strncpy(es->file_dialog_state_synth.dirPathText, *files, strlen(*files) - strlen(es->file_dialog_state_synth.fileNameText));

                        if(!save_load_synth(&es->tones_data.data[es->current_tone_index].synth, es, false)){
                            es->dialog_message = "Failed to load synth";
                            es->display_mode = ERROR_DIALOG;
                        } else {
                             update_tone_list(&es->tones, &es->tones_data, es->score_state);
                        }
                    }
                }
                ClearDroppedFiles();
            }
        }

        x_pos.y += step*2;

        pos.x = x_pos.x;
        // tone list
        {
            pos2 = pos;
            pos2.width = base_width*2;
            pos2.height = 300;
            int start, end;

            GuiListViewGetInfo(pos2, es->tones_data.length, es->tone_list_scroll, &start, &end);
            end = MIN(end + 1, (i32)es->tones_data.length);
            char *buf = calloc(es->tones_data.length, 48);
            char **ptr = malloc(sizeof(char**)*es->tones_data.length);
            for(i32 i=start; i< end; i++){
                if(es->tones_data.data[i].note == KS_NOTENUMBER_ALL){
                    snprintf(ptr[i] = buf + 48*i, 48, "0x%02x%02x%4d%4s%20s", es->tones_data.data[i].msb, es->tones_data.data[i].lsb, es->tones_data.data[i].program, "    ", es->tones_data.data[i].name);
                } else {
                    snprintf(ptr[i] = buf + 48*i, 48, "0x%02x%02x%4d%4d%20s", es->tones_data.data[i].msb, es->tones_data.data[i].lsb, es->tones_data.data[i].program, es->tones_data.data[i].note, es->tones_data.data[i].name);
                }
            }
            {

                const i32 new_select= GuiListViewEx(pos2, (const char**)ptr, es->tones_data.length, NULL, &es->tone_list_scroll, es->current_tone_index);
                if(new_select  != es->current_tone_index && new_select >= 0 && (u32)new_select < es->tones_data.length){
                    es->current_tone_index = new_select;
                    es->temp_synth = es->tones_data.data[es->current_tone_index].synth;
                    update_tone_list(&es->tones, &es->tones_data, es->score_state);
                }
            }
            free(buf);
            free(ptr);

            if(es->current_tone_index < 0 || (u32)es->current_tone_index >= es->tones_data.length){
                es->current_tone_index = 0;
            }

            pos.y += pos2.height + margin;

            pos2 = pos;
            pos2.width = base_width-margin;
            pos2.height *= 2.0f;

            if(es->tones_data.length >= 128*128*128) {
                GuiDisable();
            }
            if(GuiButton(pos2, "Add")){
                ks_tone_list_insert_empty(&es->tones_data, &es->current_tone_index);
                program_number_increment(&es->tones_data.data[es->current_tone_index].msb, &es->tones_data.data[es->current_tone_index].lsb, &es->tones_data.data[es->current_tone_index].program, &es->tones_data.data[es->current_tone_index].note);
                mark_dirty(&es->dirty, &es->file_dialog_state);
            }
            if(es->tones_data.length >= 128*128*128 && es->display_mode== EDIT){
                GuiEnable();
            }

            pos2.x += base_width;

            if(es->tones_data.length == 1) {
                GuiDisable();
            }
            if(GuiButton(pos2, "Remove")){
                ks_vector_remove(&es->tones_data, es->current_tone_index);
                es->current_tone_index --;
                if(es->current_tone_index < 0) es->current_tone_index = 0;
                mark_dirty(&es->dirty, &es->file_dialog_state);
            }
            if(es->tones_data.length == 1 && es->display_mode== EDIT){
                GuiEnable();
            }

             pos.y += pos2.height + margin;
             pos2.y = pos.y;
             pos2.x = pos.x;

             if(GuiButton(pos2, "Copy")){
                 ks_tone_list_insert(&es->tones_data, es->tones_data.data[es->current_tone_index], &es->current_tone_index);
                 mark_dirty(&es->dirty, &es->file_dialog_state);
             }

            pos2.x += base_width;
            if(GuiButton(pos2, "Sort")){
                ks_tone_list_sort(&es->tones_data, &es->current_tone_index);
                mark_dirty(&es->dirty, &es->file_dialog_state);
            }

            pos.y += pos2.height + margin;
            pos2.y = pos.y;
            pos2.width = (int)(pos.width*0.75f - margin);
            pos2.x = pos.x;

            int id = 0;
            int tmp_focus = -1;
            bool mouse_button_pressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            {
                GuiAlignedLabel("Name", pos2, GUI_TEXT_ALIGN_RIGHT);
                pos2.x += (int)(base_width*0.75f);
                pos2.width = (int)(pos.width * 1.25f);
                if(GuiTextBox(pos2, es->tones_data.data[es->current_tone_index].name, sizeof(es->tones_data.data[es->current_tone_index].name), es->testbox_focus == id)){
                    if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2)) tmp_focus = id;
                }
                id++;
                pos2.x = pos.x;
            }
            pos2.y += pos2.height + margin;
            pos2.x += (int)(pos.width*0.75f);
            {
                int tmp_val = es->tones_data.data[es->current_tone_index].msb;
                if(GuiValueBox(pos2, "MSB", &tmp_val, 0, 127, es->testbox_focus == id)){
                    if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2)) tmp_focus = id;
                }
                es->tones_data.data[es->current_tone_index].msb = tmp_val & 0x7f;
                id++;
            }
            pos2.y += pos2.height + margin;
            {
                int tmp_val = es->tones_data.data[es->current_tone_index].lsb;
                if(GuiValueBox(pos2, "LSB", &tmp_val, 0, 127, es->testbox_focus == id)){
                    if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2)) tmp_focus = id;
                }
                es->tones_data.data[es->current_tone_index].lsb = tmp_val & 0x7f;
                id++;
            }
            pos2.y += pos2.height + margin;
            {
                int tmp_val = es->tones_data.data[es->current_tone_index].program;
                if(GuiValueBox(pos2, "Program", &tmp_val, 0, 127, es->testbox_focus == id)){
                   if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2))  tmp_focus = id;
                }
                es->tones_data.data[es->current_tone_index].program = tmp_val & 0x7f;
                id++;
            }
            pos2.y += pos2.height + margin;
            {
                bool tmp = es->tones_data.data[es->current_tone_index].note != KS_NOTENUMBER_ALL;
                tmp = GuiCheckBox((Rectangle){pos2.x, pos2.y, base_pos.height, base_pos.height}, "Is Percussion", tmp);
                if(!tmp) {
                    es->tones_data.data[es->current_tone_index].note = KS_NOTENUMBER_ALL;
                } else {
                    es->tones_data.data[es->current_tone_index].note &= 0x7f;
                }
            }
            pos2.y += step;

            if(es->tones_data.data[es->current_tone_index].note != KS_NOTENUMBER_ALL){
                int tmp_val = es->tones_data.data[es->current_tone_index].note;
                if(GuiValueBox(pos2, "Note Number", &tmp_val, 0, 127, es->testbox_focus == id)){
                    if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2)) tmp_focus = id;
                }
                es->tones_data.data[es->current_tone_index].note = tmp_val & 0x7f;
                id++;
            }

            if(mouse_button_pressed){
                es->testbox_focus = tmp_focus;
            }

            if(es->display_mode ==EDIT){
                if(GetCharPressed() > 0){
                    mark_dirty(&es->dirty, &es->file_dialog_state);
                }
            }

            pos.y = x_pos.y;
        }
        x_pos.x += step_x*2;
        pos.x = x_pos.x;
        pos.y += (int)(step / 2.0);


        // common params
        {
            ks_synth_common_data * common = & es->tones_data.data[es->current_tone_index].synth.common;
            // output
            {
                pos2 = pos;
                pos2.height = 20;
                GuiAlignedLabel("Outputs", pos2, GUI_TEXT_ALIGN_RIGHT);
                pos2.x += step_x;
                common->output = PropertyIntImage(pos2, es->output_images, common->output, 0, KS_NUM_OPERATORS-1, 1);
            }
            pos.x = x_pos.x;
            pos.y += (int)pos2.height + margin;
            // feedback_level
            {
                GuiAlignedLabel("Feedback", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;

                pos2 = pos;
                pos2.width = (int)(pos2.width );

                text = FormatText("%.2f PI", 4.0f *calc_feedback_level(common->feedback_level) / ks_1(KS_FEEDBACK_LEVEL_BITS));
                common->feedback_level = PropertyInt(pos2, text, common->feedback_level, 0, 15, 1);


                pos.x += step_x;
            }
            pos.x = x_pos.x;
            pos.y += step + 2;
            // panpot
            {
                GuiAlignedLabel("Panpot", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;

                text = FormatText("%.3f", 2.0f * (calc_panpot(common->panpot)) / ks_1(KS_PANPOT_BITS) - 1.0f);
                common->panpot = PropertyInt(pos, text, common->panpot, 1, 15, 1);
            }
            pos.x = x_pos.x;
            pos.y += step;
            // wave_type
            {
                pos2 = pos;
                pos2.height = pos.height*2;
                GuiAlignedLabel("LFO Wave", pos2, GUI_TEXT_ALIGN_RIGHT);
                pos2.x += step_x;


                common->lfo_wave_type = PropertyIntImage(pos2, es->wave_images, common->lfo_wave_type, 0, KS_NUM_WAVES-1, 1);

            }
            pos.x = x_pos.x;
            pos.y += pos2.height + margin;
            // lfo_freq
            {
                GuiAlignedLabel("LFO Freqency", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;

                float hz = calc_lfo_freq(common->lfo_freq) / (float)ks_1(KS_FREQUENCY_BITS);
                if(hz >= 1.0f){
                    text = FormatText("%.1f Hz", hz);
                }
                else if(hz< 1.0f && hz>= 0.01f){
                    text = FormatText("%.1f mHz", hz*1000.0f);
                }
                else {
                    text = FormatText("%.3f mHz", hz*1000.0f);
                }


                common->lfo_freq = PropertyInt(pos, text, common->lfo_freq, 0, 31, 1);
            }
            pos.x = x_pos.x;
            pos.y += step;
            // lfo_det
            {
                GuiAlignedLabel("LFO Initial Phase", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;

                text = FormatText("%.1f deg", 360.0f *common->lfo_offset / (float)16);
                common->lfo_offset= PropertyInt(pos, text, common->lfo_offset, 0, 15, 1);
            }
            pos.x = x_pos.x;
            pos.y += step;
            // lfo_fms_depth
            {
                GuiAlignedLabel("LFO FMS", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;

                text = FormatText("%.3f %%", 100.0f *calc_lfo_fms_depth(common->lfo_fms_depth) / (float)ks_1(KS_LFO_DEPTH_BITS));
                common->lfo_fms_depth= PropertyInt(pos, text, common->lfo_fms_depth, 0, 31, 1);
            }
            pos.x = x_pos.x;
            pos.y += step;
        }

        // wave
        {
            Rectangle wave_rec ={x_pos.x + step_x * 2 + margin, x_pos.y, step_x*3 - margin, pos.y - x_pos.y };
            BeginScissorMode(wave_rec.x, wave_rec.y, wave_rec.width, wave_rec.height);

            DrawRectangleRec(wave_rec, DARKGRAY);

            float env_width =wave_rec.width*0.6f / KS_NUM_OPERATORS;
            float env_x = wave_rec.width*0.2f / KS_NUM_OPERATORS + wave_rec.x;
            float env_y = wave_rec.y + wave_rec.height;
            float env_step = wave_rec.width / KS_NUM_OPERATORS;
            float env_mul = wave_rec.height / ks_1(KS_ENVELOPE_BITS);

            for(unsigned i=0; i<KS_NUM_OPERATORS; i++){
                float w = env_mul* es->note.envelope_now_amps[i];
                Rectangle rec ={env_x + env_step*i, env_y - w, env_width, w};
                DrawRectangleRec(rec, BLUE);
                const char *text;
                if(es->note.envelope_states[i] == KS_ENVELOPE_RELEASED){
                    text = "Released";
                }
                else if(es->note.envelope_states[i] == KS_ENVELOPE_OFF) {
                    text = "Off";
                }
                else{
                    text = FormatText(es->note.envelope_now_points[i] == 0 ?
                                          "Attack" : es->note.envelope_now_points[i] == 1 ?
                                              "Decay" : "Sustain");
                }

                DrawText(text, rec.x+ 1, env_y -  11, 10, YELLOW);
            }

            int samp = SAMPLING_RATE  * 2/ 110.0f;
            float dx = wave_rec.width * 2.0 / samp;
            float x = 0.0f;
            float y =  wave_rec.y + wave_rec.height/2.0f ;
            float amp = wave_rec.height;

            for(int i=0; i<samp; i+=2){
                float base_x = wave_rec.x + x;

                DrawLineV((Vector2){base_x, y - ((i32)es->buf[i] + es->buf[i+1])*amp}, (Vector2){base_x+dx, y - ((i32)es->buf[i+2] + es->buf[i+3])*amp}, GREEN);
                x+= dx;
            }

            if(es->display_mode!= EDIT ){
                DrawRectangleRec(wave_rec, (Color){200,200,200,128});
            }

            EndScissorMode();
        }


        GuiGroupBox((Rectangle){pos.x, x_pos.y, step_x*2, pos.y - x_pos.y}, "Common Params");
        pos.y += (int)(step / 2.0f);
        x_pos.y = pos.y;

        // operators
        {
            ks_synth_operator_data *op = es->tones_data.data[es->current_tone_index].synth.operators;

            pos.y += (int)(step / 2.0f) ;

            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                pos.x += step_x;
                pos2 = pos;
                GuiAlignedLabel(FormatText("Op %d ", i+1), pos2, GUI_TEXT_ALIGN_LEFT);

                pos2.x += step_x;
            }
            pos.x = x_pos.x; pos.y += step +2;

            // wave type
            GuiAlignedLabel("Ocillator Type", pos, GUI_TEXT_ALIGN_RIGHT);
            pos.x += step_x;

            for(unsigned i=0; i<KS_NUM_OPERATORS; i++){
                pos2 = pos;
                pos2.width =pos2.height;
                const bool sync_less = op[i].mod_type == KS_MOD_PASS || op[i].mod_type == KS_MOD_LPF || op[i].mod_type == KS_MOD_HPF ;
                if(sync_less){
                    GuiDisable();
                }
                op[i].mod_sync = GuiCheckBox(pos2, "Sync", op[i].mod_sync);
                if(sync_less){
                    GuiEnable();
                }
                pos2.width =(int)(pos.width * 0.5) - 2;
                pos2.height += 2;
                pos2.x += pos2.width + margin;

                const bool in_less = op[i].mod_type == KS_MOD_PASS ;
                if(in_less){
                    GuiDisable();
                }
                op[i].mod_src = PropertyIntImage(pos2, es->operator_images,op[i].mod_src, 0, KS_NUM_OPERATORS-1, 1);
                if(in_less){
                    GuiEnable();
                }
                 pos.x += step_x;
            }

            pos.x = x_pos.x + step_x;
            pos.y += step;
            pos2 = pos;
            pos2.height = pos.height*2;
            pos2.width = step_x * 0.5 - margin;
            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){

               op[i].wave_type = PropertyIntImage(pos2, es->wave_images,op[i].wave_type, 0, KS_NUM_WAVES-1, 1);
                pos2.x += pos2.width + margin;
               op[i].mod_type = PropertyIntImage(pos2, es->mod_images,op[i].mod_type, 0, KS_NUM_MODS-1, 1);
                pos2.x += pos2.width + margin;
            }
            pos.x = x_pos.x; pos.y += pos2.height + margin;


            // fixed frequency

            // phase coarse
            GuiAlignedLabel("Phase Coarse", pos, GUI_TEXT_ALIGN_RIGHT);
            pos.x += step_x;

            pos2 = pos;
            pos2.width = step_x*0.5 - margin;

            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){

               op[i].fixed_frequency = GuiCheckBox((Rectangle){pos2.x, pos2.y, pos.height, pos.height}, "FixFQ",op[i].fixed_frequency);
                pos2.x += pos2.width + margin;

                if(op[i].fixed_frequency){
                    text = FormatText("%d Hz",calc_frequency_fixed(op[i].phase_coarse));
                }
                else {
                    text = FormatText("%.1f", calc_phase_coarses(op[i].phase_coarse) / 2.0f);
                }
               op[i].phase_coarse = PropertyInt(pos2, text,op[i].phase_coarse, 0, 63, 1);
                pos2.x += pos2.width + margin;
            }
            pos.x = x_pos.x; pos.y += step;

            // phase offset
            GuiAlignedLabel("Phase Offset", pos, GUI_TEXT_ALIGN_RIGHT);
            pos.x += step_x;
            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                text = FormatText("%.1f deg",op[i].phase_offset * 360.0f / 16.0f);
               op[i].phase_offset = PropertyInt(pos, text,op[i].phase_offset, 0, 15, 1);
                pos.x += step_x;
            }
            pos.x = x_pos.x; pos.y += step;

            // phase tune
            GuiAlignedLabel("Phase Tune", pos, GUI_TEXT_ALIGN_RIGHT);
            pos.x += step_x;
            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                if(op[i].fixed_frequency){
                    text = FormatText("x %.3f", calc_phase_tunes(op[i].phase_detune) * 9.0f / (float)ks_1(KS_PHASE_FINE_BITS) + 1.0f);
                }
                else {
                    text = FormatText("x %.3f", calc_phase_tunes(op[i].phase_detune) / (float)ks_1(KS_PHASE_FINE_BITS) + 1.0f);
                }
               op[i].phase_detune = PropertyInt(pos, text,op[i].phase_detune, 0, 127, 1);
                pos.x += step_x;
            }
             pos.x = x_pos.x; pos.y += step;

            // phase fine
            GuiAlignedLabel("Phase Fine", pos, GUI_TEXT_ALIGN_RIGHT);
            pos.x += step_x;
            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                text = FormatText("%d", (op[i].phase_fine - 8));
               op[i].phase_fine = PropertyInt(pos, text,op[i].phase_fine, 1, 15, 1);
                pos.x += step_x;
            }
             pos.x = x_pos.x; pos.y += step;

            // phase det
             GuiAlignedLabel("Level", pos, GUI_TEXT_ALIGN_RIGHT);
             pos.x += step_x;
            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                text = FormatText("%.1f", 100.0*calc_levels(op[i].level) / (float)ks_1(KS_LEVEL_BITS));
               op[i].level = PropertyInt(pos, text,op[i].level, 0, 127, 1);
                pos.x += step_x;
            }
            pos.x = x_pos.x; pos.y += step;

            {
                const char *envelope_texts[4] ={
                    "Envelope Attack",
                    "Decay",
                    "Sustain",
                    "Release",
                };

                const u32 amp_max = 7;
                const u32 time_max = 31;
                // envelope points and times
                for (u32 e = 0; e< KS_ENVELOPE_NUM_POINTS; e++){
                    GuiAlignedLabel(envelope_texts[e], pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    pos2 = pos;
                    pos2.width /= 2.0f;
                    for(unsigned i=0; i< KS_NUM_OPERATORS; i++) {
                        text = FormatText("%.3f", calc_envelope_points(op[i].envelopes[e].amp) / (float)ks_1(KS_ENVELOPE_BITS));
                       op[i].envelopes[e].amp = PropertyInt(pos2, text,op[i].envelopes[e].amp, 0, amp_max, 1);
                        pos2.x += step_x / 2.0f;

                        float sec = calc_envelope_times(op[i].envelopes[e].time) / (float)ks_1(16);
                        if(sec >= 1.0f){
                            text = FormatText("%.1f s", sec);
                        }
                        else if(sec< 1.0f && sec>= 0.01f){
                            text = FormatText("%.1f ms", sec*1000.0f);
                        }
                        else {
                            text = FormatText("%.3f ms", sec*1000.0f);
                        }

                       op[i].envelopes[e].time = PropertyInt(pos2, text,op[i].envelopes[e].time, 0, time_max, 1);
                        pos2.x += step_x / 2.0f;
                    }
                    pos.x = x_pos.x; pos.y += step;
                }

            }
            // rate scale
           GuiAlignedLabel("Rate Scale", pos, GUI_TEXT_ALIGN_RIGHT);
           pos.x += step_x;
           for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
               text = FormatText("%.1f %%", 100.0*calc_ratescales(op[i].ratescale) / (float)ks_1(KS_RATESCALE_BITS));
              op[i].ratescale = PropertyInt(pos, text,op[i].ratescale, 0, 7, 1);
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
               op[i].keyscale_left = PropertyIntImage(pos2, es->keyscale_left_images,op[i].keyscale_left, 0, KS_KEYSCALE_CURVE_NUM_TYPES-1, 1);
                pos2.x += pos2.width  +margin;
               op[i].keyscale_right = PropertyIntImage(pos2, es->keyscale_right_images,op[i].keyscale_right, 0, KS_KEYSCALE_CURVE_NUM_TYPES-1, 1);
                pos2.x +=  pos2.width +margin;
            }
            pos.x = x_pos.x; pos.y += (pos2.height + margin);

            // keyscale low depth
            GuiAlignedLabel("Keyscale Depth", pos, GUI_TEXT_ALIGN_RIGHT);
            pos.x += step_x;

            pos2 = pos;
            pos2.width /=2.0f;

            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                text = FormatText("%.1f %%", 100*calc_keyscale_low_depths(op[i].keyscale_low_depth) / (float)ks_1(KS_KEYSCALE_DEPTH_BITS));
               op[i].keyscale_low_depth = PropertyInt(pos2, text,op[i].keyscale_low_depth, 0, 15, 1);

                pos2.x += step_x/2.0f;

                text = FormatText("%.1f %%", 100*calc_keyscale_high_depths(op[i].keyscale_high_depth) / (float)ks_1(KS_KEYSCALE_DEPTH_BITS));
               op[i].keyscale_high_depth = PropertyInt(pos2, text,op[i].keyscale_high_depth, 0, 15, 1);
                pos2.x += step_x / 2.0f;
            }
            pos.x = x_pos.x; pos.y += step;


            // keyscale mid point
            GuiAlignedLabel("Keyscale Mid Key", pos, GUI_TEXT_ALIGN_RIGHT);
            pos.x += step_x;
            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                text = FormatText("%d", calc_keyscale_mid_points(op[i].keyscale_mid_point));
               op[i].keyscale_mid_point = PropertyInt(pos, text,op[i].keyscale_mid_point, 0, 31, 1);
                pos.x += step_x;
            }
            pos.x = x_pos.x; pos.y += step;

            // velocity sensitive
            GuiAlignedLabel("Velocity Sensitive", pos, GUI_TEXT_ALIGN_RIGHT);
            pos.x += step_x;
            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                text = FormatText("%.1f %%", 100*calc_velocity_sens(op[i].velocity_sens) / (float)ks_1(KS_VELOCITY_SENS_BITS));
               op[i].velocity_sens = PropertyInt(pos, text,op[i].velocity_sens, 0, 15, 1);
                pos.x += step_x;
            }
            pos.x = x_pos.x; pos.y += step;

            // lfo ams depth
            GuiAlignedLabel("LFO AMS", pos, GUI_TEXT_ALIGN_RIGHT);
            pos.x += step_x;
            for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                text = FormatText("%.1f %%", 100*calc_lfo_ams_depths(op[i].lfo_ams_depth) / (float)ks_1(KS_LFO_DEPTH_BITS));
               op[i].lfo_ams_depth = PropertyInt(pos, text,op[i].lfo_ams_depth, 0, 15, 1);
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

            i8 noteonw = -1;
            i8 noteonb = -1;
            i8 velocity = 0;

            float x = pos.x;
            float y = pos.y;
            for(int o=1; o<= oct; o++){
                const i8 whites[] = {0, 2, 4, 5, 7, 9, 11};
                const i8 blacks[] = {1, 3, 6, 8, 10};
                float x2 = x;
                const i8 offset = 60-(oct / 2)* 12 + o*12;
                Rectangle recs[12];

                for(int k=0; k<7; k++){
                    Rectangle rec = recs[whites[k]] = (Rectangle){x2, y, white.x, white.y};
                    i8 n = whites[k]+ offset;


                    DrawRectangleRec(rec, RAYWHITE);
                    BeginBlendMode(BLEND_MULTIPLIED);
                    if(n == es->noteon_number){
                        noteon_rec = rec;
                        DrawRectangleRec(rec, SKYBLUE);
                    }
                    if(n == 60) {
                        DrawCircle(rec.x + rec.width*0.5, rec.y + rec.height - rec.width*0.5, rec.width*0.3, PINK);
                    }
                    EndBlendMode();
                    DrawRectangleLinesEx(rec, 1, es->display_mode!= EDIT ? LIGHTGRAY : GRAY);


                    x2 += white.x;
                }

                x2 = x + white.x * 0.6;
                for(int k=0; k<5; k++){

                    Rectangle rec = recs[blacks[k]] = (Rectangle){x2, y, black.x, black.y};
                    i8 n = blacks[k]+ offset;



                     DrawRectangleRec(rec, es->display_mode!= EDIT ? LIGHTGRAY : DARKGRAY);
                     BeginBlendMode(BLEND_MULTIPLIED);
                     if(n == es->noteon_number){
                         noteon_rec = rec;
                         DrawRectangleRec(rec, SKYBLUE);
                     }
                     EndBlendMode();


                    x2 += (k != 1 && k != 4) ? white.x : white.x*2;
                }


                if(es->display_mode==EDIT){
                    for(int i=0; i<7; i++){
                        i8 n = offset + whites[i];
                        Vector2 mouse = GetMousePosition();
                        if(CheckCollisionPointRec(mouse, recs[whites[i]]) ){
                            if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
                                    noteonw = n;
                                    velocity = (mouse.y - recs[whites[i]].y)*127 / recs[whites[i]].height;
                            }
                        }
                    }
                    for(int i=0; i<5; i++){
                        i8 n = offset + blacks[i];
                        Vector2 mouse = GetMousePosition();
                        if(CheckCollisionPointRec(mouse, recs[blacks[i]]) ){
                            if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
                                    noteonb = n;
                                    velocity = (mouse.y - recs[blacks[i]].y)*127 / recs[blacks[i]].height;
                            }
                        }
                    }

                }
                x += white.x*7;
            }



            if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && es->noteon_number != -1){
                ks_synth_note_off(&es->note);
                es->noteon_number = -1;
            }

            if(noteonb != -1){
                if(noteonb != es->noteon_number){
                    ks_synth_set(&es->synth, SAMPLING_RATE, &es->tones_data.data[es->current_tone_index].synth);
                    ks_synth_note_on(&es->note, &es->synth, SAMPLING_RATE, noteonb, velocity);
                    es->noteon_number = noteonb;
                }
            } else if(noteonw != -1){
                if(noteonw != es->noteon_number){
                    ks_synth_set(&es->synth, SAMPLING_RATE, &es->tones_data.data[es->current_tone_index].synth);
                    ks_synth_note_on(&es->note, &es->synth, SAMPLING_RATE, noteonw, velocity);
                    es->noteon_number = noteonw;
                }
            }
        }

        GuiEnable();

        switch (es->display_mode) {

        case CONFIRM_NEW:{
            bool run_new = !es->dirty;

            if(es->dirty){
                Rectangle rec={0,0, 200, 100};
                rec.x = (SCREEN_WIDTH - rec.width)/ 2;
                rec.y = (SCREEN_HEIGHT - rec.height) / 2;
                int res = GuiMessageBox(rec, "Confirm", "Clear current and Create new ?" , "Yes;No");

                if(res == 0 || res ==2){
                    es->display_mode= EDIT;
                }
                else if(res == 1) {
                    run_new = true;
                }
            }

            if(run_new){
                strcpy(es->file_dialog_state.fileNameText, "");
                unmark_dirty(&es->dirty, &es->file_dialog_state);
                es->display_mode= EDIT;
                ks_vector_clear(&es->tones_data);
                ks_tone_list_insert_empty(&es->tones_data, &es->current_tone_index);
                es->current_tone_index = 0;
                es->temp_synth = es->tones_data.data[es->current_tone_index].synth;
            }
            break;
        }
        case LOAD_DIALOG:{
            GuiFileDialog(&es->file_dialog_state, false);
            if(es->file_dialog_state.fileDialogActiveState == DIALOG_DEACTIVE) {
                if(es->file_dialog_state.SelectFilePressed){
                    if(strcmp(es->file_dialog_state.fileNameText, "") == 0){
                        es->file_dialog_state.SelectFilePressed = false;
                        es->file_dialog_state.fileDialogActiveState = DIALOG_ACTIVE;
                        break;
                    }

                    ks_tone_list_data load;
                    if(!save_load_tone_list(&load, es, false)){
                        es->dialog_message = "Failed to load tone list";
                        es->display_mode = ERROR_DIALOG;
                    }else {
                        free(es->tones_data.data);
                        es->tones_data = load;
                        es->current_tone_index = 0;
                        es->temp_synth = es->tones_data.data[es->current_tone_index].synth;
                        update_tone_list(&es->tones, &es->tones_data, es->score_state);

                        es->display_mode= EDIT;
                    }
                }
                else {
                    es->display_mode= EDIT;
                }
            }
            break;
        }
        case SAVE_DIALOG:{
#ifdef PLATFORM_WEB
            const float lr_margin =  SCREEN_WIDTH * 0.35f;
            const float td_margin = SCREEN_HEIGHT * 0.4f;
            const Rectangle window ={ lr_margin, td_margin, SCREEN_WIDTH - lr_margin *2, SCREEN_HEIGHT - td_margin*2 };
            const int msgbox_res = GuiMessageBox(window, "Save Tone List", "", "OK;Cancel");
            if(msgbox_res == 1){
                if(save_load_tone_list(&es->tones_data, es, true)){
                    update_tone_list(&es->tones, &es->tones_data, es->score_state);
                    es->temp_synth = es->tones_data.data[es->current_tone_index].synth;
                    es->display_mode= EDIT;
                }
                else {
                    es->dialog_message = "Failed to save tone list";
                    es->display_mode = ERROR_DIALOG;
                }
            }
            else if(msgbox_res != -1){
                es->display_mode = EDIT;
            }
            const Rectangle labelrec = {window.x + margin*2, window.y + line_width*2 + margin * 5, window.width *0.3 - margin*4, line_width*2};
            const Rectangle textrec = {labelrec.x + labelrec.width + margin*2, window.y + line_width*2 + margin * 5, window.width *0.7 - margin*4, line_width*2};
            GuiLabel(labelrec, "File name");
            GuiTextBox(textrec, es->file_dialog_state.fileNameText, 10, true);
#else
            GuiFileDialog(&es->file_dialog_state, true);
            if(es->file_dialog_state.fileDialogActiveState == DIALOG_DEACTIVE){
                if(es->file_dialog_state.SelectFilePressed){
                    if(save_load_tone_list(&es->tones_data, es, true)){
                        update_tone_list(&es->tones, &es->tones_data, es->score_state);
                        es->temp_synth = es->tones_data.data[es->current_tone_index].synth;
                        es->display_mode= EDIT;
                    }
                    else {
                        es->dialog_message = "Failed to save tone list";
                        es->display_mode = ERROR_DIALOG;
                    }
                }
                else {
                    es->display_mode= EDIT;
                }
            }
#endif
            break;

        }
        case IMPORT_SYNTH_DIALOG:{
            GuiFileDialog(&es->file_dialog_state_synth, false);
            if(es->file_dialog_state_synth.fileDialogActiveState == DIALOG_DEACTIVE){
                if(es->file_dialog_state_synth.SelectFilePressed){
                    ks_synth_data tmp;
                    if(save_load_synth(&tmp, es, false)){
                        es->tones_data.data[es->current_tone_index].synth = es->temp_synth = tmp;
                        es->display_mode= EDIT;
                    }
                    else {
                        es->dialog_message = "Failed to load synth";
                        es->display_mode = ERROR_DIALOG;
                    }
                }
                else {
                    es->display_mode= EDIT;
                }

            }
            break;
        }
        case EXPORT_SYNTH_DIALOG:{
#ifdef PLATFORM_WEB
            const float lr_margin =  SCREEN_WIDTH * 0.35f;
            const float td_margin = SCREEN_HEIGHT * 0.4f;
            const Rectangle window ={ lr_margin, td_margin, SCREEN_WIDTH - lr_margin *2, SCREEN_HEIGHT - td_margin*2 };
            const int msgbox_res = GuiMessageBox(window, "Save Synth", "", "OK;Cancel");
            if(msgbox_res == 1){
                if(!save_load_synth(&es->tones_data.data[es->current_tone_index].synth, es, true)){
                    es->dialog_message = "Failed to save synth";
                    es->display_mode = ERROR_DIALOG;
                }
                else {
                    es->display_mode= EDIT;
                }
            }
            else if(msgbox_res != -1){
                es->display_mode = EDIT;
            }
            const Rectangle labelrec = {window.x + margin*2, window.y + line_width*2 + margin * 5, window.width *0.3 - margin*4, line_width*2};
            const Rectangle textrec = {labelrec.x + labelrec.width + margin*2, window.y + line_width*2 + margin * 5, window.width *0.7 - margin*4, line_width*2};
            GuiLabel(labelrec, "File name");
            GuiTextBox(textrec, es->file_dialog_state_synth.fileNameText, 10, true);
#else

            GuiFileDialog(&es->file_dialog_state_synth, true);
            if(es->file_dialog_state_synth.fileDialogActiveState == DIALOG_DEACTIVE){
                if(es->file_dialog_state_synth.SelectFilePressed){
                    if(!save_load_synth(&es->tones_data.data[es->current_tone_index].synth, es, true)){
                        es->dialog_message = "Failed to save synth";
                        es->display_mode = ERROR_DIALOG;
                    }
                    else {
                        es->display_mode= EDIT;
                    }
                }
                else {
                    es->display_mode= EDIT;
                }

            }
#endif
            break;
        }
        case ERROR_DIALOG:{
            const float lr_margin =  SCREEN_WIDTH * 0.3f;
            const float td_margin = SCREEN_HEIGHT * 0.4f;
            Rectangle window ={ lr_margin, td_margin, SCREEN_WIDTH - lr_margin *2, SCREEN_HEIGHT - td_margin*2 };
            if(GuiMessageBox(window, "Error", es->dialog_message, "OK") >= 0){
                es->display_mode = EDIT;
            }
            break;
        }
        case INFO_DIALOG:{
            const float lr_margin =  SCREEN_WIDTH * 0.3f;
            const float td_margin = SCREEN_HEIGHT * 0.4f;
            Rectangle window ={ lr_margin, td_margin, SCREEN_WIDTH - lr_margin *2, SCREEN_HEIGHT - td_margin*2 };
            if(GuiMessageBox(window, "Info", es->dialog_message, "OK") >= 0){
                es->display_mode = EDIT;
            }
            break;
        }
        case SETTINGS:{
#ifdef PLATFORM_WEB
            es->display_mode= EDIT;
#else
            const float lr_margin =  SCREEN_WIDTH * 0.2f;
            const float td_margin = SCREEN_HEIGHT * 0.28f;
            Rectangle window ={ lr_margin, td_margin, SCREEN_WIDTH - lr_margin *2, SCREEN_HEIGHT - td_margin*2 };
            if(GuiWindowBox(window, "#141#Settings")){
                es->display_mode= EDIT;
            }
            Rectangle cp = {window.x + margin*5, window.y + margin*2 + line_width*2, base_width, line_width};
            if(es->midiin == NULL){
                GuiDisable();
            }

            {
                GuiLabel(cp, "Midi Input Port");
                cp.y += step;

                const Rectangle listrec = {cp.x, cp.y, window.width - margin*10, step *10};
                const u32 midiin_count = es->midiin == NULL ? 0 : rtmidi_get_port_count(es->midiin);
                const char **midiin_list = malloc(sizeof(char*)*midiin_count);
                for(u32 i=0; i< midiin_count; i++){
                    midiin_list[i] = rtmidi_get_port_name(es->midiin, i);
                }
                const u32 new_midiin_port = GuiListViewEx(listrec, midiin_list, midiin_count, NULL, &es->midiin_list_scroll, es->midiin_port);
                if(new_midiin_port != es->midiin_port && new_midiin_port < midiin_count){
                    es->midiin_port = new_midiin_port;
                    rtmidi_close_port(es->midiin);
                    rtmidi_open_port(es->midiin, es->midiin_port, midiin_list[es->midiin_port]);
                }
                free(midiin_list);
            }
            if(es->midiin == NULL){
                GuiEnable();
            }
            cp.y += step*10 + margin;

            {
                const Rectangle button_rec = {window.x + window.width - margin*5 - cp.width, cp.y, cp.width, cp.height*2};
                if(GuiButton(button_rec, "Done")){
                    es->display_mode= EDIT;
                }
            }
#endif
        }

        default:
            break;
    }

    DrawCursor();
    DrawText(TextFormat("%2i FPS", GetFPS()), 1,SCREEN_HEIGHT - 11, 10, LIME);
    EndDrawing();
}

void init(editor_state* es){
    es->tone_list_scroll=0;
    es->testbox_focus = -1;
    es->current_tone_index =-1;
    memset(&es->note, 0, sizeof(es->note));
    es->noteon_number = -1;
    es->dirty = false;
    es->display_mode= EDIT;

    es->audiostream = InitAudioStream(SAMPLING_RATE, 32, NUM_CHANNELS);
    es->buf = malloc(sizeof(float)*BUFFER_LENGTH_PER_UPDATE);

    ks_vector_init(&es->tones_data);
    ks_tone_list_insert_empty(&es->tones_data, &es->current_tone_index);

    es->events[0].delta = 0xffffffff;
    es->events[0].status = 0xff;
    es->events[0].data[0] = 0x2f;
    es->score.data = es->events;
    es->score.length = 0;
    es->score.resolution = MIDIIN_RESOLUTION;

    es->score_state= ks_score_state_new(MIDIIN_POLYPHONY_BITS);

    update_tone_list(&es->tones, &es->tones_data, es->score_state);

    PlayAudioStream(es->audiostream);

    ks_synth_data_set_default(&es->tones_data.data[0].synth);
    ks_synth_data_set_default(&es->temp_synth);

    InitAudioDevice();
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "krsyn editor - noname");
    SetTargetFPS(60);

    GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, 14);
#ifdef PLATFORM_DESKTOP
    es->midi_clock = 0;
    es->last_event_time = -0.2;
    es->last_event_tick = -0.2*MIDIIN_RESOLUTION*2;
    enum RtMidiApi api;
    if(rtmidi_get_compiled_api(&api, 1) != 0){
        es->midiin = rtmidi_in_create(api, "C", sizeof(es->events) / sizeof(es->events[0]));
        rtmidi_open_port(es->midiin, 0, rtmidi_get_port_name(es->midiin, 0));
        es->midiin_port = 0;

    } else {
        es->midiin = NULL;
        es->midiin_port = 0;
    }
    es->midiin_list_scroll =0;
#endif

    es->file_dialog_state= InitGuiFileDialog(SCREEN_WIDTH*0.8, SCREEN_HEIGHT*0.8, es->file_dialog_state.dirPathText, false);
    strcpy(es->file_dialog_state.filterExt,  tone_list_ext);
    es->file_dialog_state_synth = InitGuiFileDialog(SCREEN_WIDTH*0.8, SCREEN_HEIGHT*0.8, es->file_dialog_state_synth.dirPathText, false);
    strcpy(es->file_dialog_state_synth.filterExt, synth_ext);

    es->output_images = LoadTexture("resources/images/outputs.png");
    es->operator_images= LoadTexture("resources/images/operators.png");
    es->wave_images= LoadTexture("resources/images/waves.png");
    es->mod_images = LoadTexture("resources/images/mods.png");
    es->keyscale_left_images = LoadTexture("resources/images/keyscale_curves_l.png");
    es->keyscale_right_images =  LoadTexture("resources/images/keyscale_curves_r.png");

}

void deinit(editor_state* es){
    //----------------------------------------------------------------------------------
    // unload
    UnloadTexture(es->output_images);
    UnloadTexture(es->operator_images);
    UnloadTexture(es->wave_images);
    UnloadTexture(es->mod_images);
    UnloadTexture(es->keyscale_left_images);
    UnloadTexture(es->keyscale_right_images);

    CloseWindow();
    CloseAudioDevice();

    ks_score_state_free(es->score_state);
#ifdef PLATFORM_DESKTOP
    if(es->midiin != NULL){
        rtmidi_close_port(es->midiin);
        rtmidi_in_free(es->midiin);
    }
#endif
    free(es->buf);
    free(es->tones_data.data);
    ks_tone_list_free(es->tones);
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    //--------------------------------------------------------------------------------------
    // editor state
    editor_state es = { 0 };

    //--------------------------------------------------------------------------------------
    // editor init
    init(&es);


    //--------------------------------------------------------------------------------------
    // argments
    if(argc == 2){
        if(!FileExists(argv[1])){
            es.dialog_message = "File not exists.";
            es.display_mode = ERROR_DIALOG;
        }
        else if(IsFileExtension(argv[1], tone_list_ext)){
            const char* filename = GetFileName(argv[1]);
            strcpy(es.file_dialog_state.fileNameText, filename);
            memset(es.file_dialog_state.dirPathText, 0 , sizeof(es.file_dialog_state.dirPathText));
            //for unix file system
            if(argv[1][0] == '/'){
                strncpy(es.file_dialog_state.dirPathText, argv[1], strlen(argv[1]) - strlen(filename));
            }
            else {
                const char* dir = GetDirectoryPath(argv[1]);
                strcpy(es.file_dialog_state.dirPathText, dir);
            }

            if(!save_load_tone_list(&es.tones_data, &es, false)){
                es.dialog_message = "Failed to load tone list";
                es.display_mode = ERROR_DIALOG;
            } else {
                es.current_tone_index = 0;
                es.temp_synth = es.tones_data.data[es.current_tone_index].synth;
                update_tone_list(&es.tones, &es.tones_data, es.score_state);
            }
        }
        else if(IsFileExtension(argv[1], synth_ext)){
            const char* filename = GetFileName(argv[1]);
            strcpy(es.file_dialog_state_synth.fileNameText, filename);
            memset(es.file_dialog_state_synth.dirPathText, 0 , sizeof(es.file_dialog_state_synth.dirPathText));
            //for unix file system
            if(argv[1][0] == '/'){
                strncpy(es.file_dialog_state_synth.dirPathText, argv[1], strlen(argv[1]) - strlen(filename));
            }
            else {
                const char* dir = GetDirectoryPath(argv[1]);
                strcpy(es.file_dialog_state_synth.dirPathText, dir);
            }

            if(!save_load_synth(&es.tones_data.data->synth, &es, false)){
                es.dialog_message = "Failed to load synth";
                es.display_mode = ERROR_DIALOG;
            } else{
                update_tone_list(&es.tones, &es.tones_data, es.score_state);
            }
        }
        else {
            es.dialog_message = "Invalid file type.";
            es.display_mode = ERROR_DIALOG;
        }
    }

#ifdef PLATFORM_WEB
    SetExitKey(0);
#endif
    HideCursor();
#ifdef PLATFORM_DESKTOP
    SetTargetFPS(60);
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        EditorUpdate(&es);

    }
#else
    emscripten_set_main_loop_arg(EditorUpdate, &es, 0, 1);
#endif



   deinit(&es);

    return 0;
}


