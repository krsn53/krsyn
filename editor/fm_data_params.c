#include "fm_data_params.h"

/**
  * value_chagend
*/
static void operator_fixed_frequency_checked(GtkToggleButton *togglebutton, gpointer user)
{
    KrsynPhaseCoarseData* data = (KrsynPhaseCoarseData*) user;
    data->fixed_frequency = gtk_toggle_button_get_active(togglebutton);
}

static void operator_phase_coarse_changed(GtkRange* range, gpointer user)
{
    KrsynPhaseCoarseData* data = (KrsynPhaseCoarseData*) user;
    data->value = gtk_range_get_value(range);
}

static void ks_curve_left_changed(GtkComboBox* combo, gpointer user)
{
    KrsynKSCurveTypeData* data = (KrsynKSCurveTypeData*) user;
    data->left = gtk_combo_box_get_active(combo);
}

static void ks_curve_right_changed(GtkComboBox* combo, gpointer user)
{
    KrsynKSCurveTypeData* data = (KrsynKSCurveTypeData*) user;
    data->right = gtk_combo_box_get_active(combo);
}

static void enum_param_value_changed(GtkComboBox* combo, gpointer user)
{
    uint8_t* data = (uint8_t*) user;
    *data = gtk_combo_box_get_active(combo);
}



/**
  * format-value
*/
static gchar* phase_coarse_format(GtkScale* scale, gdouble value, gpointer user)
{
    KrsynPhaseCoarseData* data = (KrsynPhaseCoarseData*) user;
    if(data->fixed_frequency)
    {
        return g_strdup_printf ( "%d", (int)value);
    }
    return   g_strdup_printf ( "%.1f", value / 2.0f);
}

static gchar* zero_one_format(GtkScale* scale, gdouble value, gpointer user)
{
    return g_strdup_printf( "%.3f", value / 256.0f );
}

static gchar* zero_one_format2(GtkScale* scale, gdouble value, gpointer user)
{
    return g_strdup_printf( "%.3f", value / 255.0f );
}

static gchar* feedback_level_format(GtkScale* scale, gdouble value, gpointer user)
{
    return  g_strdup_printf( "%.2f%s",  4.0f * value / 256.0f, "π" );
}

static gchar* phase_det_format(GtkScale* scale, gdouble value, gpointer user)
{
    return g_strdup_printf ( "%.1f%s", value*360.0f / 256.0f, "°");
}

static gchar* lfo_freq_format(GtkScale* scale, gdouble value, gpointer user)
{
    double time = calc_lfo_freq((uint8_t)value) / (double)(1<<16);
    if(time < 1.0)
    {
        return g_strdup_printf ("%.1f%s", time*1000.0, "mHz");
    }
    return g_strdup_printf ("%.1f%s", time, "Hz");
}

static gchar* envelop_time_format(GtkScale* scale, gdouble value, gpointer user)
{
    double time = krsyn_calc_envelop_times(value) / (double)(1<<16);
    if(time < 0.1)
    {
        return g_strdup_printf("%.2f%s", time*1000.0, "msec");
    }
    return g_strdup_printf("%.2f%s", time, "sec");
}

/**
  * widget_set
*/
static void fixed_frequency_widget_set(GtkWidget* widget, void*param)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), ((KrsynPhaseCoarseData*)param)->fixed_frequency);
}


static void phase_coarse_widget_set(GtkWidget* widget, void*param)
{
    gtk_range_set_value(GTK_RANGE(widget), ((KrsynPhaseCoarseData*)param)->value);
}

static void ks_curve_left_widget_set(GtkWidget* widget, void*param)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), ((KrsynKSCurveTypeData*)param)->left);
}

static void ks_curve_right_widget_set(GtkWidget* widget, void*param)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), ((KrsynKSCurveTypeData*)param)->right);
}

/**
  * add label
*/
static void add_operator_label(GtkGrid* grid, const char* param_name, int left, int top)
{
    GtkWidget* label = gtk_label_new(param_name);
    gtk_grid_attach(grid, label, left, top, 1 ,1);
}


/**
  * add widgets
*/
static void add_operator_fixed_frequency( GtkGrid* grid, const char* param_name, KrsynPhaseCoarseData* param_ptr, int top)
{
    add_operator_label(grid, param_name, 0, top);

    for(int i=0; i < KRSYN_NUM_OPERATORS; i++)
    {
        GtkWidget* check = gtk_check_button_new();

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), param_ptr[i].fixed_frequency);
        gtk_grid_attach(grid, check, i+1, top, 1, 1);

        g_signal_connect(check, "update-params", G_CALLBACK(fixed_frequency_widget_set), param_ptr + i);
        g_signal_connect(check, "toggled", G_CALLBACK(operator_fixed_frequency_checked), param_ptr + i);
    }
}

