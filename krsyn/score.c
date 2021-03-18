#include "score.h"
#include "tone_list.h"

#include <ksio/logger.h>
#include <ksio/vector.h>
#include <ksio/formats/midi.h>

#include <stdlib.h>
#include <malloc.h>


ks_io_begin_custom_func(ks_score_event)
    ks_func_prop(ks_io_variable_length_number, ks_prop_u32(delta));
    ks_u8(status);
    if(ks_access(status) == 0xff ) {
        ks_u8(data[0]);
        if(ks_access(data[0]) == 0x51){
            ks_u8(data[2]);
            ks_u8(data[3]);
        }
    }
     else {
        switch (ks_access(status) >> 4) {
        case 0xc:
        case 0xd:
            ks_u8(data[0]);
            break;
        default:
            ks_u8(data[0]);
            ks_u8(data[1]);
            break;
        }
    }
ks_io_end_custom_func(ks_score_event)

ks_io_begin_custom_func(ks_score_data)
    ks_magic_number("KSCR");
    ks_str(title);
    ks_str(author);
    ks_str(license);
    ks_u16(resolution);
    ks_u32(length);
    ks_arr_obj_len(data, ks_score_event, ks_access(length));
ks_io_end_custom_func(ks_score_data)

KS_INLINE  bool  ks_synth_note_is_enabled     (const ks_synth_note* note){
    return *((u32*)note->envelope_states) != 0;
}

KS_INLINE bool ks_synth_note_is_on (const ks_synth_note* note){
    return (*((u32*)note->envelope_states) & 0x0f0f0f0f) != 0;
}

KS_INLINE bool ks_score_note_is_enabled(const ks_score_note* note){
    return ks_synth_note_is_enabled(&note->note);
}

KS_INLINE  bool ks_score_note_is_on(const ks_score_note* note){
    return ks_synth_note_is_on(&note->note);
}

inline bool ks_score_note_info_equals(ks_score_note_info i1, ks_score_note_info i2){
    return (((i1.channel)<<8) + i1.note_number) == (((i2.channel)<<8) + i2.note_number);
}

inline i16 ks_score_note_info_hash(ks_score_note_info id){
    return id.note_number +  id.channel;
}

inline ks_score_note_info ks_score_note_info_of(u8 note_number, u8 channel){
    return (ks_score_note_info){
                .note_number = note_number,
                .channel = channel,
            };
}


ks_score_data* ks_score_data_new(u32 resolution, u32 num_events, ks_score_event* events){
    ks_score_data* ret = calloc(1, sizeof(ks_score_data));
    ret->data = events;
    ret->length = num_events;
    ret->resolution = resolution;

    return ret;
}

void ks_score_data_free(ks_score_data* song){
    ks_score_events_free(song->data);
    free(song);
}

KS_INLINE u32 ks_calc_quarter_time(const u8* data){
    return data[1] + ks_v(data[2], KS_QUARTER_TIME_BITS);
}

KS_INLINE u32 ks_calc_frames_per_event(const ks_synth_context *ctx, u16 quarter_time, u16 resolution){
    u64 ret = ctx->sampling_rate;
    ret *= quarter_time;
    ret /= resolution;
    ret >>= KS_QUARTER_TIME_BITS;
    return (u32)ret;
}


float ks_score_data_calc_score_length(ks_score_data* score, const ks_synth_context *ctx){
    u16 quarter_time = KS_DEFAULT_QUARTER_TIME;
    u32 frames_per_event = ks_calc_frames_per_event(ctx, quarter_time, score->resolution);

    u32 tick = 0;
    double time = 0;

    for(unsigned i=0; i< score->length; i++){
        tick = tick+ score->data[i].delta;
        const u64 delta_frame = (u64)score->data[i].delta* frames_per_event;
        const double delta_time =  (double)delta_frame / ctx->sampling_rate;
        time = time + delta_time;
        // tempo change
        if(score->data[i].status ==  0xff && score->data[i].data[0] == 0x51){
            quarter_time = ks_calc_quarter_time(score->data[i].data);
            frames_per_event= ks_calc_frames_per_event(ctx, quarter_time, score->resolution);
        }
    }

    return score->score_length = time;
}


