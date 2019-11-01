#include "fm_data_params.h"

/**
  * Callbacks
*/
static void operator_fixed_frequency_checked(GtkToggleButton *togglebutton, gpointer user)
{
    krsyn_phase_coarse_data* data = (krsyn_phase_coarse_data*) user;
    data->fixed_frequency = gtk_toggle_button_get_active(togglebutton);
}

static void operator_phase_coarse_changed(GtkRange* range, gpointer user)
{
    krsyn_phase_coarse_data* data = (krsyn_phase_coarse_data*) user;
    data->value = gtk_range_get_value(range);
}

static void ks_curve_left_changed(GtkComboBox* combo, gpointer user)
{
    krsyn_ks_curve_type_data* data = (krsyn_ks_curve_type_data*) user;
    data->left = gtk_combo_box_get_active(combo);
}

static void ks_curve_right_changed(GtkComboBox* combo, gpointer user)
{
    krsyn_ks_curve_type_data* data = (krsyn_ks_curve_type_data*) user;
    data->right = gtk_combo_box_get_active(combo);
}

static void enum_param_value_changed(GtkComboBox* combo, gpointer user)
{
    uint8_t* data = (uint8_t*) user;
    *data = gtk_combo_box_get_active(combo);
}

static void param_value_changed(GtkRange* range, gpointer user)
{
    uint8_t* data = (uint8_t*) user;
    *data = gtk_range_get_value(range);
}

/**
  * getter
*/
static uint8_t uint8_getter(void*param)
{
    return *((uint8_t*)param);
}

static uint8_t phase_coarse_getter(void*param)
{
    return ((krsyn_phase_coarse_data*)param)->value;
}

static uint8_t ks_curve_left_getter(void*param)
{
    return ((krsyn_ks_curve_type_data*)param)->left;
}

static uint8_t ks_curve_right_getter(void*param)
{
    return ((krsyn_ks_curve_type_data*)param)->right;
}

/**
  * add label
*/
static void add_operator_label(GtkGrid* grid, const char* param_name, int left, int top)
{
    GtkWidget* label = gtk_label_new(param_name);
    gtk_grid_attach(grid, label, left, top, 1 ,1);
}


static GtkWidget* param_new(uint8_t* param_ptr,
                           uint8_t(*getter)(void*), void(*setter)(GtkRange*, gpointer),
                           int min, int max)
{
    GtkWidget* scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, 1);
    gboolean t;

    gtk_range_set_value(GTK_RANGE(scale), getter(param_ptr));
    g_signal_emit_by_name(scale, "value-changed", G_CALLBACK(setter), param_ptr, &t);

    return scale;
}

typedef struct combo_attribute
{
    const char* name;
    unsigned num;
}combo_attribute;

static GtkWidget* enum_param_new(uint8_t* param_ptr,
                                uint8_t(*getter)(void*), void(*setter)(GtkComboBox*, gpointer),
                                 GtkCellRenderer *column, unsigned num_colmun,
                                 GType list_types[], const combo_attribute attributes[],
                                 void*enum_data[], int list_num)
{
    GtkListStore* list = gtk_list_store_newv(num_colmun, list_types);
    GtkWidget* combo;
    gboolean t;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));

    for(int j=0; j < list_num; j++)
    {
        for(int k=0; k < num_colmun; k++)
        {
            gtk_list_store_insert_with_values(list, NULL, -1, 0, enum_data[j*num_colmun + k], -1);
        }
    }

    // listはcomboに所有されているため初期リファレンスを解除してやる必要があるらしい。
    // 参考　https://stackoverflow.com/questions/16630528/trying-to-populate-a-gtkcombobox-with-model-in-c
    g_object_unref(list);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), column, TRUE);

    for(int j=0; j < num_colmun; j++)
    {
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), column, attributes[j].name, attributes[j].num, NULL);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), getter(param_ptr));
    g_signal_emit_by_name(combo, "changed", G_CALLBACK(setter), param_ptr, &t);

    return combo;
}




/**
  * add widgets
*/
static void add_operator_fixed_frequency(GtkGrid* grid, const char* param_name, krsyn_phase_coarse_data* param_ptr, int top)
{
    add_operator_label(grid, param_name, 0, top);

    for(int i=0; i < KRSYN_NUM_OPERATORS; i++)
    {
        GtkWidget* check = gtk_check_button_new();
        gboolean t;

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), param_ptr[i].fixed_frequency);
        gtk_grid_attach(grid, check, i+1, top, 1, 1);

        g_signal_emit_by_name(check, "toggled", G_CALLBACK(operator_fixed_frequency_checked), &param_ptr[i], &t);
    }
}