static void add_operator_enum_param_base( GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                         void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkComboBox*, gpointer),
                                         GtkCellRenderer* colmun, GType list_type,
                                         void*enum_data[], int list_num,
                                         int top)
{
    add_operator_label(grid, param_name, 0, top);

    for(int i=0; i < KRSYN_NUM_OPERATORS; i++)
    {
        GtkWidget* combo = enum_param_new(param_ptr+i,
                                          widget_set, setter,
                                          colmun, list_type,
                                          enum_data, list_num);
        g_signal_connect(combo, "update-params", G_CALLBACK(widget_set), param_ptr+i);
        gtk_grid_attach(grid, combo, i+1, top, 1, 1);
    }
}

static void add_operator_param_base( GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                    gchar*(* format)(GtkScale*, gdouble, gpointer),
                                      void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkRange*, gpointer),
                                    int min, int max, int top)
{
    add_operator_label(grid, param_name, 0, top);

    for(int i=0; i < KRSYN_NUM_OPERATORS; i++)
    {
        GtkWidget* scale = param_new(param_ptr+i, format, widget_set, setter, min, max);
        g_signal_connect(scale, "update-params", G_CALLBACK(widget_set), param_ptr+i);
        gtk_grid_attach(grid, scale, i+1, top, 1, 1);
    }
}

static void add_operator_phase_coarse( GtkGrid* grid, const char* param_name, KrsynPhaseCoarseData* param_ptr, int top)
{
    add_operator_param_base(grid, param_name, (uint8_t*)param_ptr, phase_coarse_format,
                            phase_coarse_widget_set, operator_phase_coarse_changed,
                            0, 127, top);
}

static void add_operator_param( GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                               gchar*(* format)(GtkScale*, gdouble, gpointer), int max, int top)
{
    add_operator_param_base(grid, param_name, param_ptr, format,
                            uint8_range_widget_set, uint8_value_changed,
                            0, max, top);
}


static void add_param_with_label_base( GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                      gchar*(* format)(GtkScale*, gdouble, gpointer),
                                 void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkRange*, gpointer),
                                 int min, int max, int top)
{
    GtkWidget* label = gtk_label_new(param_name);
    GtkWidget* param = param_new(param_ptr, format, widget_set, setter, min, max);

    g_signal_connect(param, "update-params", G_CALLBACK(widget_set), param_ptr);

    gtk_grid_attach(grid, label, 0, top, 1, 1);
    gtk_grid_attach(grid, param, 1, top, 1, 1);
}

static void add_enum_param_with_label_base( GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                    void(*widget_set)(GtkWidget*, void*), void(*setter)(GtkComboBox*, gpointer),
                                    GtkCellRenderer* colmun, GType list_type,
                                    void* enum_data[], int list_num,
                                    int top)
{
    GtkWidget* label = gtk_label_new(param_name);
    GtkWidget* enum_param = enum_param_new(param_ptr,
                                           widget_set, setter,
                                           colmun, list_type,
                                           enum_data, list_num);
    g_signal_connect(enum_param, "update-params", G_CALLBACK(widget_set), param_ptr);

    gtk_grid_attach(grid, label, 0, top, 1, 1);
    gtk_grid_attach(grid, enum_param, 1, top, 1, 1);
}

static void* ks_curve_types[] ={
    "Linear -",
    "Exp -",
    "Linear +",
    "Exp +",
};

static void* lfo_table_types[] ={
    "Triangle",
    "Saw Up",
    "Saw Down",
    "Square",
    "Sin",
};



static void add_ks_curve_left_param( GtkGrid* grid, const char* param_name, KrsynKSCurveTypeData* param_ptr, int top)
{
    add_operator_enum_param_base(grid, param_name, (uint8_t*)param_ptr,
                                 ks_curve_left_widget_set, ks_curve_left_changed,
                                 gtk_cell_renderer_text_new(), G_TYPE_STRING,
                                 ks_curve_types, KRSYN_KS_CURVE_NUM_TYPES,
                                 top);
}

static void add_ks_curve_right_param( GtkGrid* grid, const char* param_name, KrsynKSCurveTypeData* param_ptr, int top)
{
    add_operator_enum_param_base(grid, param_name, (uint8_t*)param_ptr,
                                 ks_curve_right_widget_set, ks_curve_right_changed,
                                 gtk_cell_renderer_text_new(), G_TYPE_STRING,
                                 ks_curve_types, KRSYN_KS_CURVE_NUM_TYPES,
                                 top);
}

static void add_param( GtkGrid* grid,
                      const char* param_name, uint8_t* param_ptr,
                      gchar*(* format)(GtkScale*, gdouble, gpointer),
                      int max, int top)
{
    add_param_with_label_base(grid, param_name, param_ptr, format,
                              uint8_range_widget_set, uint8_value_changed,
                              0, max, top);
}

static void add_enum_param( GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                           void* enum_data[], int list_num, int top)
{
    add_enum_param_with_label_base(grid, param_name, param_ptr,
                                   uint8_combo_widget_set, enum_param_value_changed,
                                   gtk_cell_renderer_text_new(), G_TYPE_STRING,
                                   enum_data, list_num,
                                   top);
}

static void add_enum_pixbuf_param( GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                  void* enum_data[], int list_num, int top)
{
    add_enum_param_with_label_base(grid, param_name, param_ptr,
                                   uint8_combo_widget_set, enum_param_value_changed,
                                   gtk_cell_renderer_pixbuf_new(), G_TYPE_OBJECT,
                                   enum_data, list_num,
                                   top);
}

