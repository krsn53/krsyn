#include "audio_manager.h"



static gboolean wave_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    audio_state* state = (audio_state* )user_data;
    gint width, height;
    gint show_samples = 256;
    GtkStyleContext *context;
    double half_height;

    double env_width;
    double env_draw_width;
    double env_reg;


    context = gtk_widget_get_style_context (widget);

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    gtk_render_background (context, cr, 0, 0, width, height);
    cairo_set_source_rgb(cr,0,0,0);
    cairo_rectangle(cr, 0, 0,width, height);
    cairo_fill (cr);

    env_width = (double)width / KS_NUM_OPERATORS;
    env_draw_width = env_width*0.8;
    cairo_set_source_rgb(cr, 0, 0, 1);
    env_reg = env_width*0.1;
    for(unsigned i = 0; i<KS_NUM_OPERATORS; i++, env_reg+=env_width)
    {
        double envelope_amp = state->note.envelope_now_amps[i];
        envelope_amp /= (double)(1<<KS_ENVELOPE_BITS);
        envelope_amp *= height;

        cairo_rectangle(cr, env_reg, height - envelope_amp, env_draw_width, envelope_amp);

        cairo_fill(cr);
    }

    half_height= (double)height / width * show_samples * 0.5;
    cairo_scale(cr, (double)width/ show_samples, (double)width/show_samples);
    cairo_set_line_width(cr,  show_samples/(double)width);

    cairo_set_source_rgb(cr, 0, 1, 0);
    for(int i=0; i<show_samples-1; i++)
    {
        double x1 = i ;
        double y1 =  half_height * state->buf[NUM_CHANNELS*i] / INT16_MAX +  half_height;
        double x2 = i + 1;
        double y2 =  half_height * state->buf[NUM_CHANNELS*i + NUM_CHANNELS] / INT16_MAX + half_height;

        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    }

    return FALSE;
}

static void wave_delete(GtkWidget *widget, gpointer user_data)
{
    void** data = (void**)user_data;
    g_source_remove(*((guint*)data[2]));
    free(data[2]);
    free(data);
}

#define NUM_OCTAVES (7.0)
#define NUM_WHITE_KEYS (NUM_OCTAVES * 9.0)

static int next_notenum(int notenum)
{
    switch(notenum %12)
    {
    case 0: // C
    case 2: // D
    case 5: // F
    case 7: // G
    case 9: // A
        notenum += 2;
        break;
    case 4: // E
    case 11:// B
        notenum++;
    }
    return notenum;
}

static gboolean keyboard_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    audio_state* state = (audio_state* )user_data;
    gint width, height;
    GtkStyleContext *context;
    double step, key_w, key_h;
    int notenum;

    context = gtk_widget_get_style_context (widget);
    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    step = width / NUM_WHITE_KEYS;
    key_w = step;
    key_h = height;
    notenum = 12;
    for(double i=0; i<width; i+=step)
    {
        if(notenum == state->noteon)
        {
            cairo_set_source_rgb(cr, 1, 0, 0);
        }
        else
        {
            cairo_set_source_rgb(cr, 1, 1, 1);
        }

        cairo_rectangle(cr, i, 0, key_w, key_h);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0.75, 0.75, 0.75);
        cairo_stroke(cr);
        notenum = next_notenum(notenum);
    }


    key_w *= 0.6;
    key_h *= 0.6;
    notenum = 13;

    for(double i=step*0.70; i< width; )
    {
        for(int j=0; j<2; j++)
        {
            if(notenum == state->noteon)
            {
                cairo_set_source_rgb(cr, 1, 0, 0);
            }
            else {
                cairo_set_source_rgb(cr, 0.25, 0.25, 0.25);
            }
            cairo_rectangle(cr, i, 0, key_w, key_h);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);

            i+= step;
            notenum +=2;
        }
        i+=step;
        notenum ++;
        for(int j=0; j<3; j++)
        {
            if(notenum == state->noteon)
            {
                cairo_set_source_rgb(cr, 1, 0, 0);
            }
            else {
                cairo_set_source_rgb(cr, 0.25, 0.275, 0.25);
            }
            cairo_rectangle(cr, i, 0, key_w, key_h);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_stroke(cr);

            i+= step;
            notenum+=2;
        }
        i+=step;
        notenum++;
    }


    return FALSE;
}


static gboolean keyboard_key_pressed(GdkEvent* event, editor_state* state,
                                     double x, double y,double width, double height,
                                     u8 notenum)
{
    u8 velocity;
    if(event->button.button == GDK_BUTTON_PRIMARY &&
        event->button.x > x && event->button.y > 0 &&
        event->button.x < x+width && event->button.y < height)
    {
        velocity = (event->button.y - y) * 127 / height;
        state->state->noteon = notenum;
        ks_synth_set(&state->state->fm, state->state->sampling_rate,  &state->data);
        ks_synth_note_on(&state->state->note, &state->state->fm,  state->state->sampling_rate, notenum, velocity);
        return TRUE;
    }
    return FALSE;
}