static void add_operator_enum_param_base(GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                         uint8_t(*getter)(void*), void(*setter)(GtkComboBox*, gpointer),
                                         GtkCellRenderer* colmun, unsigned num_colmun,
                                         GType list_types[], const combo_attribute attributes[],
                                         void*enum_data[], int list_num,
                                         int top)
{
    add_operator_label(grid, param_name, 0, top);

    for(int i=0; i < KRSYN_NUM_OPERATORS; i++)
    {
        GtkWidget* combo = enum_param_new(param_ptr+i,
                                          getter, setter,
                                          colmun, num_colmun,
                                          list_types, attributes,
                                          enum_data, list_num);
        gtk_grid_attach(grid, combo, i+1, top, 1, 1);
    }
}

static void add_operator_param_base(GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                      uint8_t(*getter)(void*), void(*setter)(GtkRange*, gpointer),
                                    int min, int max, int top)
{
    add_operator_label(grid, param_name, 0, top);

    for(int i=0; i < KRSYN_NUM_OPERATORS; i++)
    {
        GtkWidget* scale = param_new(param_ptr+i, getter, setter, min, max);
        gtk_grid_attach(grid, scale, i+1, top, 1, 1);
    }
}

static void add_operator_phase_coarse(GtkGrid* grid, const char* param_name, krsyn_phase_coarse_data* param_ptr, int top)
{
    add_operator_param_base(grid, param_name, (uint8_t*)param_ptr,
                            phase_coarse_getter, operator_phase_coarse_changed,
                            0, 127, top);
}

static void add_operator_param(GtkGrid* grid, const char* param_name, uint8_t* param_ptr, int max, int top)
{
    add_operator_param_base(grid, param_name, param_ptr,
                            uint8_getter, param_value_changed,
                            0, max, top);
}


static void add_param_with_label_base(GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                 uint8_t(*getter)(void*), void(*setter)(GtkRange*, gpointer),
                                 int min, int max, int top)
{
    GtkWidget* label = gtk_label_new(param_name);
    GtkWidget* param = param_new(param_ptr, getter, setter, min, max);

    gtk_grid_attach(grid, label, 0, top, 1, 1);
    gtk_grid_attach(grid, param, 1, top, 1, 1);
}

