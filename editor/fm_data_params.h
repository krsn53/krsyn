#ifndef FM_DATA_PARAMS_H
#define FM_DATA_PARAMS_H

#include <gtk/gtk.h>
#include "../krsyn.h"

/**
 * @brief add_global_params 共通のパラメータ軍を編集する。
 * @param window　編集するウインドウ。
 * @param data　編集するデータのポインタ。
 */
void add_common_params(GtkWidget* box, krsyn_fm_data* data);

/**
 * @brief add_operator_params オペレータのパラメータ軍を編集する。
 * @param window 編集するウィンドウ。
 * @param data 編集するデータのポインタ。
 */
void add_operator_params(GtkWidget* box, krsyn_fm_data* data);

#endif // FM_DATA_PARAMS_H
