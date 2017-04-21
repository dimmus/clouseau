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
#include <elm_fileselector_button.eo.h>
#include "elm_widget_container.h"
#include "elm_interface_scrollable.h"
#include "elm_interface_fileselector.h"
#include "gui.h"

static const char* objs_types_strings[] =
{
   "Show all canvas objects",
   "Only show Elementary widgets",
   NULL
};

static Gui_Widgets g_pub_widgets;

extern void gui_new_profile_win_create_done(Gui_New_Profile_Win_Widgets *wdgs);

#ifdef GUI_IMAGES_PATH
  const char *SHOW_SCREENSHOT_ICON = GUI_IMAGES_PATH"/show-screenshot.png";
  const char *TAKE_SCREENSHOT_ICON = GUI_IMAGES_PATH"/take-screenshot.png";
#else
   #error "Please define GUI_IMAGES_PATH"
#endif

extern void
new_profile_save_cb(void *data, const Efl_Event *event);
extern void
screenshot_req_cb(void *data, const Efl_Event *event);
extern void
conn_menu_show(void *data, Evas_Object *obj, void *event_info);
extern void
reload_perform(void *data, Evas_Object *obj, void *event_info);
extern void
save_load_perform(void *data, Evas_Object *obj, void *event_info);
extern void
jump_entry_changed(void *data, const Efl_Event *event);
extern void
take_screenshot_button_clicked(void *data, const Efl_Event *event);
extern void
show_screenshot_button_clicked(void *data, const Efl_Event *event);
extern void
snapshot_do(void *data, Evas_Object *fs, void *event_info);
extern void
objs_type_changed(void *data, Evas_Object *obj, void *event_info);
extern void
highlight_changed(void *data, Evas_Object *obj, void *event_info);

static void
_pubs_free_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   free(data);
}

static void
_jump_to_ptr_inwin_show(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_win_inwin_activate(data);
}