GtkWidget* common_params_new(EditorState *state)
{
    GtkWidget *grid_widget = gtk_grid_new();
    GtkGrid *grid = GTK_GRID(grid_widget);

    void* algorithm_types[] =
    {
            gdk_pixbuf_new_from_file("resource/images/algorithm_00.png", NULL),
            gdk_pixbuf_new_from_file("resource/images/algorithm_01.png", NULL),
            gdk_pixbuf_new_from_file("resource/images/algorithm_02.png", NULL),
            gdk_pixbuf_new_from_file("resource/images/algorithm_03.png", NULL),
            gdk_pixbuf_new_from_file("resource/images/algorithm_04.png", NULL),
            gdk_pixbuf_new_from_file("resource/images/algorithm_05.png", NULL),
            gdk_pixbuf_new_from_file("resource/images/algorithm_06.png", NULL),
            gdk_pixbuf_new_from_file("resource/images/algorithm_07.png", NULL),
            gdk_pixbuf_new_from_file("resource/images/algorithm_08.png", NULL),
            gdk_pixbuf_new_from_file("resource/images/algorithm_09.png", NULL),
    };

    int row = 0;

    gtk_grid_set_column_homogeneous(grid, true);

    gtk_grid_attach(grid, gtk_label_new("Common"), 0, row++, 2, 1);

    add_enum_pixbuf_param(grid, "Algorithm", &state->data.algorithm, algorithm_types, 10,  row++);
    add_param(grid, "Feedback Level", &state->data.feedback_level, feedback_level_format, 255, row++);
    add_enum_param(grid, "LFO Wave Type", &state->data.lfo_wave_type, lfo_table_types, KRSYN_LFO_NUM_WAVES, row++);
    add_param(grid, "LFO Frequency", &state->data.lfo_freq, lfo_freq_format, 255, row++);
    add_param(grid, "FLO Det", &state->data.lfo_det, phase_det_format, 255, row++);
    add_param(grid, "LFO FMS Depth", &state->data.lfo_fms_depth, zero_one_format2, 255, row++);

    return grid_widget;
}

GtkWidget*  operator_params_new(EditorState *state)
{
    GtkWidget *grid_widget = gtk_grid_new();
    GtkGrid *grid = GTK_GRID(grid_widget);
    int row = 0;
    // 列のサイズを揃える。
    gtk_grid_set_column_homogeneous(grid, true);

    //オペレータナンバー表示
    for(int i=1; i<=4; i++)
    {
        gchar* str = g_strdup_printf("Operator %d", i);
        GtkWidget* label = gtk_label_new(str);
        g_free(str);
        gtk_grid_attach(grid, label, i, row, 1, 1);
    }
    row ++;

    // パラメータ一覧表示
    add_operator_fixed_frequency(grid, "Fixed Frequency", state->data.phase_coarses, row++);
    add_operator_phase_coarse(grid, "Phase Coarse", state->data.phase_coarses, row++);
    add_operator_param(grid, "Phase Fine", state->data.phase_fines, zero_one_format, 255, row++);
    add_operator_param(grid, "Phase Dat", state->data.phase_dets, phase_det_format, 255, row++);

    add_operator_param(grid, "Attack Level", state->data.envelop_points[0], zero_one_format2, 255, row++);
    add_operator_param(grid, "Decay Level", state->data.envelop_points[1], zero_one_format2, 255, row++);
    add_operator_param(grid, "Sustain Level 1", state->data.envelop_points[2], zero_one_format2, 255, row++);
    add_operator_param(grid, "Sustain Level 2", state->data.envelop_points[3], zero_one_format2,  255, row++);
    add_operator_param(grid, "Attack Time", state->data.envelop_times[0], envelop_time_format, 255, row++);
    add_operator_param(grid, "Decay Time", state->data.envelop_times[1], envelop_time_format,  255, row++);
    add_operator_param(grid, "Sustain Time 1", state->data.envelop_times[2], envelop_time_format, 255, row++);
    add_operator_param(grid, "Sustain Time 2", state->data.envelop_times[3], envelop_time_format, 255, row++);
    add_operator_param(grid, "Release Time", state->data.envelop_release_times, envelop_time_format, 255, row++);

    add_operator_param(grid, "Rate Scale", state->data.ratescales, zero_one_format2, 255, row++);
    add_operator_param(grid, "KS Low Depth", state->data.ks_low_depths, zero_one_format2, 255, row++);
    add_operator_param(grid, "KS High Depth", state->data.ks_high_depths, zero_one_format2, 255, row++);
    add_operator_param(grid, "KS Mid Point", state->data.ks_mid_points, NULL, 127, row++);
    add_ks_curve_left_param(grid, "KS Type Left", state->data.ks_curve_types, row++);
    add_ks_curve_right_param(grid, "KS Type Right", state->data.ks_curve_types, row++);

    add_operator_param(grid, "LFO AMS Depth", state->data.lfo_ams_depths,  zero_one_format2, 255, row++);

    return grid_widget;
}