static void add_enum_param_with_label_base(GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                    uint8_t(*getter)(void*), void(*setter)(GtkComboBox*, gpointer),
                                    GtkCellRenderer* colmun, unsigned num_colmun,
                                           GType list_types[], const combo_attribute attributes[],
                                    void* enum_data[], int list_num,
                                    int top)
{
    GtkWidget* label = gtk_label_new(param_name);
    GtkWidget* enum_param = enum_param_new(param_ptr,
                                           getter, setter,
                                           colmun, num_colmun,
                                           list_types, attributes,
                                           enum_data, list_num);

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

static combo_attribute combo_text[] ={
    {.name="text", .num=0},
};

static combo_attribute combo_pixbuf[]={
    {.name="pixbuf", .num=0},
    };

static GType combo_text_type_list[] ={
    G_TYPE_STRING,
};

static GType combo_pixbuf_type_list[] ={
    G_TYPE_OBJECT,
};

static void add_ks_curve_left_param(GtkGrid* grid, const char* param_name, krsyn_ks_curve_type_data* param_ptr, int top)
{
    add_operator_enum_param_base(grid, param_name, (uint8_t*)param_ptr,
                                 ks_curve_left_getter, ks_curve_left_changed,
                                 gtk_cell_renderer_text_new(), 1, combo_text_type_list, combo_text,
                                 ks_curve_types, KRSYN_KS_CURVE_NUM_TYPES,
                                 top);
}

static void add_ks_curve_right_param(GtkGrid* grid, const char* param_name, krsyn_ks_curve_type_data* param_ptr, int top)
{
    add_operator_enum_param_base(grid, param_name, (uint8_t*)param_ptr,
                                 ks_curve_right_getter, ks_curve_right_changed,
                                 gtk_cell_renderer_text_new(), 1, combo_text_type_list, combo_text,
                                 ks_curve_types, KRSYN_KS_CURVE_NUM_TYPES,
                                 top);
}

static void add_param(GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                      int max, int top)
{
    add_param_with_label_base(grid, param_name, param_ptr,
                              uint8_getter, param_value_changed,
                              0, max, top);
}

static void add_enum_param(GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                           void* enum_data[], int list_num, int top)
{
    add_enum_param_with_label_base(grid, param_name, param_ptr,
                                   uint8_getter, enum_param_value_changed,
                                   gtk_cell_renderer_text_new(), 1,
                                   combo_text_type_list, combo_text,
                                   enum_data, list_num,
                                   top);
}

static void add_enum_pixbuf_param(GtkGrid* grid, const char* param_name, uint8_t* param_ptr,
                                  void* enum_data[], int list_num, int top)
{
    add_enum_param_with_label_base(grid, param_name, param_ptr,
                                   uint8_getter, enum_param_value_changed,
                                   gtk_cell_renderer_pixbuf_new(), 1,
                                   combo_pixbuf_type_list, combo_pixbuf,
                                   enum_data, list_num,
                                   top);
}

void add_common_params(GtkWidget* box, krsyn_fm_data* data)
{
    GtkWidget *grid_widget = gtk_grid_new();
    GtkGrid *grid = GTK_GRID(grid_widget);

    void* algorithm_types[] =
    {
            gdk_pixbuf_new_from_file("test.bmp", NULL),
            gdk_pixbuf_new_from_file("test.bmp", NULL),
            gdk_pixbuf_new_from_file("test.bmp", NULL),
            gdk_pixbuf_new_from_file("test.bmp", NULL),
            gdk_pixbuf_new_from_file("test.bmp", NULL),
            gdk_pixbuf_new_from_file("test.bmp", NULL),
            gdk_pixbuf_new_from_file("test.bmp", NULL),
            gdk_pixbuf_new_from_file("test.bmp", NULL),
            gdk_pixbuf_new_from_file("test.bmp", NULL),
            gdk_pixbuf_new_from_file("test.bmp", NULL),
    };

    int row = 0;

    gtk_grid_attach(grid, gtk_label_new("Common"), 0, row++, 2, 1);

    add_enum_pixbuf_param(grid, "Algorithm", &data->algorithm, algorithm_types, 10,  row++);
    add_param(grid, "Feedback Level", &data->feedback_level, 255, row++);
    add_enum_param(grid, "LFO Wave Type", &data->lfo_table_type, lfo_table_types, KRSYN_LFO_NUM_WAVES, row++);
    add_param(grid, "LFO Frequency", &data->lfo_freq, 255, row++);
    add_param(grid, "FLO Det", &data->lfo_det, 255, row++);
    add_param(grid, "LFO FMS Depth", &data->lfo_fms_depth, 255, row++);

    gtk_container_add(GTK_CONTAINER(box), grid_widget);
}

void  add_operator_params(GtkWidget* box, krsyn_fm_data *data)
{
    GtkWidget *grid_widget = gtk_grid_new();
    GtkGrid *grid = GTK_GRID(grid_widget);
    int row = 0;
    // 列のサイズを揃える。
    gtk_grid_set_column_homogeneous(grid, true);

    //オペレータナンバー表示
    for(int i=1; i<=4; i++)
    {
        GtkWidget* label = gtk_label_new(g_strdup_printf("Operator %d", i));
        gtk_grid_attach(grid, label, i, row, 1, 1);
    }
    row ++;

    // パラメータ一覧表示
    add_operator_fixed_frequency(grid, "Fixed Frequency", data->phase_coarses, row++);
    add_operator_phase_coarse(grid, "Phase Coarse", data->phase_coarses, row++);
    add_operator_param(grid, "Phase Fine", data->phase_fines, 255, row++);
    add_operator_param(grid, "Phase Dat", data->phase_dets, 255, row++);

    add_operator_param(grid, "Attack Level", data->envelop_points[0], 255, row++);
    add_operator_param(grid, "Decay Level", data->envelop_points[1], 255, row++);
    add_operator_param(grid, "Sustain Level 1", data->envelop_points[2], 255, row++);
    add_operator_param(grid, "Sustain Level 2", data->envelop_points[3], 255, row++);
    add_operator_param(grid, "Attack Time", data->envelop_times[0], 255, row++);
    add_operator_param(grid, "Decay Time", data->envelop_times[1], 255, row++);
    add_operator_param(grid, "Sustain Time 1", data->envelop_times[2], 255, row++);
    add_operator_param(grid, "Sustain Time 2", data->envelop_times[3], 255, row++);
    add_operator_param(grid, "Release Time", data->envelop_release_times, 255, row++);

    add_operator_param(grid, "Rate Scale", data->ratescales, 255, row++);
    add_operator_param(grid, "KS Low Depth", data->ks_low_depths, 255, row++);
    add_operator_param(grid, "KS High Depth", data->ks_high_depths, 255, row++);
    add_operator_param(grid, "KS Mid Point", data->ks_mid_points, 127, row++);
    add_ks_curve_left_param(grid, "KS Type Left", data->ks_curve_types, row++);
    add_ks_curve_right_param(grid, "KS Type Right", data->ks_curve_types, row++);

    gtk_container_add(GTK_CONTAINER(box), grid_widget);
}
