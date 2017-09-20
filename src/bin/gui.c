#ifndef EFL_BETA_API_SUPPORT
#define EFL_BETA_API_SUPPORT
#endif
#ifndef EFL_EO_API_SUPPORT
#define EFL_EO_API_SUPPORT
#endif
#ifndef ELM_INTERNAL_API_ARGESFSDFEFC
#define ELM_INTERNAL_API_ARGESFSDFEFC
#endif
#include <Elementary.h>
#include "elm_widget_container.h"
#include "elm_interface_scrollable.h"
#include "elm_interface_fileselector.h"
#include "gui.h"

extern void
conn_menu_show(void *data, Evas_Object *obj, void *event_info);
extern void
save_load_perform(void *data, Evas_Object *obj, void *event_info);
extern void
remote_port_entry_changed(void *data, const Efl_Event *event);

static void
_pubs_free_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   free(data);
}

Gui_Main_Win_Widgets *
gui_main_win_create(Eo *__main_parent)
{
   Gui_Main_Win_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *main_win;
   Eo *bg;
   Eo *main_box, *ext_box;
   Eo *tb;
   Eo *ext_selector;
   Eo *freeze_pulse;
   Eo *freeze_inwin;

   main_win = elm_win_add(__main_parent, "Window", ELM_WIN_BASIC);
   pub_widgets->main_win = main_win;
   elm_win_autodel_set(main_win, EINA_TRUE);
   elm_win_title_set(main_win, "Clouseau");
   efl_gfx_size_set(main_win, EINA_SIZE2D(478, 484));

   bg = efl_add(ELM_BG_CLASS, main_win);
   evas_object_size_hint_weight_set(bg, 1.000000, 1.000000);
   efl_gfx_visible_set(bg, EINA_TRUE);
   efl_gfx_position_set(bg, EINA_POSITION2D(0, 0));
   elm_win_resize_object_add(main_win, bg);

   main_box = elm_box_add(main_win);
   evas_object_size_hint_weight_set(main_box, 1.000000, 1.000000);
   efl_gfx_visible_set(main_box, EINA_TRUE);
   elm_win_resize_object_add(main_win, main_box);

   tb = elm_toolbar_add(main_win);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_MENU);
   evas_object_size_hint_weight_set(tb, 0, 0);
   evas_object_size_hint_align_set(tb, -1, 0);
   elm_toolbar_menu_parent_set(tb, main_win);

   pub_widgets->conn_selector = elm_toolbar_item_append(tb, "call-start", "Connection", NULL, NULL);
   elm_toolbar_item_menu_set(pub_widgets->conn_selector, EINA_TRUE);

   pub_widgets->conn_selector_menu = elm_toolbar_item_menu_get(pub_widgets->conn_selector);

   pub_widgets->apps_selector = elm_toolbar_item_append(tb, "view-list-details", "Select App", NULL, NULL);
   elm_toolbar_item_menu_set(pub_widgets->apps_selector, EINA_TRUE);

   pub_widgets->apps_selector_menu = elm_toolbar_item_menu_get(pub_widgets->apps_selector);

   pub_widgets->save_load_bt = elm_toolbar_item_append(tb, "document-export", "Save", save_load_perform, NULL);

   ext_selector = elm_toolbar_item_append(tb, "system-reboot", "Extensions", NULL, NULL);
   elm_toolbar_item_menu_set(ext_selector, EINA_TRUE);

   pub_widgets->ext_selector_menu = elm_toolbar_item_menu_get(ext_selector);

   /*
   Eo *settings_it = elm_toolbar_item_append(tb, "system-run", "Settings", NULL, NULL);
   elm_toolbar_item_menu_set(settings_it, EINA_TRUE);
   */

   elm_box_pack_end(main_box, tb);
   efl_gfx_visible_set(tb, EINA_TRUE);

   freeze_pulse = elm_progressbar_add(main_win);
   pub_widgets->freeze_pulse = freeze_pulse;
   elm_object_style_set(freeze_pulse, "wheel");
   elm_object_text_set(freeze_pulse, "Style: wheel");
   elm_progressbar_pulse_set(freeze_pulse, EINA_TRUE);
   elm_progressbar_pulse(freeze_pulse, EINA_FALSE);
   evas_object_size_hint_align_set(freeze_pulse, 0.5, 0.0);
   evas_object_size_hint_weight_set(freeze_pulse, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(main_win, freeze_pulse);

   ext_box = elm_box_add(main_box);
   pub_widgets->ext_box = ext_box;
   evas_object_size_hint_weight_set(ext_box, 1.000000, 1.000000);
   evas_object_size_hint_align_set(ext_box, -1.000000, -1.000000);
   efl_gfx_visible_set(ext_box, EINA_TRUE);
   elm_box_pack_end(main_box, ext_box);

   freeze_inwin = elm_win_inwin_add(main_win);
   pub_widgets->freeze_inwin = freeze_inwin;
   elm_object_style_set(freeze_inwin, "minimal");

   efl_gfx_visible_set(main_win, EINA_TRUE);
   efl_event_callback_add(main_win, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   return pub_widgets;
}

Gui_Remote_Port_Win_Widgets *
gui_remote_port_win_create(Eo *__main_parent)
{
   Gui_Remote_Port_Win_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *inwin;
   Eo *entry;

   inwin = elm_win_inwin_add(__main_parent);
   pub_widgets->inwin = inwin;

   entry = elm_entry_add(inwin);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_object_part_text_set(entry, "guide", "Port to connect to remote device");
   efl_event_callback_add(entry, ELM_ENTRY_EVENT_ACTIVATED, remote_port_entry_changed, inwin);
   evas_object_show(entry);

   elm_win_inwin_content_set(inwin, entry);
   elm_win_inwin_activate(inwin);

   efl_event_callback_add(inwin, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   return pub_widgets;
}

