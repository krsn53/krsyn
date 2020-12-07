#include "raygui_impl.h"
#include "rtmidi/rtmidi_c.h"

#define SCREEN_WIDTH 730
#define SCREEN_HEIGHT 565

#define SAMPLING_RATE               48000
#define SAMPLES_PER_UPDATE          4096
#define BUFFER_LENGTH_PER_UPDATE    (SAMPLES_PER_UPDATE*NUM_CHANNELS)
#define TIME_PER_UPDATE             ((double)SAMPLES_PER_UPDATE / SAMPLING_RATE)
#define NUM_CHANNELS                2
#define MIDIIN_RESOLUTION           480
#define MIDIIN_POLYPHONY_BITS       6

#include "../krsyn.h"
#include "../krsyn/vector.h"

//------------------------------------------------------------------------------------
bool SaveLoadSynth(ks_synth_data* bin, GuiFileDialogState* file_dialog_state, bool serialize){
    ks_io * io = ks_io_new();
    bool text_format = false;
    if(IsFileExtension(file_dialog_state->fileNameText, ".ksyc")){
        text_format= true;
    } else {
        if(!IsFileExtension(file_dialog_state->fileNameText, ".ksyb")){
            strcpy(file_dialog_state->fileNameText + strlen(file_dialog_state->fileNameText), ".ksyb");
        }
    }

    if(!serialize){
        ks_io_read_file(io, FormatText("%s/%s", file_dialog_state->dirPathText, file_dialog_state->fileNameText));
    }

    bool ret;
    if(text_format){
        ret = serialize ? ks_io_begin_serialize(io, clike, ks_prop_root(*bin, ks_synth_data)) :
                          ks_io_begin_deserialize(io, clike, ks_prop_root(*bin, ks_synth_data)) ;
    }
     else {
        ret = serialize ? ks_io_begin_serialize(io, binary_little_endian, ks_prop_root(*bin, ks_synth_data)) :
                          ks_io_begin_deserialize(io, binary_little_endian, ks_prop_root(*bin, ks_synth_data)) ;
    }

    if(serialize){
        SaveFileData(FormatText("%s/%s", file_dialog_state->dirPathText, file_dialog_state->fileNameText), io->str->data, io->str->length);
    }

    ks_io_free(io);

    return ret;
}

