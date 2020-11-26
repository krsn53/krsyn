#ifndef FM_DATA_PARAMS_H
#define FM_DATA_PARAMS_H

#include "editor_state.h"
#include "param_editor.h"
#include <gtk/gtk.h>
#include "../krsyn.h"

/**
 * @brief global_params_new 共通のパラメータ軍を編集する。
 * @param window　編集するウインドウ。
 * @param data　編集するデータのポインタ。
 * @return GtkWidget* 共通パラメータが編集できるウィジェット。
 */
GtkWidget* common_params_new(editor_state* state);

/**
 * @brief operator_params_new オペレータのパラメータ軍を編集する。
 * @param window 編集するウィンドウ。
 * @param data 編集するデータのポインタ。
 * @return GtkWidget* オペレータのパラメータが編集できるウィジェット。
 */
GtkWidget* operator_params_new(editor_state* state);

#endif // FM_DATA_PARAMS_H