Gui_Main_Win_Widgets *
gui_main_win_create(Eo *__main_parent)
{
   Gui_Main_Win_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *main_win;
   Eo *elm_bg1;
   Eo *elm_box1;
   Eo *tb;
   Eo *conn_selector;
   Eo *jump2ptr_inwin;
   Eo *jump_to_entry;
   Eo *freeze_pulse;
   Eo *freeze_inwin;
   Eo *elm_panes1;
   Eo *object_infos_list;
   Eo *objects_list;

   main_win = elm_win_add(__main_parent, "Window", ELM_WIN_BASIC);
   pub_widgets->main_win = main_win;
   elm_win_autodel_set(main_win, EINA_TRUE);
   evas_object_size_hint_weight_set(main_win, 1.000000, 1.000000);
   efl_gfx_size_set(main_win, 478, 484);
   elm_bg1 = efl_add(ELM_BG_CLASS, main_win);
   evas_object_size_hint_weight_set(elm_bg1, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_bg1, EINA_TRUE);
   efl_gfx_position_set(elm_bg1, 0, 0);

   elm_box1 = elm_box_add(main_win);
   evas_object_size_hint_weight_set(elm_box1, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_box1, EINA_TRUE);
   efl_gfx_size_set(elm_box1, 643, 598);
   efl_gfx_position_set(elm_box1, -7, -2);
   elm_box_padding_set(elm_box1, 0, 0);
   elm_box_align_set(elm_box1, 0.000000, 0.000000);
   elm_win_resize_object_add(main_win, elm_bg1);
   elm_win_resize_object_add(main_win, elm_box1);

   tb = elm_toolbar_add(elm_box1);
   pub_widgets->tb = tb;
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_MENU);
   elm_toolbar_menu_parent_set(tb, main_win);
   evas_object_size_hint_weight_set(tb, 1.000000, 0.000000);
   evas_object_size_hint_align_set(tb, -1.00000, 0);
   efl_gfx_visible_set(tb, EINA_TRUE);

   conn_selector = elm_toolbar_item_append(tb, "call-start", "Connection", conn_menu_show, NULL);
   pub_widgets->conn_selector = conn_selector;
   elm_toolbar_item_menu_set(conn_selector, EINA_TRUE);
   elm_toolbar_item_priority_set(conn_selector, -9999);

   pub_widgets->conn_selector_menu = elm_toolbar_item_menu_get(conn_selector);

   pub_widgets->reload_button = elm_toolbar_item_append(tb, "view-refresh", "Reload", reload_perform, NULL);

   pub_widgets->apps_selector = elm_toolbar_item_append(tb, "view-list-details", "Select App", NULL, NULL);
   elm_toolbar_item_menu_set(pub_widgets->apps_selector, EINA_TRUE);
   elm_toolbar_item_priority_set(pub_widgets->apps_selector, -9999);

   pub_widgets->apps_selector_menu = elm_toolbar_item_menu_get(pub_widgets->apps_selector);

   jump2ptr_inwin = elm_win_inwin_add(main_win);
   jump_to_entry = elm_entry_add(jump2ptr_inwin);
   elm_entry_scrollable_set(jump_to_entry, EINA_TRUE);
   elm_entry_single_line_set(jump_to_entry, EINA_TRUE);
   elm_object_part_text_set(jump_to_entry, "guide", "Jump To Pointer");
   efl_event_callback_add(jump_to_entry, ELM_ENTRY_EVENT_ACTIVATED, jump_entry_changed, jump2ptr_inwin);
   evas_object_show(jump_to_entry);
   elm_win_inwin_content_set(jump2ptr_inwin, jump_to_entry);

   elm_toolbar_item_append(tb, "edit-find", "Jump To Pointer", _jump_to_ptr_inwin_show, jump2ptr_inwin);

   /*
   extensions_bt = elm_button_add(tb);
   evas_object_size_hint_weight_set(extensions_bt, 1.000000, 1.000000);
   evas_object_size_hint_align_set(extensions_bt, -1.00000, -1.000000);
   efl_gfx_visible_set(extensions_bt, EINA_TRUE);
   elm_obj_widget_part_text_set(extensions_bt, NULL, "Extensions");
   elm_box_pack_end(tb, extensions_bt);
   */

     {
        int i;
        Eo *settings_it = elm_toolbar_item_append(tb, "system-run", "Settings", NULL, NULL);
        elm_toolbar_item_menu_set(settings_it, EINA_TRUE);
        elm_toolbar_item_priority_set(settings_it, -9999);

        Eo *settings_menu = elm_toolbar_item_menu_get(settings_it);
        Eo *objs_type_it = elm_menu_item_add(settings_menu, NULL, NULL,
              "Objects types display", NULL, NULL);
        while (objs_types_strings[i])
          {
             Eo *it = elm_menu_item_add(settings_menu, objs_type_it, NULL,
                   objs_types_strings[i], objs_type_changed, (void *)(uintptr_t)i);
             Eo *rd = elm_radio_add(settings_menu);
             elm_radio_state_value_set(rd, i);
             if (!i) pub_widgets->objs_type_radio = rd;
             else elm_radio_group_add(rd, pub_widgets->objs_type_radio);
             elm_object_item_content_set(it, rd);
             i++;
          }
        Eo *highlight_it = elm_menu_item_add(settings_menu, NULL, NULL,
              "Hightlight", highlight_changed, NULL);
        Eo *ck = elm_check_add(settings_menu);
        elm_object_item_content_set(highlight_it, ck);
        pub_widgets->highlight_ck = ck;
     }

   pub_widgets->save_load_bt = elm_toolbar_item_append(tb, "document-export", "Save", save_load_perform, NULL);

   freeze_pulse = elm_progressbar_add(main_win);
   pub_widgets->freeze_pulse = freeze_pulse;
   elm_object_style_set(freeze_pulse, "wheel");
   elm_object_text_set(freeze_pulse, "Style: wheel");
   elm_progressbar_pulse_set(freeze_pulse, EINA_TRUE);
   elm_progressbar_pulse(freeze_pulse, EINA_FALSE);
   evas_object_size_hint_align_set(freeze_pulse, 0.5, 0.0);
   evas_object_size_hint_weight_set(freeze_pulse, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(main_win, freeze_pulse);

   freeze_inwin = elm_win_inwin_add(main_win);
   pub_widgets->freeze_inwin = freeze_inwin;
   elm_object_style_set(freeze_inwin, "minimal");

   elm_panes1 = efl_add(ELM_PANES_CLASS, elm_box1);
   elm_obj_panes_content_right_size_set(elm_panes1, 0.600000);
   evas_object_size_hint_weight_set(elm_panes1, 1.000000, 1.000000);
   efl_gfx_size_set(elm_panes1, 75, 75);
   efl_gfx_visible_set(elm_panes1, EINA_TRUE);
   evas_object_size_hint_weight_set(elm_panes1, 1.000000, 1.000000);
   evas_object_size_hint_align_set(elm_panes1, -1.000000, -1.000000);
   elm_box_pack_end(elm_box1, tb);
   elm_box_pack_end(elm_box1, elm_panes1);
   object_infos_list = elm_genlist_add(elm_panes1);
   pub_widgets->object_infos_list = object_infos_list;
   evas_object_size_hint_weight_set(object_infos_list, 1.000000, 1.000000);
   efl_gfx_visible_set(object_infos_list, EINA_TRUE);
   objects_list = efl_add(ELM_GENLIST_CLASS, elm_panes1);
   pub_widgets->objects_list = objects_list;
   evas_object_size_hint_weight_set(objects_list, 1.000000, 1.000000);
   efl_gfx_visible_set(objects_list, EINA_TRUE);
   elm_object_part_content_set(elm_panes1, "left", objects_list);
   elm_object_part_content_set(elm_panes1, "right", object_infos_list);
   efl_gfx_visible_set(main_win, EINA_TRUE);
   efl_event_callback_add(main_win, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   return pub_widgets;
}

Gui_New_Profile_Win_Widgets *
gui_new_profile_win_create(Eo *__main_parent)
{
   Gui_New_Profile_Win_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *new_profile_win;
   Eo *elm_bg4;
   Eo *elm_box5;
   Eo *elm_box4;
   Eo *new_profile_cancel_button;
   Eo *new_profile_save_button;
   Eo *elm_box6;
   Eo *elm_label3;
   Eo *new_profile_name;
   Eo *elm_label1;
   Eo *new_profile_command;
   Eo *elm_label2;
   Eo *new_profile_script;


   new_profile_win = elm_win_add(__main_parent, "Window", ELM_WIN_BASIC);
   pub_widgets->new_profile_win = new_profile_win;
   elm_win_autodel_set(new_profile_win, EINA_TRUE);
   evas_object_size_hint_weight_set(new_profile_win, 1.000000, 1.000000);
   efl_gfx_size_set(new_profile_win, 689, 390);
   elm_win_title_set(new_profile_win, "New profile...");
   elm_win_modal_set(new_profile_win, EINA_TRUE);
   elm_bg4 = efl_add(ELM_BG_CLASS, new_profile_win);
   evas_object_size_hint_weight_set(elm_bg4, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_bg4, EINA_TRUE);
   elm_box5 = elm_box_add(new_profile_win);
   elm_box_padding_set(elm_box5, 7, 0);
   evas_object_size_hint_weight_set(elm_box5, 1.000000, 1.000000);
   evas_object_size_hint_align_set(elm_box5, -1.000000, -1.000000);
   efl_gfx_size_set(elm_box5, 200, 200);
   efl_gfx_visible_set(elm_box5, EINA_TRUE);
   elm_win_resize_object_add(new_profile_win, elm_bg4);
   elm_win_resize_object_add(new_profile_win, elm_box5);
   elm_box4 = elm_box_add(elm_box5);
   elm_box_padding_set(elm_box4, 7, 0);
   evas_object_size_hint_weight_set(elm_box4, 1.000000, 1.000000);
   evas_object_size_hint_align_set(elm_box4, -1.000000, -1.000000);
   efl_gfx_size_set(elm_box4, 200, 200);
   efl_gfx_visible_set(elm_box4, EINA_TRUE);
   elm_box_horizontal_set(elm_box4, EINA_TRUE);
   new_profile_cancel_button = efl_add(ELM_BUTTON_CLASS, elm_box4);
   pub_widgets->new_profile_cancel_button = new_profile_cancel_button;
   evas_object_size_hint_weight_set(new_profile_cancel_button, 1.000000, 1.000000);
   efl_gfx_visible_set(new_profile_cancel_button, EINA_TRUE);
   efl_gfx_size_set(new_profile_cancel_button, 73, 30);
   elm_obj_widget_part_text_set(new_profile_cancel_button, NULL, "Cancel");
   new_profile_save_button = efl_add(ELM_BUTTON_CLASS, elm_box4);
   pub_widgets->new_profile_save_button = new_profile_save_button;
   evas_object_size_hint_weight_set(new_profile_save_button, 1.000000, 1.000000);
   efl_gfx_visible_set(new_profile_save_button, EINA_TRUE);
   efl_gfx_size_set(new_profile_save_button, 73, 30);
   elm_obj_widget_part_text_set(new_profile_save_button, NULL, "Save");
   efl_event_callback_add(new_profile_save_button, EFL_UI_EVENT_CLICKED, new_profile_save_cb, NULL);
   elm_box_pack_end(elm_box4, new_profile_save_button);
   elm_box_pack_end(elm_box4, new_profile_cancel_button);
   elm_box6 = elm_box_add(elm_box5);
   elm_box_padding_set(elm_box6, 7, 0);
   evas_object_size_hint_weight_set(elm_box6, 1.000000, 1.000000);
   evas_object_size_hint_align_set(elm_box6, -1.000000, -1.000000);
   efl_gfx_size_set(elm_box6, 200, 200);
   efl_gfx_visible_set(elm_box6, EINA_TRUE);
   elm_box_horizontal_set(elm_box6, EINA_TRUE);
   elm_label3 = efl_add(ELM_LABEL_CLASS, elm_box6);
   efl_gfx_visible_set(elm_label3, EINA_TRUE);
   efl_gfx_size_set(elm_label3, 60, 30);
   evas_object_size_hint_align_set(elm_label3, 1.000000, -1.000000);
   elm_obj_widget_part_text_set(elm_label3, NULL, "Name: ");
   evas_object_size_hint_weight_set(elm_label3, 0.000000, 1.000000);
   new_profile_name = efl_add(ELM_ENTRY_CLASS, elm_box6);
   pub_widgets->new_profile_name = new_profile_name;
   evas_object_size_hint_align_set(new_profile_name, -1.000000, -1.000000);
   efl_gfx_visible_set(new_profile_name, EINA_TRUE);
   efl_gfx_size_set(new_profile_name, 65, 35);
   elm_obj_entry_scrollable_set(new_profile_name, EINA_TRUE);
   elm_obj_entry_single_line_set(new_profile_name, EINA_TRUE);
   evas_object_size_hint_weight_set(new_profile_name, 4.000000, 1.000000);
   elm_obj_entry_editable_set(new_profile_name, EINA_TRUE);
   elm_obj_widget_part_text_set(new_profile_name, NULL, "");
   elm_box_pack_end(elm_box6, elm_label3);
   elm_box_pack_end(elm_box6, new_profile_name);
   elm_label1 = efl_add(ELM_LABEL_CLASS, elm_box5);
   evas_object_size_hint_weight_set(elm_label1, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_label1, EINA_TRUE);
   efl_gfx_size_set(elm_label1, 115, 30);
   efl_gfx_position_set(elm_label1, 847, 0);
   elm_obj_widget_part_text_set(elm_label1, NULL, "Command: ");
   evas_object_size_hint_align_set(elm_label1, 0.000000, 2.000000);
   new_profile_command = efl_add(ELM_ENTRY_CLASS, elm_box5);
   pub_widgets->new_profile_command = new_profile_command;
   elm_obj_entry_scrollable_set(new_profile_command, EINA_TRUE);
   evas_object_size_hint_align_set(new_profile_command, -1.000000, -1.000000);
   efl_gfx_visible_set(new_profile_command, EINA_TRUE);
   efl_gfx_size_set(new_profile_command, 65, 35);
   elm_obj_entry_single_line_set(new_profile_command, EINA_TRUE);
   evas_object_size_hint_weight_set(new_profile_command, 1.000000, 2.000000);
   elm_label2 = efl_add(ELM_LABEL_CLASS, elm_box5);
   evas_object_size_hint_weight_set(elm_label2, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_label2, EINA_TRUE);
   efl_gfx_size_set(elm_label2, 60, 30);
   evas_object_size_hint_align_set(elm_label2, 0.000000, 1.000000);
   elm_obj_widget_part_text_set(elm_label2, NULL, "Script: ");
   new_profile_script = efl_add(ELM_ENTRY_CLASS, elm_box5);
   pub_widgets->new_profile_script = new_profile_script;
   elm_obj_entry_scrollable_set(new_profile_script, EINA_TRUE);
   evas_object_size_hint_align_set(new_profile_script, -1.000000, -1.000000);
   efl_gfx_visible_set(new_profile_script, EINA_TRUE);
   efl_gfx_size_set(new_profile_script, 65, 35);
   evas_object_size_hint_weight_set(new_profile_script, 1.000000, 8.000000);
   elm_box_pack_end(elm_box5, elm_box6);
   elm_box_pack_end(elm_box5, elm_label1);
   elm_box_pack_end(elm_box5, new_profile_command);
   elm_box_pack_end(elm_box5, elm_label2);
   elm_box_pack_end(elm_box5, new_profile_script);
   elm_box_pack_end(elm_box5, elm_box4);
   efl_gfx_visible_set(new_profile_win, EINA_TRUE);
   efl_event_callback_add(new_profile_win, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   return pub_widgets;
}


Gui_Take_Screenshot_Button_Widgets *
gui_take_screenshot_button_create(Eo *__main_parent)
{
   Gui_Take_Screenshot_Button_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *bt;
   Eo *elm_icon1;

   bt = efl_add(ELM_BUTTON_CLASS, __main_parent);
   pub_widgets->bt = bt;
   evas_object_size_hint_weight_set(bt, 1.000000, 1.000000);
   efl_gfx_visible_set(bt, EINA_TRUE);
   efl_gfx_size_set(bt, 73, 30);
   efl_event_callback_add(bt, EFL_UI_EVENT_CLICKED, take_screenshot_button_clicked, NULL);

   elm_icon1 = elm_icon_add(bt);
   evas_object_size_hint_weight_set(elm_icon1, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_icon1, EINA_TRUE);
   efl_gfx_size_set(elm_icon1, 40, 40);
   efl_file_set(elm_icon1, TAKE_SCREENSHOT_ICON, NULL);
   elm_object_part_content_set(bt, "icon", elm_icon1);
   efl_event_callback_add(bt, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   return pub_widgets;
}

Gui_Show_Screenshot_Button_Widgets *
gui_show_screenshot_button_create(Eo *__main_parent)
{
   Gui_Show_Screenshot_Button_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *bt;
   Eo *elm_icon1;

   bt = efl_add(ELM_BUTTON_CLASS, __main_parent);
   pub_widgets->bt = bt;
   evas_object_size_hint_weight_set(bt, 1.000000, 1.000000);
   efl_gfx_visible_set(bt, EINA_TRUE);
   efl_gfx_size_set(bt, 73, 30);
   efl_event_callback_add(bt, EFL_UI_EVENT_CLICKED, show_screenshot_button_clicked, NULL);

   elm_icon1 = elm_icon_add(bt);
   evas_object_size_hint_weight_set(elm_icon1, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_icon1, EINA_TRUE);
   efl_gfx_size_set(elm_icon1, 40, 40);
   efl_file_set(elm_icon1, SHOW_SCREENSHOT_ICON, NULL);
   elm_object_part_content_set(bt, "icon", elm_icon1);
   efl_event_callback_add(bt, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   return pub_widgets;
}

Gui_Show_Screenshot_Win_Widgets *
gui_show_screenshot_win_create(Eo *__main_parent)
{
   Gui_Show_Screenshot_Win_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *win;
   Eo *bg;

   win = elm_win_add(__main_parent, "Screenshot", ELM_WIN_BASIC);
   pub_widgets->win = win;
   elm_win_autodel_set(win, EINA_TRUE);
   efl_gfx_size_set(win, 300, 300);
   evas_object_size_hint_weight_set(win, 1.000000, 1.000000);
   elm_win_title_set(win, "Screenshot");
   elm_win_modal_set(win, EINA_TRUE);
   bg = efl_add(ELM_BG_CLASS, win);
   pub_widgets->bg = bg;
   evas_object_size_hint_weight_set(bg, 1.000000, 1.000000);
   efl_gfx_visible_set(bg, EINA_TRUE);
   elm_win_resize_object_add(win, bg);
   efl_gfx_visible_set(win, EINA_TRUE);
   efl_event_callback_add(win, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   return pub_widgets;
}

Gui_Widgets *gui_gui_get()
{
   static Eina_Bool initialized = EINA_FALSE;
   if (!initialized)
     {
        g_pub_widgets.main_win = gui_main_win_create(NULL);
        initialized = EINA_TRUE;
     }
   return &g_pub_widgets;
}