bool SaveLoadToneList(ks_tone_list_data* bin, GuiFileDialogState* file_dialog_state, bool serialize){
    ks_io * io = ks_io_new();
    bool text_format = false;
    if(IsFileExtension(file_dialog_state->fileNameText, ".kstc")){
        text_format= true;
    } else {
        if(!IsFileExtension(file_dialog_state->fileNameText, ".kstb")){
            strcpy(file_dialog_state->fileNameText + strlen(file_dialog_state->fileNameText), ".kstb");
        }
    }

    if(!serialize){
        ks_io_read_file(io, FormatText("%s/%s", file_dialog_state->dirPathText, file_dialog_state->fileNameText));
    }

    bool ret;
    if(text_format){
        ret = serialize ? ks_io_begin_serialize(io, clike, ks_prop_root(*bin, ks_tone_list_data)) :
                          ks_io_begin_deserialize(io, clike, ks_prop_root(*bin, ks_tone_list_data)) ;
    }
     else {
        ret = serialize ? ks_io_begin_serialize(io, binary_little_endian, ks_prop_root(*bin, ks_tone_list_data)) :
                          ks_io_begin_deserialize(io, binary_little_endian, ks_prop_root(*bin, ks_tone_list_data)) ;
    }

    if(serialize){
        SaveFileData(FormatText("%s/%s", file_dialog_state->dirPathText, file_dialog_state->fileNameText), io->str->data, io->str->length);
    }

    ks_io_free(io);

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

static void update_tone_list(ks_tone_list ** ptr, ks_tone_list_data* dat, ks_score_state* score_state){
    if(*ptr != NULL)ks_tone_list_free(*ptr);
    *ptr = ks_tone_list_new_from_data(SAMPLING_RATE ,dat);
    u32 tmptick = score_state->current_tick;
    ks_score_state_set_default(score_state, *ptr, SAMPLING_RATE, MIDIIN_RESOLUTION);
    score_state->current_tick = tmptick;
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    static const char* tone_list_ext = ".kstb;.kstc";
    static const char* synth_ext = ".ksyb;.ksyc";
    //--------------------------------------------------------------------------------------
    // editor state
    ks_tone_list_data tones_data;
    ks_tone_list *tones;
    int tone_list_scroll=0;
    int textbox_focus = -1;
    i32 current =-1;
    ks_synth_data temp;
    ks_synth synth;
    ks_synth_note note = { 0 };

    ks_score_data score;
    ks_score_state* score_state;
    const int update_per_events = MIDIIN_RESOLUTION / 0.5f * SAMPLES_PER_UPDATE / SAMPLING_RATE;
    double  time;
    ks_score_event events[KS_NUM_CHANNELS * 2 * update_per_events];

    i8 noteon_number = -1;
    bool dirty = false;
    GuiFileDialogState file_dialog_state, file_dialog_state_synth;
    enum{
        EDIT,
        CONFIRM_NEW,
        SAVE_DIALOG,
        LOAD_DIALOG,
        IMPORT_SYNTH_DIALOG,
        EXPORT_SYNTH_DIALOG,
        SETTINGS,
    }state = EDIT;

    AudioStream audiostream;
    i16 *buf;

    RtMidiInPtr midiin;
    u32 midiin_port;
    int midiin_list_scroll =0;

    //--------------------------------------------------------------------------------------
    // editor state init
    audiostream = InitAudioStream(SAMPLING_RATE, 16, NUM_CHANNELS);
    buf = malloc(sizeof(i16)*BUFFER_LENGTH_PER_UPDATE);

    ks_vector_init(&tones_data);
    ks_tone_list_insert_empty(&tones_data, &current);

    events[0].delta = 0xffffffff;
    events[0].status = 0xff;
    events[0].data[0] = 0x2f;
    score.data = events;
    score.length = 0;
    score.resolution = MIDIIN_RESOLUTION;

    score_state = ks_score_state_new(MIDIIN_POLYPHONY_BITS);
    time = 0;

    update_tone_list(&tones, &tones_data, score_state);

    PlayAudioStream(audiostream);

    ks_synth_data_set_default(&tones_data.data[0].synth);
    ks_synth_data_set_default(&temp);

    InitAudioDevice();
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "krsyn editor - noname");
    SetTargetFPS(60);

    GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, 14);

    enum RtMidiApi api;
    if(rtmidi_get_compiled_api(&api, 1) != 0){
        midiin = rtmidi_in_create(api, "C", sizeof(events) / sizeof(events[0]));
        rtmidi_open_port(midiin, 0, rtmidi_get_port_name(midiin, 0));
        midiin_port = 0;

    } else {
        midiin = NULL;
        midiin_port = 0;
    }


    //--------------------------------------------------------------------------------------
    const int margin = 3;
    const int base_width = 100;
    Rectangle base_pos = (Rectangle) { margin, margin, base_width,  GetLineWidth() };

    const char* text;

    const float line_width = 12;
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

    file_dialog_state = InitGuiFileDialog(SCREEN_WIDTH*0.8, SCREEN_HEIGHT*0.8, file_dialog_state.dirPathText, false);
    strcpy(file_dialog_state.filterExt, tone_list_ext);
    file_dialog_state_synth = InitGuiFileDialog(SCREEN_WIDTH*0.8, SCREEN_HEIGHT*0.8, file_dialog_state_synth.dirPathText, false);
    strcpy(file_dialog_state_synth.filterExt, synth_ext);


    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if(IsAudioStreamProcessed(audiostream)){
            if(midiin != NULL){
                score.length = 0;

                while(score.data[score_state->current_event].delta != 0xffffffff){
                    score.data[score.length] = score.data[score_state->current_event];
                    score.length ++;
                    score_state->current_event++;
                }

                score_state->current_event = 0;


                size_t size;
                do{
                    unsigned char message[4];
                    size = 4;
                    double stamp = rtmidi_in_get_message(midiin, message, &size);
                    if(size != 0){
                        u64 pre_time = time*MIDIIN_RESOLUTION*2;
                        time += stamp;

                        if(score.length >= (sizeof(events)/ sizeof(events[0])) - 1) continue;
                        if((message[0] == 0xff && message[1] != 0x51) || (message[0] >=0x80 && message[0] < 0xf0)){
                            score.data[score.length].delta = (u32)((time)*MIDIIN_RESOLUTION*2) - pre_time;
                            score.data[score.length].status = message[0];
                            memcpy(score.data[score.length].data, message + 1, 3);
                            score.length ++;
                        }
                    }
                }while(size != 0);

                score.data[score.length].delta = 0xffffffff;
                score.data[score.length].status = 0xff;
                score.data[score.length].data[0] = 0x2f;
                score.length ++;

                ks_score_data_render(&score, SAMPLING_RATE, score_state, tones, buf, BUFFER_LENGTH_PER_UPDATE);
            } else {
                memset(buf, 0, BUFFER_LENGTH_PER_UPDATE*sizeof(i16));
            }

            i16 tmpbuf[BUFFER_LENGTH_PER_UPDATE];
            memset(tmpbuf, 0, sizeof(tmpbuf));
            ks_synth_render(&synth, &note, ks_1(KS_LFO_DEPTH_BITS), tmpbuf, BUFFER_LENGTH_PER_UPDATE);
            for(u32 i=0; i< BUFFER_LENGTH_PER_UPDATE; i++){
                buf[i] += tmpbuf[i];
            }
            UpdateAudioStream(audiostream, buf, BUFFER_LENGTH_PER_UPDATE);
        }

        if(state == EDIT && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
            if(memcmp(&tones_data.data[current].synth, &temp, sizeof(ks_synth_data)) != 0 ){
                if(!dirty){
                    mark_dirty(&dirty, &file_dialog_state);
                    temp = tones_data.data[current].synth;
                }
            }
        }

        // Draw (parameters update)
        //----------------------------------------------------------------------------------
        BeginDrawing();
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            if(state == EDIT) GuiEnable();
            else GuiDisable();

            Rectangle x_pos = base_pos;
            Rectangle pos = base_pos;
            Rectangle pos2;
            {
                pos2 = pos;
                pos2.width *= 0.85f;
                pos2.height *= 2;
                pos.y += step *2;

                float step_x = pos2.width + margin;
                if(GuiLabelButton(pos2, "#8#New")){
                    state = CONFIRM_NEW;
                }
                pos2.x += step_x;

                if(GuiLabelButton(pos2, "#1#Open")){
                    state = LOAD_DIALOG;
                    strcpy(file_dialog_state.titleText, "Load Tone List");
                    file_dialog_state.fileDialogActiveState = DIALOG_ACTIVE;
                }
                pos2.x += step_x;

                if(GuiLabelButton(pos2, "#2#Save")){
                    if(strcmp(file_dialog_state.fileNameText, "") == 0){
                        strcpy(file_dialog_state.titleText, "Save Tone List");
                        file_dialog_state.fileDialogActiveState = DIALOG_ACTIVE;
                        strcpy(file_dialog_state.filterExt, tone_list_ext);
                        state = SAVE_DIALOG;
                    } else {
                        if(SaveLoadToneList(&tones_data, &file_dialog_state, true)){
                            unmark_dirty(&dirty, &file_dialog_state);
                            update_tone_list(&tones, &tones_data, score_state);
                        }
                    }
                }
                pos2.x += step_x;

                if(GuiLabelButton(pos2, "#3#Save As")){
                    strcpy(file_dialog_state.titleText, "Save Tone List");
                    file_dialog_state.fileDialogActiveState = DIALOG_ACTIVE;
                    state = SAVE_DIALOG;
                }
                pos2.x += step_x;

                if(GuiLabelButton(pos2, "#6#LoadSynth")){
                    strcpy(file_dialog_state.titleText, "Load Synth");
                    file_dialog_state_synth.fileDialogActiveState = DIALOG_ACTIVE;
                    state = IMPORT_SYNTH_DIALOG;
                }
                pos2.x += step_x;

                if(GuiLabelButton(pos2, "#7#SaveSynth")){
                    strcpy(file_dialog_state.titleText, "Save Synth");
                    file_dialog_state_synth.fileDialogActiveState = DIALOG_ACTIVE;
                    state = EXPORT_SYNTH_DIALOG;
                }
                pos2.x += step_x;

                if(GuiLabelButton(pos2, "#141#Settings")){
                    state = SETTINGS;
                }

                pos2.x += step_x;


                if(GuiLabelButton(pos2, "#159#Exit")){
                    break;
                }
            }

            x_pos.y += step*2;

            pos.x = x_pos.x;
            // tone list
            {
                pos2 = pos;
                pos2.width = base_width*2;
                pos2.height = 300;
                char str[tones_data.length][40];
                int start, end;

                GuiListViewGetInfo(pos2, tones_data.length, tone_list_scroll, &start, &end);

                const char *ptr[tones_data.length];
                for(i32 i=start; i< end; i++){
                    if(tones_data.data[i].note == KS_NOTENUMBER_ALL){
                        snprintf(str[i], 48, "0x%02x%02x%4d%4s%20s", tones_data.data[i].msb, tones_data.data[i].lsb, tones_data.data[i].program, "    ", tones_data.data[i].name);
                    } else {
                        snprintf(str[i], 48, "0x%02x%02x%4d%4d%20s", tones_data.data[i].msb, tones_data.data[i].lsb, tones_data.data[i].program, tones_data.data[i].note, tones_data.data[i].name);
                    }
                }

                for(u32 i=0; i<tones_data.length; i++){
                    ptr[i] = str[i];
                }
                {

                    const i32 new_select= GuiListViewEx(pos2, ptr, tones_data.length, NULL, &tone_list_scroll, current);
                    if(new_select  != current && new_select >= 0 && (u32)new_select < tones_data.length){
                        current = new_select;
                        temp = tones_data.data[current].synth;
                        update_tone_list(&tones, &tones_data, score_state);
                    }
                }
                if(current < 0 || (u32)current >= tones_data.length){
                    current = 0;
                }

                pos.y += pos2.height + margin;

                pos2 = pos;
                pos2.width = base_width-margin;
                pos2.height *= 2.0f;

                if(tones_data.length >= 128*128*128) {
                    GuiDisable();
                }
                if(GuiButton(pos2, "Add")){
                    ks_tone_list_insert_empty(&tones_data, &current);
                    program_number_increment(&tones_data.data[current].msb, &tones_data.data[current].lsb, &tones_data.data[current].program, &tones_data.data[current].note);
                    mark_dirty(&dirty, &file_dialog_state);
                }
                if(tones_data.length >= 128*128*128 && state == EDIT){
                    GuiEnable();
                }

                pos2.x += base_width;

                if(tones_data.length == 1) {
                    GuiDisable();
                }
                if(GuiButton(pos2, "Remove")){
                    ks_vector_remove(&tones_data, current);
                    current --;
                    if(current < 0) current = 0;
                    mark_dirty(&dirty, &file_dialog_state);
                }
                if(tones_data.length == 1 && state == EDIT){
                    GuiEnable();
                }

                 pos.y += pos2.height + margin;
                 pos2.y = pos.y;
                 pos2.x = pos.x;

                 if(GuiButton(pos2, "Copy")){
                     ks_tone_list_insert(&tones_data, tones_data.data[current], &current);
                     mark_dirty(&dirty, &file_dialog_state);
                 }

                pos2.x += base_width;
                if(GuiButton(pos2, "Sort")){
                    ks_tone_list_sort(&tones_data, &current);
                    mark_dirty(&dirty, &file_dialog_state);
                }

                pos.y += pos2.height + margin;
                pos2.y = pos.y;
                pos2.width = pos.width*0.75f - margin;
                pos2.x = pos.x;

                int id = 0;
                int tmp_focus = -1;
                bool mouse_button_pressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
                {
                    GuiAlignedLabel("Name", pos2, GUI_TEXT_ALIGN_RIGHT);
                    pos2.x += base_width*0.75f;
                    pos2.width = pos.width * 1.25f;
                    if(GuiTextBox(pos2, tones_data.data[current].name, sizeof(tones_data.data[current].name), textbox_focus == id)){
                        if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2)) tmp_focus = id;
                    }
                    id++;
                    pos2.x = pos.x;
                }
                pos2.y += pos2.height + margin;
                pos2.x += pos.width*0.75f;
                {
                    int tmp_val = tones_data.data[current].msb;
                    if(GuiValueBox(pos2, "MSB", &tmp_val, 0, 127, textbox_focus == id)){
                        if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2)) tmp_focus = id;
                    }
                    tones_data.data[current].msb = tmp_val & 0x7f;
                    id++;
                }
                pos2.y += pos2.height + margin;
                {
                    int tmp_val = tones_data.data[current].lsb;
                    if(GuiValueBox(pos2, "LSB", &tmp_val, 0, 127, textbox_focus == id)){
                        if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2)) tmp_focus = id;
                    }
                    tones_data.data[current].lsb = tmp_val & 0x7f;
                    id++;
                }
                pos2.y += pos2.height + margin;
                {
                    int tmp_val = tones_data.data[current].program;
                    if(GuiValueBox(pos2, "Program", &tmp_val, 0, 127, textbox_focus == id)){
                       if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2))  tmp_focus = id;
                    }
                    tones_data.data[current].program = tmp_val & 0x7f;
                    id++;
                }
                pos2.y += pos2.height + margin;
                {
                    bool tmp = tones_data.data[current].note != KS_NOTENUMBER_ALL;
                    tmp = GuiCheckBox((Rectangle){pos2.x, pos2.y, base_pos.height, base_pos.height}, "Is Percussion", tmp);
                    if(!tmp) {
                        tones_data.data[current].note = KS_NOTENUMBER_ALL;
                    } else {
                        tones_data.data[current].note &= 0x7f;
                    }
                }
                pos2.y += step;

                if(tones_data.data[current].note != KS_NOTENUMBER_ALL){
                    int tmp_val = tones_data.data[current].note;
                    if(GuiValueBox(pos2, "Note Number", &tmp_val, 0, 127, textbox_focus == id)){
                        if(mouse_button_pressed && CheckCollisionPointRec(GetMousePosition(), pos2)) tmp_focus = id;
                    }
                    tones_data.data[current].note = tmp_val & 0x7f;
                    id++;
                }

                if(mouse_button_pressed){
                    textbox_focus = tmp_focus;
                }

                pos.y = x_pos.y;
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
                    tones_data.data[current].synth.algorithm = PropertyIntImage(pos2, algorithm_images, tones_data.data[current].synth.algorithm, 0, 10, 1);
                }
                pos.x = x_pos.x;
                pos.y += pos2.height + margin;
                // feedback_level
                {
                    GuiAlignedLabel("Feedback", pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    text = FormatText("%.3f PI", 2.0f *calc_feedback_level(tones_data.data[current].synth.feedback_level) / ks_1(KS_FEEDBACK_LEVEL_BITS));
                    tones_data.data[current].synth.feedback_level = PropertyInt(pos, text, tones_data.data[current].synth.feedback_level, 0, 255, 1);
                }
                pos.x = x_pos.x;
                pos.y += step;
                // panpot
                {
                    GuiAlignedLabel("Panpot", pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    text = FormatText("%.3f", 2.0f * calc_panpot(tones_data.data[current].synth.panpot) / ks_1(KS_PANPOT_BITS) - 1.0f);
                    tones_data.data[current].synth.panpot = PropertyInt(pos, text, tones_data.data[current].synth.panpot, 0, 127, 1);
                }
                pos.x = x_pos.x;
                pos.y += step;
                // lfo_wave_type
                {
                    pos2 = pos;
                    pos2.height = pos.height*2;
                    GuiAlignedLabel("LFO Wave", pos2, GUI_TEXT_ALIGN_RIGHT);
                    pos2.x += step_x;


                    tones_data.data[current].synth.lfo_wave_type = PropertyIntImage(pos2, lfo_wave_images, tones_data.data[current].synth.lfo_wave_type, 0, KS_LFO_NUM_WAVES-1, 1);

                }
                pos.x = x_pos.x;
                pos.y += pos2.height + margin;
                // lfo_freq
                {
                    GuiAlignedLabel("LFO Freqency", pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    float hz = calc_lfo_freq(tones_data.data[current].synth.lfo_freq) / (float)ks_1(KS_FREQUENCY_BITS);
                    if(hz >= 1.0f){
                        text = FormatText("%.1f Hz", hz);
                    }
                    else if(hz< 1.0f && hz>= 0.01f){
                        text = FormatText("%.1f mHz", hz*1000.0f);
                    }
                    else {
                        text = FormatText("%.3f mHz", hz*1000.0f);
                    }


                    tones_data.data[current].synth.lfo_freq = PropertyInt(pos, text, tones_data.data[current].synth.lfo_freq, 0, 255, 1);
                }
                pos.x = x_pos.x;
                pos.y += step;
                // lfo_det
                {
                    GuiAlignedLabel("LFO Initial Phase", pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    text = FormatText("%.1f deg", 360.0f *calc_lfo_det(tones_data.data[current].synth.lfo_det) / (float)ks_1(KS_PHASE_MAX_BITS));
                    tones_data.data[current].synth.lfo_det = PropertyInt(pos, text, tones_data.data[current].synth.lfo_det, 0, 255, 1);
                }
                pos.x = x_pos.x;
                pos.y += step;
                // lfo_fms_depth
                {
                    GuiAlignedLabel("LFO FMS", pos, GUI_TEXT_ALIGN_RIGHT);
                    pos.x += step_x;

                    text = FormatText("%.3f %%", 100.0f *calc_lfo_fms_depth(tones_data.data[current].synth.lfo_fms_depth) / (float)ks_1(KS_LFO_DEPTH_BITS));
                    tones_data.data[current].synth.lfo_fms_depth= PropertyInt(pos, text, tones_data.data[current].synth.lfo_fms_depth, 0, 240, 1);
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
                float dx = wave_rec.width * 2.0 / samp;
                float x = 0.0f;
                float y =  wave_rec.y + wave_rec.height/2.0f ;
                float amp = wave_rec.height * 0.5f /2.0f / INT16_MAX;

                for(int i=0; i<samp; i+=2){
                    float base_x = wave_rec.x + x;

                    DrawLineV((Vector2){base_x, y - ((i32)buf[i] + buf[i+1])*amp}, (Vector2){base_x+dx, y - ((i32)buf[i+2] + buf[i+3])*amp}, GREEN);
                    x+= dx;
                }

                if(state != EDIT ){
                    DrawRectangleRec(wave_rec, (Color){200,200,200,128});
                }
            }


            GuiGroupBox((Rectangle){pos.x, x_pos.y, step_x*2, pos.y - x_pos.y}, "Common Params");
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
                    tones_data.data[current].synth.phase_coarses.st[i].fixed_frequency = GuiCheckBox((Rectangle){pos.x, pos.y, pos.height, pos.height}, "", tones_data.data[current].synth.phase_coarses.st[i].fixed_frequency);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;

                // phase coarse
                GuiAlignedLabel("Phase Coarse", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    if(tones_data.data[current].synth.phase_coarses.st[i].fixed_frequency){
                        text = FormatText("%.1f Hz", ks_notefreq(tones_data.data[current].synth.phase_coarses.st[i].value) / (float)ks_1(KS_FREQUENCY_BITS));
                    }
                    else {
                        text = FormatText("%.1f", calc_phase_coarses(tones_data.data[current].synth.phase_coarses.st[i].value) / 2.0f);
                    }
                    tones_data.data[current].synth.phase_coarses.st[i].value = PropertyInt(pos, text, tones_data.data[current].synth.phase_coarses.st[i].value, 0, 127, 1);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;

                // phase tune
                GuiAlignedLabel("Phase Tune", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%d", (tones_data.data[current].synth.phase_tunes[i] - 127));
                    tones_data.data[current].synth.phase_tunes[i] = PropertyInt(pos, text, tones_data.data[current].synth.phase_tunes[i], 0, 255, 1);
                    pos.x += step_x;
                }
                 pos.x = x_pos.x; pos.y += step;

                // phase fine
                GuiAlignedLabel("Phase Fine", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.3f", calc_phase_fines(tones_data.data[current].synth.phase_fines[i]) / (float)ks_v(2, KS_PHASE_FINE_BITS));
                    tones_data.data[current].synth.phase_fines[i] = PropertyInt(pos, text, tones_data.data[current].synth.phase_fines[i], 0, 255, 1);
                    pos.x += step_x;
                }
                 pos.x = x_pos.x; pos.y += step;

                // phase det
                 GuiAlignedLabel("Inital Phase", pos, GUI_TEXT_ALIGN_RIGHT);
                 pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.1f deg", 360.0*calc_phase_dets(tones_data.data[current].synth.phase_dets[i]) / (float)ks_1(KS_PHASE_MAX_BITS));
                    tones_data.data[current].synth.phase_dets[i] = PropertyInt(pos, text, tones_data.data[current].synth.phase_dets[i], 0, 255, 1);
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
                        text = FormatText("%.3f", calc_envelope_points(tones_data.data[current].synth.envelope_points[e][i]) / (float)ks_1(KS_ENVELOPE_BITS));
                        tones_data.data[current].synth.envelope_points[e][i] = PropertyInt(pos2, text, tones_data.data[current].synth.envelope_points[e][i], 0, 255, 1);
                        pos2.x += step_x / 2.0f;

                        float sec = ks_calc_envelope_times(tones_data.data[current].synth.envelope_times[e][i]) / (float)ks_1(16);
                        if(sec >= 1.0f){
                            text = FormatText("%.1f s", sec);
                        }
                        else if(sec< 1.0f && sec>= 0.01f){
                            text = FormatText("%.1f ms", sec*1000.0f);
                        }
                        else {
                            text = FormatText("%.3f ms", sec*1000.0f);
                        }

                        tones_data.data[current].synth.envelope_times[e][i] = PropertyInt(pos2, text, tones_data.data[current].synth.envelope_times[e][i], 0, 255, 1);
                        pos2.x += step_x / 2.0f;
                    }
                    pos.x = x_pos.x; pos.y += step;
                }

                // envelope release time
                GuiAlignedLabel("Release Time", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    float sec = ks_calc_envelope_times(tones_data.data[current].synth.envelope_release_times[i]) / (float)ks_1(16);
                    if(sec >= 1.0f){
                        text = FormatText("%.1f s", sec);
                    }
                    else if(sec< 1.0f && sec>= 0.01f){
                        text = FormatText("%.1f ms", sec*1000.0f);
                    }
                    else {
                        text = FormatText("%.3f ms", sec*1000.0f);
                    }

                    tones_data.data[current].synth.envelope_release_times[i] = PropertyInt(pos, text, tones_data.data[current].synth.envelope_release_times[i], 0, 255, 1);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;

                // rate scale
               GuiAlignedLabel("Rate Scale", pos, GUI_TEXT_ALIGN_RIGHT);
               pos.x += step_x;
               for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                   text = FormatText("%.1f %%", 100.0*calc_ratescales(tones_data.data[current].synth.ratescales[i]) / (float)ks_1(KS_RATESCALE_BITS));
                   tones_data.data[current].synth.ratescales[i] = PropertyInt(pos, text, tones_data.data[current].synth.ratescales[i], 0, 255, 1);
                   pos.x += step_x;
               }
               pos.x = x_pos.x; pos.y += step;

                // velocity sensitive
                GuiAlignedLabel("Velocity Sensitive", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.1f %%", 100*calc_velocity_sens(tones_data.data[current].synth.velocity_sens[i]) / (float)ks_1(KS_VELOCITY_SENS_BITS));
                    tones_data.data[current].synth.velocity_sens[i] = PropertyInt(pos, text, tones_data.data[current].synth.velocity_sens[i], 0, 255, 1);
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
                    tones_data.data[current].synth.keyscale_curve_types.st[i].left = PropertyIntImage(pos2, keyscale_left_images, tones_data.data[current].synth.keyscale_curve_types.st[i].left, 0, KS_KEYSCALE_CURVE_NUM_TYPES-1, 1);
                    pos2.x += pos2.width  +margin;
                    tones_data.data[current].synth.keyscale_curve_types.st[i].right = PropertyIntImage(pos2, keyscale_right_images, tones_data.data[current].synth.keyscale_curve_types.st[i].right, 0, KS_KEYSCALE_CURVE_NUM_TYPES-1, 1);
                    pos2.x +=  pos2.width +margin;
                }
                pos.x = x_pos.x; pos.y += (pos2.height + margin);

                // keyscale low depth
                GuiAlignedLabel("Keyscale Depth", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;

                pos2 = pos;
                pos2.width /=2.0f;

                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.1f %%", 100*calc_keyscale_low_depths(tones_data.data[current].synth.keyscale_low_depths[i]) / (float)ks_1(KS_KEYSCALE_DEPTH_BITS));
                    tones_data.data[current].synth.keyscale_low_depths[i] = PropertyInt(pos2, text, tones_data.data[current].synth.keyscale_low_depths[i], 0, 255, 1);

                    pos2.x += step_x/2.0f;

                    text = FormatText("%.1f %%", 100*calc_keyscale_high_depths(tones_data.data[current].synth.keyscale_high_depths[i]) / (float)ks_1(KS_KEYSCALE_DEPTH_BITS));
                    tones_data.data[current].synth.keyscale_high_depths[i] = PropertyInt(pos2, text, tones_data.data[current].synth.keyscale_high_depths[i], 0, 255, 1);
                    pos2.x += step_x / 2.0f;
                }
                pos.x = x_pos.x; pos.y += step;


                // keyscale mid point
                GuiAlignedLabel("Keyscale Mid Key", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%d", calc_keyscale_mid_points(tones_data.data[current].synth.keyscale_mid_points[i]));
                    tones_data.data[current].synth.keyscale_mid_points[i] = PropertyInt(pos, text, tones_data.data[current].synth.keyscale_mid_points[i], 0, 127, 1);
                    pos.x += step_x;
                }
                pos.x = x_pos.x; pos.y += step;

                // lfo ams depth
                GuiAlignedLabel("LFO AMS", pos, GUI_TEXT_ALIGN_RIGHT);
                pos.x += step_x;
                for(unsigned i=0; i< KS_NUM_OPERATORS; i++){
                    text = FormatText("%.1f %%", 100*calc_lfo_ams_depths(tones_data.data[current].synth.lfo_ams_depths[i]) / (float)ks_1(KS_LFO_DEPTH_BITS));
                    tones_data.data[current].synth.lfo_ams_depths[i] = PropertyInt(pos, text, tones_data.data[current].synth.lfo_ams_depths[i], 0, 255, 1);
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

                i8 noteon = -1;
                i8 velocity = 0;

                float x = pos.x;
                float y = pos.y;
                for(int o=0; o< oct; o++){
                    const i8 whites[] = {0, 2, 4, 5, 7, 9, 11};
                    const i8 blacks[] = {1, 3, 6, 8, 10};
                    float x2 = x;
                    const i8 offset = 60-(oct / 2)* 12 + o*12;
                    Rectangle recs[12];

                    for(int k=0; k<7; k++){


                        Rectangle rec = recs[whites[k]] = (Rectangle){x2, y, white.x, white.y};
                        i8 n = whites[k]+ offset;

                        if(n == noteon_number){
                            noteon_rec = rec;
                            DrawRectangleRec(rec, SKYBLUE);
                            DrawRectangleLinesEx(rec, 1, state != EDIT ? LIGHTGRAY : GRAY);
                        }
                        else {
                            DrawRectangleRec(rec, RAYWHITE);
                            DrawRectangleLinesEx(rec, 1, state != EDIT ? LIGHTGRAY : GRAY);
                        }

                        x2 += white.x;
                    }

                    x2 = x + white.x * 0.6;
                    for(int k=0; k<5; k++){

                        Rectangle rec = recs[blacks[k]] = (Rectangle){x2, y, black.x, black.y};
                        i8 n = blacks[k]+ offset;


                        if(n == noteon_number){
                            noteon_rec = rec;
                            DrawRectangleRec(rec, DARKBLUE);
                        } else {
                            DrawRectangleRec(rec, state != EDIT ? LIGHTGRAY : DARKGRAY);
                        }


                        x2 += (k != 1 && k != 4) ? white.x : white.x*2;
                    }


                    if(state ==EDIT){
                        for(int i=0; i<7; i++){
                            i8 n = offset + whites[i];
                            Vector2 mouse = GetMousePosition();
                            if(CheckCollisionPointRec(mouse, recs[whites[i]]) ){
                                if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && noteon_number != n){
                                        noteon = n;
                                        velocity = (mouse.y - recs[whites[i]].y)*127 / recs[whites[i]].height;
                                }
                            }
                        }
                        for(int i=0; i<5; i++){
                            i8 n = offset + blacks[i];
                            Vector2 mouse = GetMousePosition();
                            if(CheckCollisionPointRec(mouse, recs[blacks[i]]) ){
                                if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && noteon_number != n){
                                        noteon = n;
                                        velocity = (mouse.y - recs[blacks[i]].y)*127 / recs[blacks[i]].height;
                                }
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
                    ks_synth_set(&synth, SAMPLING_RATE, &tones_data.data[current].synth);
                    ks_synth_note_on(&note, &synth, SAMPLING_RATE, noteon, velocity);
                    noteon_number = noteon;
                }
            }

            GuiEnable();

            switch (state) {

            case CONFIRM_NEW:{
                bool run_new = !dirty;

                if(dirty){
                    Rectangle rec={0,0, 200, 100};
                    rec.x = (SCREEN_WIDTH - rec.width)/ 2;
                    rec.y = (SCREEN_HEIGHT - rec.height) / 2;
                    int res = GuiMessageBox(rec, "Confirm", "Clear current and Create new ?" , "Yes;No");

                    if(res == 0 || res ==2){
                        state = EDIT;
                    }
                    else if(res == 1) {
                        run_new = true;
                    }
                }

                if(run_new){
                    strcpy(file_dialog_state.fileNameText, "");
                    unmark_dirty(&dirty, &file_dialog_state);
                    state = EDIT;
                    ks_vector_clear(&tones_data);
                    ks_tone_list_insert_empty(&tones_data, &current);
                    current = 0;
                    temp = tones_data.data[current].synth;
                }
                break;
            }
            case LOAD_DIALOG:{
                GuiFileDialog(&file_dialog_state, false);
                if(file_dialog_state.fileDialogActiveState == DIALOG_DEACTIVE) {
                    if(file_dialog_state.SelectFilePressed){
                        if(strcmp(file_dialog_state.fileNameText, "") == 0){
                            file_dialog_state.SelectFilePressed = false;
                            file_dialog_state.fileDialogActiveState = DIALOG_ACTIVE;
                            break;
                        }

                        ks_tone_list_data load;
                        if(!SaveLoadToneList(&load,&file_dialog_state, false)){
                            ks_error("Failed to load synth");

                        }else {
                            free(tones_data.data);
                            tones_data = load;
                            current = 0;
                            temp = tones_data.data[current].synth;
                            unmark_dirty(&dirty, &file_dialog_state);
                            update_tone_list(&tones, &tones_data, score_state);
                        }
                    }
                    state = EDIT;
                }
                break;
            }
            case SAVE_DIALOG:{
                GuiFileDialog(&file_dialog_state, true);
                if(file_dialog_state.fileDialogActiveState == DIALOG_DEACTIVE){
                    if(file_dialog_state.SelectFilePressed){
                        if(SaveLoadToneList(&tones_data ,&file_dialog_state, true)){
                            unmark_dirty(&dirty, &file_dialog_state);
                            update_tone_list(&tones, &tones_data, score_state);
                        }
                    }
                    state = EDIT;
                }
                break;
            }
            case IMPORT_SYNTH_DIALOG:{
                GuiFileDialog(&file_dialog_state_synth, false);
                if(file_dialog_state_synth.fileDialogActiveState == DIALOG_DEACTIVE){
                    if(file_dialog_state_synth.SelectFilePressed){
                        ks_synth_data tmp;
                        if(SaveLoadSynth(&tmp ,&file_dialog_state_synth, false)){
                            mark_dirty(&dirty, &file_dialog_state);
                            tones_data.data[current].synth = temp = tmp;
                        }
                    }
                    state = EDIT;
                }
                break;
            }
            case EXPORT_SYNTH_DIALOG:{
                GuiFileDialog(&file_dialog_state_synth, true);
                if(file_dialog_state_synth.fileDialogActiveState == DIALOG_DEACTIVE){
                    if(file_dialog_state_synth.SelectFilePressed){
                        SaveLoadSynth(&tones_data.data[current].synth ,&file_dialog_state_synth, true);
                    }
                    state = EDIT;
                }
                break;
            }
            case SETTINGS:{
                const float lr_margin =  SCREEN_WIDTH * 0.3f;
                const float td_margin = SCREEN_HEIGHT * 0.28f;
                Rectangle window ={ lr_margin, td_margin, SCREEN_WIDTH - lr_margin *2, SCREEN_HEIGHT - td_margin*2 };
                if(GuiWindowBox(window, "#141#Settings")){
                    state = EDIT;
                }
                Rectangle cp = {window.x + margin*5, window.y + margin*2 + line_width*2, base_width, line_width};
                if(midiin == NULL){
                    GuiDisable();
                }
\
                {
                    GuiLabel(cp, "Midi Input Port");
                    cp.y += step;

                    const Rectangle listrec = {cp.x, cp.y, window.width - margin*10, step *10};
                    const u32 midiin_count = midiin == NULL ? 0 : rtmidi_get_port_count(midiin);
                    const char *midiin_list[midiin_count];
                    for(u32 i=0; i< midiin_count; i++){
                        midiin_list[i] = rtmidi_get_port_name(midiin, i);
                    }
                    const u32 new_midiin_port = GuiListViewEx(listrec, midiin_list, midiin_count, NULL, &midiin_list_scroll, midiin_port);
                    if(new_midiin_port != midiin_port && new_midiin_port < midiin_count){
                        midiin_port = new_midiin_port;
                        rtmidi_close_port(midiin);
                        rtmidi_open_port(midiin, midiin_port, midiin_list[midiin_port]);
                    }
                }
                if(midiin == NULL){
                    GuiEnable();
                }
                cp.y += step*10 + margin;

                {
                    const Rectangle button_rec = {window.x + window.width - margin*5 - cp.width, cp.y, cp.width, cp.height*2};
                    if(GuiButton(button_rec, "Done")){
                        state = EDIT;
                    }
                }
            }
            default:
                break;
        }
		EndDrawing();

    }



    //----------------------------------------------------------------------------------
    // unload
    UnloadTexture(algorithm_images);
    UnloadTexture(lfo_wave_images);
    UnloadTexture(keyscale_left_images);
    UnloadTexture(keyscale_right_images);

    CloseWindow();
    CloseAudioDevice();

    ks_score_state_free(score_state);
    if(midiin != NULL){
        rtmidi_close_port(midiin);
        rtmidi_in_free(midiin);
    }

    free(buf);
    free(tones_data.data);

    return 0;
}


