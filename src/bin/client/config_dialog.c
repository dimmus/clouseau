#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>

#include "Clouseau.h"
#include "client/cfg.h"
#include "client/config_dialog.h"

static Clouseau_Config_Changed_Cb _conf_changed_cb = NULL;
static void *_conf_changed_cb_data = NULL;

static Eina_Bool _cfg_changed;

static void
_close_btn_clicked(void *data, Evas_Object *obj EINA_UNUSED,
      void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;

   if (_cfg_changed)
      _conf_changed_cb(_conf_changed_cb_data);

   _conf_changed_cb = NULL;
   _conf_changed_cb_data = NULL;

   evas_object_del(win);
}

static void
_config_check_changed(void *data, Evas_Object *obj,
      void *event_info EINA_UNUSED)
{
   Eina_Bool *setting = data;
   *setting = elm_check_state_get(obj);
   _cfg_changed = EINA_TRUE;
}

void
clouseau_settings_dialog_open(Evas_Object *parent, Clouseau_Config_Changed_Cb callback, const void *callback_data)
{
   Evas_Object *win, *bx;

   _cfg_changed = EINA_FALSE;
   _conf_changed_cb = callback;
   _conf_changed_cb_data = (void *) callback_data;

   win = elm_win_inwin_add(parent);
   evas_object_show(win);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bx);

   elm_win_inwin_content_set(win, bx);

   Evas_Object *check;

   check = elm_check_add(bx);
   elm_object_text_set(check, "Show Hidden");
   elm_check_state_set(check, _clouseau_cfg->show_hidden);
   elm_box_pack_end(bx, check);
   evas_object_show(check);
   evas_object_smart_callback_add(check, "changed",
         _config_check_changed, &(_clouseau_cfg->show_hidden));

   check = elm_check_add(bx);
   elm_object_text_set(check, "Show Clippers");
   elm_check_state_set(check, _clouseau_cfg->show_clippers);
   elm_box_pack_end(bx, check);
   evas_object_show(check);
   evas_object_smart_callback_add(check, "changed",
         _config_check_changed, &(_clouseau_cfg->show_clippers));

   check = elm_check_add(bx);
   elm_object_text_set(check, "Only show Elementary widgets");
   elm_check_state_set(check, _clouseau_cfg->show_elm_only);
   elm_box_pack_end(bx, check);
   evas_object_show(check);
   evas_object_smart_callback_add(check, "changed",
         _config_check_changed, &(_clouseau_cfg->show_elm_only));

   Evas_Object *btn;

   btn = elm_button_add(bx);
   elm_object_text_set(btn, "Close");
   evas_object_size_hint_align_set(bx, 1.0, EVAS_HINT_FILL);
   elm_box_pack_end(bx, btn);
   evas_object_show(btn);
   evas_object_smart_callback_add(btn, "clicked",
         _close_btn_clicked, win);
}