ks_score_state* ks_score_state_new(u32 polyphony_bits){
    ks_score_state* ret = calloc(1, sizeof(ks_score_state) + ks_1(polyphony_bits)* sizeof(ks_score_note));
    ks_vector_init(&ret->effects);
    ret->polyphony_bits = polyphony_bits;

    return ret;
}

void ks_score_state_free(ks_score_state* state){
    for(unsigned i = 0; i< KS_NUM_CHANNELS; i++){
        if(state->channels[i].output_log != NULL){
            free(state->channels[i].output_log);
        }
    }
    ks_effect_list_data_free(state->effects.length, state->effects.data);
    free(state);
}

bool ks_score_state_note_on(ks_score_state* state, const ks_synth_context* ctx, u8 channel_number,  u8 note_number, u8 velocity){
    ks_score_channel* channel = state->channels + channel_number;
     ks_synth* synth = channel->program;
     if(synth == NULL) {
         ks_error("Note on failed for not set program of channel %d at tick %d", channel_number, state->current_tick);
         return false;
     }

    ks_score_note_info id =ks_score_note_info_of(note_number, channel_number);
    u16 id_v = ks_score_note_info_hash(id);
    u16 begin = ks_mask(id_v, state->polyphony_bits);
    u16 index = begin;
    while(ks_score_note_is_enabled(&state->notes[index])){
        // if found same channel and notenumber note, note off
        if(ks_score_note_info_equals(id, state->notes[index].info)){
            ks_synth_note_off(&state->notes[index].note);
        }

        index++;
        index = ks_mask(index, state->polyphony_bits);

        if(index == begin) {
            ks_warning("Note on failed for exceeded maximum of polyphony at tick %d", channel_number, state->current_tick);
            return false;
        }
    }
    if(channel->bank->bank_number.percussion) {
        synth += note_number;

    }
    state->notes[index].info = id;
    ks_synth_note_on(&state->notes[index].note, synth, ctx, note_number, velocity);

    return true;
}

