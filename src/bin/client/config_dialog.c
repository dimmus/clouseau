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
   Evas_Object *popup = data;

   if (_cfg_changed)
      _conf_changed_cb(_conf_changed_cb_data);

   _conf_changed_cb = NULL;
   _conf_changed_cb_data = NULL;

   evas_object_del(popup);
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
   Evas_Object *popup, *bx, *check, *btn;

   _cfg_changed = EINA_FALSE;
   _conf_changed_cb = callback;
   _conf_changed_cb_data = (void *) callback_data;

   popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text", "Clouseau Settings");
   evas_object_show(popup);

   bx = elm_box_add(popup);
   elm_object_content_set(popup, bx);
   evas_object_show(bx);

   check = elm_check_add(bx);
   elm_object_text_set(check, "Show Hidden");
   elm_check_state_set(check, _clouseau_cfg->show_hidden);
   evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(check, 0.0, 0.5);
   elm_box_pack_end(bx, check);
   evas_object_show(check);
   evas_object_smart_callback_add(check, "changed",
         _config_check_changed, &(_clouseau_cfg->show_hidden));

   check = elm_check_add(bx);
   elm_object_text_set(check, "Show Clippers");
   elm_check_state_set(check, _clouseau_cfg->show_clippers);
   evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(check, 0.0, 0.5);
   elm_box_pack_end(bx, check);
   evas_object_show(check);
   evas_object_smart_callback_add(check, "changed",
         _config_check_changed, &(_clouseau_cfg->show_clippers));

   check = elm_check_add(bx);
   elm_object_text_set(check, "Only show Elementary widgets");
   elm_check_state_set(check, _clouseau_cfg->show_elm_only);
   evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(check, 0.0, 0.5);
   elm_box_pack_end(bx, check);
   evas_object_show(check);
   evas_object_smart_callback_add(check, "changed",
         _config_check_changed, &(_clouseau_cfg->show_elm_only));

   btn = elm_button_add(bx);
   elm_object_text_set(btn, "Close");
   elm_object_part_content_set(popup, "button1", btn);
   evas_object_show(btn);
   evas_object_smart_callback_add(btn, "clicked",
         _close_btn_clicked, popup);
}
