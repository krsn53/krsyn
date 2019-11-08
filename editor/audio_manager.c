#include "audio_manager.h"



static gboolean wave_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    AudioState* state = (AudioState* )user_data;
    guint width, height;
    GtkStyleContext *context;
    double half_height;

    context = gtk_widget_get_style_context (widget);

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    gtk_render_background (context, cr, 0, 0, width, height);
    cairo_set_source_rgb(cr,0,0,0);
    cairo_rectangle(cr, 0, 0,width, height);
    cairo_fill (cr);

    cairo_set_source_rgb(cr, 0, 1, 0);

    half_height= (double)height / width * NUM_SAMPLES * 0.5;
    cairo_scale(cr, (double)width/ NUM_SAMPLES, (double)width/NUM_SAMPLES);
    cairo_set_line_width(cr,  NUM_SAMPLES/(double)width);


    for(int i=0; i<NUM_SAMPLES-1; i++)
    {
        double x1 = i ;
        double y1 =  half_height * state->log[NUM_CHANNELS*i] / INT16_MAX +  half_height;
        double x2 = i + 1;
        double y2 =  half_height * state->log[NUM_CHANNELS*i + NUM_CHANNELS] / INT16_MAX + half_height;

        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    }

    return FALSE;
}

static void wave_delete(GtkWidget *widget, gpointer user_data)
{
    int* id = (int *)user_data;
    g_source_remove(*id);
    free(id);
}

static gboolean ivent_loop(gpointer ptr)
{
    GtkWidget* widget = GTK_WIDGET(ptr);
    if (gtk_widget_get_window(widget) == NULL){
        return FALSE;
    }

    gtk_widget_queue_draw(widget);

    return (TRUE);
}

static int audio_callback(const void *input, void *output,
             unsigned long frameCount,
             const PaStreamCallbackTimeInfo* timeInfo,
             PaStreamCallbackFlags statusFlags,
             void *userData)
{
    AudioState* state = (AudioState* ) userData;
    state->log = (int16_t*) output;
    (void) input;
    krsyn_fm_render(state->core, &state->fm, &state->note, state->log, frameCount*2);

    return 0;
}


#define NUM_OCTAVES (7.0)
#define NUM_WHITE_KEYS (NUM_OCTAVES * 9.0)

static gboolean keyboard_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    AudioState* state = (AudioState* )user_data;
    guint width, height;
    GtkStyleContext *context;
    double step, key_w, key_h;

    context = gtk_widget_get_style_context (widget);

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    step = width / NUM_WHITE_KEYS;
    key_w = step;
    key_h = height;
    for(double i=0; i<width; i+=step)
    {
        cairo_set_source_rgb(cr, 255, 255, 255);
        cairo_rectangle(cr, i, 0, key_w, key_h);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_stroke(cr);
    }
    cairo_set_source_rgb(cr, 0, 0, 0);

    key_w *= 0.6;
    key_h *= 0.6;

    for(double i=step*0.70; i< width; )
    {
        for(int j=0; j<2; j++)
        {
            cairo_rectangle(cr, i, 0, key_w, key_h);
            cairo_fill(cr);

            i+= step;
        }
        i+=step;
        for(int j=0; j<3; j++)
        {
            cairo_rectangle(cr, i, 0, key_w, key_h);
            cairo_fill(cr);

            i+= step;
        }
        i+=step;
    }


    return FALSE;
}


static gboolean keyboard_key_pressed(GdkEvent* event, EditorState* state,
                                     double x, double y,double width, double height,
                                     uint8_t notenum)
{
    uint8_t velocity;
    if(event->button.button == GDK_BUTTON_PRIMARY &&
        event->button.x > x && event->button.y > 0 &&
        event->button.x < x+width && event->button.y < height)
    {
        velocity = (event->button.y - y) * 127 / height;
        krsyn_fm_set(state->state->core, &state->state->fm, &state->data);
        krsyn_fm_note_on(state->state->core, &state->state->fm, &state->state->note, notenum, velocity);
        return TRUE;
    }
    return FALSE;
}

static gboolean keyboard_press(GtkWidget *widget,
              GdkEvent  *event,
              gpointer   user_data)
{
    EditorState* state = (EditorState* )user_data;
    guint width, height;
    double step, key_w, key_h;
    int notenum;

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
    }



    return FALSE;
}


static gboolean keyboard_release(GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer   user_data)
{
    EditorState* state = (EditorState* )user_data;
    krsyn_fm_note_off(&state->state->note);
    return FALSE;
}


AudioState* audio_state_new()
{

    AudioState *ret = malloc(sizeof(AudioState));
    PaError err;
    KrsynFMData data;

    memset(ret, 0, sizeof(AudioState));

    Pa_Initialize();
    ret->core = krsyn_new(SAMPLE_RATE);
    err = Pa_OpenDefaultStream(&ret->stream, 0, NUM_CHANNELS, paInt16, SAMPLE_RATE, NUM_SAMPLES, audio_callback, ret);

    if(err != paNoError) goto error;

    krsyn_fm_set_data_default(&data);
    krsyn_fm_set(ret->core, &ret->fm, &data);

    memset(&ret->note, 0, sizeof(ret->note));

    return ret;

error:
    krsyn_free(ret->core);
    free(ret);
    return NULL;
}

void audio_state_free(AudioState* state)
{
    krsyn_free(state->core);
    Pa_CloseStream(state->stream);
    free(state);
}

GtkWidget* wave_viewer_new(AudioState* state)
{
    GtkWidget* drawing = gtk_drawing_area_new();
    int *callback_id = malloc(sizeof(int));
    gtk_widget_set_size_request(drawing, 200, 100);
    *callback_id = g_timeout_add(100, ivent_loop, drawing);

    g_signal_connect(drawing, "draw", G_CALLBACK(wave_draw), state);
    g_signal_connect(drawing, "destroy", G_CALLBACK(wave_delete), callback_id);

    return drawing;
}

GtkWidget* keyboard_new(EditorState* state)
{
    GtkWidget* drawing = gtk_drawing_area_new();
    gtk_widget_set_can_focus(drawing, true);
    gtk_widget_set_events(drawing, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    gtk_widget_set_size_request(drawing, 200, 100);
    g_signal_connect(drawing, "button-press-event", G_CALLBACK(keyboard_press), state);
    g_signal_connect(drawing, "button-release-event", G_CALLBACK(keyboard_release), state);
    g_signal_connect(drawing, "draw", G_CALLBACK(keyboard_draw), NULL);

    return drawing;
}