bool ks_score_state_note_off(ks_score_state* state, u8 channel_number,  u8 note_number){
    ks_score_channel* channel = state->channels + channel_number;
    ks_tone_list_bank* bank = channel->bank;
    if(bank == NULL) {
        ks_error("Note off failed for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }

    if(ks_tone_list_bank_is_empty(bank)) {
        ks_error("Note off Failed for not set bank of channel %d at tick %d", channel_number, state->current_tick);
        return false;
    }
    ks_score_note_info info =ks_score_note_info_of(note_number, channel_number);
    u16 hash = ks_score_note_info_hash(info);
    u16 begin = ks_mask(hash, state->polyphony_bits);
    u16 index = begin;
    while(!ks_score_note_info_equals(state->notes[index].info, info) || !ks_score_note_is_on(&state->notes[index])) {
        index++;
        index = ks_mask(index, state->polyphony_bits);
        if(index == begin) {
            ks_warning("Note off Failed for not found note with note number %d and channel %d at tick %d", note_number, channel_number, state->current_tick);
            return false;
        }
    }

    ks_synth_note_off(&state->notes[index].note);

    return true;
}

bool ks_score_state_program_change(ks_score_state* state, const ks_tone_list* tones, int ch_number,  u8 program){
    ks_score_channel* channel = state->channels + ch_number;

    if(channel->bank == NULL) {
        ks_error("Program change of channel %d failed for not set bank at tick %d", (channel - state->channels) / sizeof(ks_score_channel),state->current_tick);
        return false;
    }

    ks_synth* p = channel->bank->programs[program];
    if(p == NULL){
        ks_tone_list_bank* bank = ks_tone_list_find_bank(tones, ks_tone_list_bank_number_of(0,0, ch_number == 9));
        if(bank == NULL){
            bank = tones->data;
        }
        p = channel->bank->programs[program];

        ks_tone_list_bank* begin = tones->data;
        unsigned it=0;
        while(begin[it].programs[program] == NULL){
            it++;
            it %= tones->length;
            if(it == 0){
                ks_error("Program change of channel %d failed for not found program %d at tick %d",(channel - state->channels) / sizeof(ks_score_channel), program,state->current_tick);
                return  false;
            }
        }
    }
    channel->program_number = program;
    channel->program = p;
    return true;
}

bool ks_score_state_bank_select(ks_score_state* state, const ks_tone_list* tones, int ch_number,  u8 msb, u8 lsb){
    ks_score_channel* channel = state->channels + ch_number;

    ks_tone_list_bank *bank = ks_tone_list_find_bank(tones, ks_tone_list_bank_number_of(msb, lsb, ch_number == 9));
    if(bank == NULL) {
        ks_error("Bank select of channel %d failed for not found bank at tick %d", (channel - state->channels) / sizeof(ks_score_channel),state->current_tick);
        return false;
    }

    channel->bank = bank;

    return ks_score_state_program_change(state, tones, ch_number, channel->program_number);
}

bool ks_score_state_bank_select_msb(ks_score_state* state, const ks_tone_list* tones, int ch_number,  u8 msb){
    ks_score_channel* channel = state->channels + ch_number;
    u8 lsb = 0;
    if(channel->bank != NULL){
        lsb = channel->bank->bank_number.lsb;
    }
    return ks_score_state_bank_select(state, tones, ch_number, msb, lsb);
}

bool ks_score_state_bank_select_lsb(ks_score_state* state, const ks_tone_list* tones, int ch_number,  u8 lsb){
    ks_score_channel* channel = state->channels + ch_number;
    u8 msb = 0;
    if(channel->bank != NULL){
        msb = channel->bank->bank_number.msb;
    }
    return ks_score_state_bank_select(state, tones, ch_number,  msb, lsb);
}

bool ks_score_channel_set_panpot(ks_score_channel* channel, const ks_synth_context* ctx, u8 value){
    ks_calc_panpot(ctx, &channel->panpot_left, &channel->panpot_right, value);
    return true;
}

bool ks_score_channel_set_picthbend(ks_score_channel* channel, u8 msb, u8 lsb){
    channel->pitchbend = ks_v(msb, 7) + lsb - ks_v(64, 7);
    channel->pitchbend = ks_fms_depth(channel->pitchbend << (KS_LFO_DEPTH_BITS - KS_PITCH_BEND_EVENT_BITS));
    return true;
}

bool ks_score_state_tempo_change(ks_score_state* state, const ks_synth_context* ctx, const ks_score_data* score, const u8* data){
    state->quarter_time = ks_calc_quarter_time(data);
    state->frames_per_event= ks_calc_frames_per_event(ctx, state->quarter_time, score->resolution);
    for(unsigned i = 0; i< KS_NUM_CHANNELS; i++){
        state->channels[i].output_log = realloc(state->channels[i].output_log, state->frames_per_event * 2 * sizeof(i32));
    }
    return true;
}

static inline void set_channel_volume_cache(ks_score_channel* ch){
    ch->volume_cache = (u16)ch->volume * ch->expression;
}


inline bool ks_score_channel_set_volume(ks_score_channel* ch, u8 value){
    ch->volume = value;
    set_channel_volume_cache(ch);
    return true;
}

inline bool ks_score_channel_set_expression(ks_score_channel* ch, u8 value){
    ch->expression = value;
    set_channel_volume_cache(ch);
    return true;
}

bool ks_score_state_control_change(ks_score_state* state, const ks_tone_list* tones, const ks_synth_context* ctx, int ch_number,  u8 type, u8 value){
    ks_score_channel* channel = state->channels + ch_number;

    switch (type) {
    case 0x00:
        return ks_score_state_bank_select_msb(state, tones, ch_number, value);
    case 0x07:
        return ks_score_channel_set_volume(channel, value);
    case 0x0b:
        return ks_score_channel_set_expression(channel, value);
    case 0x20:
        return ks_score_state_bank_select_lsb(state, tones, ch_number, value);
    case 0x0a:
        return ks_score_channel_set_panpot(channel, ctx, value);
    }
    return true;
}

void ks_score_data_render(const ks_score_data *score, const ks_synth_context* ctx, ks_score_state* state, const ks_tone_list*tones, i32* buf, u32 len){
    unsigned i=0;
    memset(buf, 0, sizeof(i32)*len);
    do{
        u32 frame = MIN(len-i, state->remaining_frame*2);
        i32* tmpbuf = malloc(sizeof(i32)*frame);

        bool channel_enabled[KS_NUM_CHANNELS];
        memset(channel_enabled, false, sizeof(channel_enabled)); // all false

        //render each notes to channels
        for(u32 p=0; p<ks_1(state->polyphony_bits); p++){
            if(!ks_score_note_is_enabled(&state->notes[p])) {
                continue;
            }

            ks_score_channel* channel = &state->channels[state->notes[p].info.channel];

            if(channel_enabled[state->notes[p].info.channel] == false){
                memset(channel->output_log, 0, frame* sizeof(i32));
                channel_enabled[state->notes[p].info.channel]= true;
            }

            // when note on, already checked,
            //if(channel->bank == NULL) continue;
            //if(channel->bank->programs[channel->program_number] == NULL) continue;
            ks_synth_note* note = &state->notes[p].note;
            ks_synth_render(ctx, note, channel->volume_cache, channel->pitchbend, tmpbuf, frame);

            for(u32 b =0; b< frame; b+=2){
                channel->output_log[b] += tmpbuf[b];
                channel->output_log[b + 1] += tmpbuf[b+1];
            }
        }

        // mix to buffer
        for(u32 c=0; c<KS_NUM_CHANNELS; c++){
            if(!channel_enabled[c]) continue;
            ks_score_channel* channel = &state->channels[c];

            for(u32 b =0; b< frame; b+=2){
                buf[(i + b)] += channel->output_log[b] = ks_apply_panpot(channel->output_log[b], channel->panpot_left);
                buf[(i + b) + 1] +=channel->output_log[b+1] =  ks_apply_panpot(channel->output_log[b+1], channel->panpot_right);
            }
        }

        // apply post effect
        for(u32 e=0; e<state->effects.length; e++){
            switch (state->effects.data[e].type) {
            case KS_EFFECT_VOLUME_ANALIZER:
                ks_effect_volume_analize(&state->effects.data[e], state, buf + i, frame, channel_enabled);
                break;
            }
        }

        free(tmpbuf);

        state->remaining_frame -= frame >> 1;

        if(state->remaining_frame == 0){
            if((u32)state->passed_tick >= score->data[state->current_event].delta){
                state->passed_tick -= score->data[state->current_event].delta;
                if(ks_score_data_event_run(score, ctx, state, tones)){
                    state->passed_tick = INT32_MIN;
                }
            }
            state->passed_tick ++;
            state->current_tick ++;

            state->remaining_frame = state->frames_per_event;
        }

        i+= frame;
    }while(i<len);
}

ks_score_event* ks_score_events_new(u32 num_events, ks_score_event events[]){
    ks_score_event* ret = malloc(sizeof(ks_score_event) * num_events);
    for(unsigned i=0; i<num_events; i++){
        ret[i] = events[i];
    }
    return ret;
}

void ks_score_events_free(const ks_score_event* events){
    free((ks_score_event*)events);
}



bool ks_score_data_event_run(const ks_score_data* score, const ks_synth_context* ctx, ks_score_state *state,  const ks_tone_list *tones){

    do{
        const ks_score_event* msg = &score->data[state->current_event];
        switch (msg->status) {
        case 0xff:
            // tempo
            if(msg->data[0] == 0x51){
                ks_score_state_tempo_change(state, ctx, score, msg->data);
            }
            // end of track
            else if(msg->data[0] == 0x2f){
                return true;
            }


        default:{
            // channel message
            u8 channel_num = msg->status & 0x0f;
            ks_score_channel* channel = &state->channels[channel_num];
            switch (msg->status >> 4) {
            // note off
            case 0x8:
                ks_score_state_note_off(state, channel_num, msg->data[0]);
                break;
            // note on
            case 0x9:
                if(msg->data[1] == 0){
                    ks_score_state_note_off(state, channel_num,  msg->data[0]);
                }
                else {
                    ks_score_state_note_on(state, ctx, channel_num, msg->data[0], msg->data[1]);
                }
                break;
            // control change
            case 0xb:
                ks_score_state_control_change(state, tones, ctx, channel_num, msg->data[0], msg->data[1]);
                break;
            // program change
            case 0xc:
                ks_score_state_program_change(state, tones, channel_num, msg->data[0]);
                break;
            // pich wheel change
            case 0xe:
                ks_score_channel_set_picthbend(channel, msg->data[1], msg->data[2]);
                break;
            }
        }
        }
        state->current_event++;
    }while(score->data[state->current_event].delta == 0); // state->current_event < score->length

    return false;
}


void ks_score_state_set_default(ks_score_state* state, const ks_tone_list *tones, const ks_synth_context*ctx, u32 resolution){
    state->quarter_time = KS_DEFAULT_QUARTER_TIME; // 0.5
    state->frames_per_event = ks_calc_frames_per_event(ctx, state->quarter_time, resolution);
    state->remaining_frame = 0;
    state->current_event = 0;
    state->passed_tick = 0;
    state->current_tick = 0;
    for(unsigned i=0; i<ks_1(state->polyphony_bits); i++) {
        memset(&state->notes[i], 0 , sizeof(ks_score_note));
    }
    for(unsigned i =0; i<KS_NUM_CHANNELS; i++){
        if( state->channels[i].output_log != NULL) {
            free( state->channels[i].output_log );
        }

        memset(&state->channels[i], 0 , sizeof(ks_score_channel));
        ks_score_channel_set_panpot(&state->channels[i], ctx, 64);
        ks_score_channel_set_picthbend(&state->channels[i], 64, 0);
        ks_score_state_bank_select(state, tones, i, 0, 0);
        state->channels[i].volume= 100;
        state->channels[i].expression= 127;
        set_channel_volume_cache(&state->channels[i]);

        state->channels[i].output_log = malloc(state->frames_per_event * 2 * sizeof(i32));
    }
}

ks_score_data* ks_score_data_from_midi(ks_midi_file* file){
    const ks_midi_file* conbined;
    if(file->format == 0){
        conbined = file;
        ks_midi_file_calc_time(file);
    } else {
        conbined = file->format == 0 ? file : ks_midi_file_conbine_tracks(file);
    }

    ks_score_data* ret = ks_score_data_new(conbined->resolution, 0, NULL);
    ks_score_event* events = calloc(conbined->tracks[0].num_events, sizeof(ks_score_event));


    u64 time=0;
    for(unsigned i=0; i< conbined->tracks[0].num_events; i++){
        ks_midi_event* msg = &conbined->tracks[0].events[i];
        switch (msg->status) {
        case 0xff:
            // tempo
            if(msg->message.meta.type == 0x51){
                events[ret->length].status = 0xff;
                events[ret->length].data[0] = 0x51;
                //msg->message.meta.length == 0x03;
                const u32 quarter_micro = ks_v((u32)msg->message.meta.data[0], 16) +
                        ks_v((u32)msg->message.meta.data[1], 8) +
                       (u32)msg->message.meta.data[2];
                const u32 quarter_mili = quarter_micro / 1000;
                const u32 quarter_mili_fp8 = quarter_mili * ks_1(KS_QUARTER_TIME_BITS);
                const u32 quarter_fp8 = quarter_mili_fp8 / 1000;
                // little endian
                events[ret->length].data[1] = ks_mask(quarter_fp8, KS_QUARTER_TIME_BITS);
                events[ret->length].data[2] = quarter_fp8 >> KS_QUARTER_TIME_BITS;

                events[ret->length].delta = msg->time - time;
                ret->length++;
                time = msg->time;
            }
            // end of track
            else if(msg->message.meta.type == 0x2f){
                events[ret->length].status = 0xff;
                events[ret->length].data[0] = 0x2f;
                events[ret->length].data[1] = 0x00;

                events[ret->length].delta = msg->time - time;
                ret->length++;
                time = msg->time;
            }
            break;
        default:
            if(msg->status >= 0x80 &&
                    msg->status < 0xf0){
                events[ret->length].status = msg->status;
                events[ret->length].data[0] = msg->message.data[0];
                events[ret->length].data[1] = msg->message.data[1];

                events[ret->length].delta = msg->time - time;
                ret->length++;
                time = msg->time;
            }
            break;
        }
    }
    if(file->format != 0) ks_midi_file_free((ks_midi_file*)conbined);

    ret->data = events;

    return ret;
}

void ks_score_state_add_volume_analizer (ks_score_state* state, const ks_synth_context *ctx, u32 duration){
    u64 frame = (u64)duration * ctx->sampling_rate;
    frame >>= KS_TIME_BITS;
    frame *= 2; // channel

    ks_effect e = {
        .type = KS_EFFECT_VOLUME_ANALIZER,
        .data ={
            .volume_analizer={
                .length = frame,
                .volume = { 0 },
                .log = { 0 },
                .seek = 0,
            }
        }
    };

    ks_vector_push(&state->effects, e);
    ks_effect* effect = &state->effects.data[state->effects.length-1];

    for(u32 c=0; c<KS_NUM_CHANNELS+1; c++){
        effect->data.volume_analizer.log[c] = calloc(frame, sizeof(i32));
    }
}

void ks_effect_volume_analize  (ks_effect* effect, ks_score_state* state, i32* buf, u32 len, bool channels_enabled[]){
    ks_volume_analizer* dat = &effect->data.volume_analizer;

    unsigned i= 0;
    do{
        u32 f;
        if(dat->seek + len - i > dat->length){
            f = dat->length - dat->seek;
        }
        else {
            f = len - i;
        }
        u32 e = i+f;
        for(u32 c=0; c<KS_NUM_CHANNELS; c++){
            ks_score_channel* channel = &state->channels[c];
            if(channels_enabled[c]){
                for(u32 s = dat->seek, j=i; j<e; s++, j++){
                    u32 o = channel->output_log[j] > 0 ? channel->output_log[j] : - channel->output_log[j];
                    dat->log[c][s] = o;
                }
            }
            else{
                memset(&dat->log[c][dat->seek], 0, sizeof(i32)*f);
            }
        }
        for(u32 s = dat->seek, j=i; j<e; s++, j++){
            u32 o = buf[j] > 0 ? buf[j] : - buf[j];
            dat->log[KS_NUM_CHANNELS][s] = o;
        }
        dat->seek = dat->seek + f;
        dat->seek %= dat->length;
        i+= f;
    }while(i<len);
}

void ks_effect_volume_analizer_clear(ks_effect* effect){
    ks_volume_analizer* a = &effect->data.volume_analizer;
    a->seek = 0;
    for(u32 c =0; c<KS_NUM_CHANNELS+1; c++){
        for(unsigned i=0; i< a->length; i++){
            a->log[c][i] = 0;
        }
        a->volume[c] = 0;
    }
}

void ks_effect_list_data_free(u32 length, ks_effect* data){
    for(unsigned i=0; i<length; i++){
        switch (data[i].type) {
        case KS_EFFECT_VOLUME_ANALIZER:
            for(u32 c=0;c<KS_NUM_CHANNELS+1;c++){
                free(data[i].data.volume_analizer.log[c]);
            }
            break;
        }
    }
    free(data);
}

const u32*  ks_effect_calc_volume(ks_effect* effect){
    ks_volume_analizer* a = &effect->data.volume_analizer;
    for(u32 c =0; c<KS_NUM_CHANNELS; c++){
        u32 out=0;
        for(unsigned i=0; i< a->length; i++){
            out += a->log[c][i];
        }
        out /= a->length;
        a->volume[c] = out;
    }
    u32 out_l=0, out_r=0;
    for(unsigned i=0; i< a->length; i+=2){
        out_l += a->log[KS_NUM_CHANNELS][i];
        out_r += a->log[KS_NUM_CHANNELS][i+1];
    }
    out_l /= a->length/2;
    out_r /= a->length/2;
    a->volume_l = out_l;
    a->volume_r = out_r;

    return a->volume;
}