static gboolean keyboard_press(GtkWidget *widget,
              GdkEvent  *event,
              gpointer   user_data)
{
    editor_state* state = (editor_state* )user_data;
    guint width, height;
    double step, key_w, key_h;
    int notenum;

    if (gtk_widget_get_window(widget) != NULL){
        gtk_widget_queue_draw(widget);
    }

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    step = width / NUM_WHITE_KEYS;
    key_w = step;
    key_h = height;
    key_w *= 0.6;
    key_h *= 0.6;

    notenum = 13;
    for(double i=step*0.70; i< width; )
    {
        for(int j=0; j<2; j++)
        {
            if(keyboard_key_pressed(event, state, i, 0, key_w, key_h, notenum))
            {
                return FALSE;
            }
            notenum += 2;
            i+= step;
        }
        notenum += 1;
        i+=step;
        for(int j=0; j<3; j++)
        {
            if (keyboard_key_pressed(event, state, i, 0, key_w, key_h, notenum))
            {
                return FALSE;
            }
            notenum += 2;
            i+= step;
        }
        i+=step;
        notenum +=1;
    }

    notenum = 12;
    key_w /= 0.6;
    key_h /= 0.6;
    for(double i=0; i<width; i+=step)
    {
        if (keyboard_key_pressed(event, state, i, 0, key_w, key_h, notenum))
        {
            return FALSE;
        }
        notenum = next_notenum(notenum);
    }

    return FALSE;
}


static gboolean keyboard_release(GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer   user_data)
{
    editor_state* state = (editor_state* )user_data;

    if (gtk_widget_get_window(widget) != NULL){
        gtk_widget_queue_draw(widget);
    }

    state->state->noteon = -1;
    ks_synth_note_off(&state->state->note);

    return FALSE;
}

void internal_process(audio_state* state, ALuint buffer){
    ks_synth_render(&state->fm, &state->note, ks_fms_depth( ks_v(0, KS_LFO_DEPTH_BITS) ), state->buf, NUM_CHANNELS*NUM_SAMPLES);

    alBufferData(buffer, AL_FORMAT_MONO16, state->buf, sizeof(state->buf), SAMPLE_RATE);
    alSourceQueueBuffers(state->source, 1, &buffer);
}

static gboolean audio_stream_update(gpointer ptr)
{
    void **user_data =ptr;
    GtkWidget* widget = GTK_WIDGET(user_data[0]);
    audio_state* state = (audio_state*)(user_data[1]);
    if (gtk_widget_get_window(widget) == NULL){
        return FALSE;
    }

    gtk_widget_queue_draw(widget);



    if (state->is_playing)
    {
        int processed;
        alGetSourcei(state->source, AL_BUFFERS_PROCESSED, &processed);
        while (processed > 0)
        {
            ALuint buffer;
            alSourceUnqueueBuffers(state->source, 1, &buffer);
            internal_process(state, buffer);
            processed--;
        }
    }

    return (TRUE);
}

audio_state* audio_state_new()
{
    audio_state *ret = malloc(sizeof(audio_state));
    ks_synth_data data;

    memset(ret, 0, sizeof(audio_state));

    ret->device = alcOpenDevice(NULL);
    ret->context = alcCreateContext(ret->device, NULL);
    alcMakeContextCurrent(ret->context);

    if(alGetError() != AL_NO_ERROR)
    {
        goto error;
    }

    ret->noteon = -1;
    ret->sampling_rate = SAMPLE_RATE;

    ks_synth_data_set_default(&data);
    ks_synth_set(&ret->fm, ret->sampling_rate, &data);

    memset(&ret->note, 0, sizeof(ret->note));

    alGenBuffers(4, ret->buffers);
    alGenSources(1, &ret->source);
    alSourcef(ret->source, AL_GAIN, 1);
    alSource3f(ret->source, AL_POSITION, 0, 0, 0);

    for (int i=0; i<4; i++)
    {
        internal_process(ret, ret->buffers[i]);
    }
    alSourcePlay(ret->source);
    ret->is_playing = true;

    return ret;
error:
    alcDestroyContext(ret->context);
    alcCloseDevice(ret->device);
    free(ret);
    return NULL;
}

void audio_state_free(audio_state* state)
{
    alcDestroyContext(state->context);
    alcCloseDevice(state->device);
    free(state);
}

GtkWidget* wave_viewer_new(audio_state* state)
{
    GtkWidget* drawing = gtk_drawing_area_new();
    guint *callback_id = malloc(sizeof(guint));
    void** user_data = malloc(sizeof(void*)*3);

    user_data[0] = drawing;
    user_data[1] = state;
    user_data[2] = callback_id;
    gtk_widget_set_size_request(drawing, 200, 100);
    *callback_id = g_timeout_add(50, audio_stream_update,  user_data);

    g_signal_connect(drawing, "draw", G_CALLBACK(wave_draw), state);
    g_signal_connect(drawing, "destroy", G_CALLBACK(wave_delete), user_data);

    return drawing;
}

GtkWidget* keyboard_new(editor_state* state)
{
    GtkWidget* drawing = gtk_drawing_area_new();
    gtk_widget_set_can_focus(drawing, true);
    gtk_widget_set_events(drawing, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    gtk_widget_set_size_request(drawing, 200, 100);
    g_signal_connect(drawing, "button-press-event", G_CALLBACK(keyboard_press), state);
    g_signal_connect(drawing, "button-release-event", G_CALLBACK(keyboard_release), state);
    g_signal_connect(drawing, "draw", G_CALLBACK(keyboard_draw), state->state);

    return drawing;
}
