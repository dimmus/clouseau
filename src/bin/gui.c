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

static Gui_Widgets g_pub_widgets;

extern void gui_new_profile_win_create_done(Gui_New_Profile_Win_Widgets *wdgs);

#ifdef GUI_IMAGES_PATH
  const char *SCREENSHOT_ICON = GUI_IMAGES_PATH"/show-screenshot.png";
#else
   #error "Please define GUI_IMAGES_PATH"
#endif

extern void
_profile_win_close_cb(void *data, const Efl_Event *event);
extern void
_new_profile_save_cb(void *data, const Efl_Event *event);
extern void
_new_profile_cancel_cb(void *data, const Efl_Event *event);
extern void
_profile_del_cb(void *data, const Efl_Event *event);
extern void
screenshot_req_cb(void *data, const Efl_Event *event);

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
   Eo *elm_bg1;
   Eo *elm_box1;
   Eo *apps_selector;
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
   apps_selector = elm_hoversel_add(elm_box1);
   pub_widgets->apps_selector = apps_selector;
   evas_object_size_hint_weight_set(apps_selector, 1.000000, 0.000000);
   evas_object_size_hint_align_set(apps_selector, 0.500000, 0.000000);
   efl_gfx_visible_set(apps_selector, EINA_TRUE);
   efl_gfx_size_set(apps_selector, 1174, 643);
   efl_gfx_position_set(apps_selector, -8, -2);
   elm_obj_widget_part_text_set(apps_selector, NULL, "Select App");
   elm_panes1 = efl_add(ELM_PANES_CLASS, elm_box1);
   elm_obj_panes_content_right_size_set(elm_panes1, 0.600000);
   evas_object_size_hint_weight_set(elm_panes1, 1.000000, 1.000000);
   efl_gfx_size_set(elm_panes1, 75, 75);
   efl_gfx_visible_set(elm_panes1, EINA_TRUE);
   evas_object_size_hint_weight_set(elm_panes1, 1.000000, 1.000000);
   evas_object_size_hint_align_set(elm_panes1, -1.000000, -1.000000);
   elm_box_pack_end(elm_box1, apps_selector);
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

static void
profile_delete_button_clicked(void *data, const Efl_Event *event)
{
   _profile_del_cb(data, event);
}

static void
profile_new_button_clicked(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   Gui_New_Profile_Win_Widgets *wdgs = gui_new_profile_win_create(NULL);
   gui_new_profile_win_create_done(wdgs);
}

static void
profile_ok_button_clicked(void *data, const Efl_Event *event)
{
   _profile_win_close_cb(data, event);
}

Gui_Profiles_Win_Widgets *
gui_profiles_win_create(Eo *__main_parent)
{
   Gui_Profiles_Win_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *profiles_win;
   Eo *elm_bg2;
   Eo *elm_box2;
   Eo *elm_box3;
   Eo *profile_delete_button;
   Eo *profile_ok_button;
   Eo *profile_cancel_button;
   Eo *profile_new_button;
   Eo *profiles_list;


   profiles_win = elm_win_add(__main_parent, "Win", ELM_WIN_BASIC);
   pub_widgets->profiles_win = profiles_win;
   elm_win_autodel_set(profiles_win, EINA_TRUE);
   elm_widget_part_text_set(profiles_win, NULL, "Window");
   efl_gfx_size_set(profiles_win, 347, 362);
   evas_object_size_hint_weight_set(profiles_win, 1.000000, 1.000000);
   evas_object_freeze_events_set(profiles_win, EINA_FALSE);
   evas_object_repeat_events_set(profiles_win, EINA_FALSE);
   elm_win_title_set(profiles_win, "Profiles");
   elm_bg2 = elm_bg_add(profiles_win);
   evas_object_size_hint_weight_set(elm_bg2, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_bg2, EINA_TRUE);
   elm_box2 = elm_box_add(profiles_win);
   elm_box_padding_set(elm_box2, 7, 0);
   evas_object_size_hint_weight_set(elm_box2, 1.000000, 1.000000);
   efl_gfx_size_set(elm_box2, 200, 200);
   efl_gfx_visible_set(elm_box2, EINA_TRUE);
   elm_win_resize_object_add(profiles_win, elm_bg2);
   elm_win_resize_object_add(profiles_win, elm_box2);
   elm_box3 = elm_box_add(elm_box2);
   elm_box_padding_set(elm_box3, 7, 0);
   evas_object_size_hint_align_set(elm_box3, -1.000000, -1.000000);
   efl_gfx_visible_set(elm_box3, EINA_TRUE);
   elm_box_horizontal_set(elm_box3, EINA_TRUE);
   efl_gfx_size_set(elm_box3, 200, 139);
   efl_gfx_position_set(elm_box3, 289, 742);
   evas_object_size_hint_weight_set(elm_box3, 1.000000, 0.200000);
   profile_delete_button = efl_add(ELM_BUTTON_CLASS, elm_box3);
   pub_widgets->profile_delete_button = profile_delete_button;
   efl_gfx_visible_set(profile_delete_button, EINA_TRUE);
   elm_obj_widget_disabled_set(profile_delete_button, EINA_TRUE);
   elm_obj_widget_part_text_set(profile_delete_button, NULL, "Delete profile");
   efl_gfx_size_set(profile_delete_button, 115, 30);
   efl_gfx_position_set(profile_delete_button, -42, 0);
   evas_object_size_hint_align_set(profile_delete_button, 0.500000, 0.500000);
   evas_object_size_hint_weight_set(profile_delete_button, 1.000000, 1.000000);
   efl_event_callback_add(profile_delete_button, EFL_UI_EVENT_CLICKED, profile_delete_button_clicked, NULL);
   profile_ok_button = efl_add(ELM_BUTTON_CLASS, elm_box3);
   pub_widgets->profile_ok_button = profile_ok_button;
   evas_object_size_hint_weight_set(profile_ok_button, 1.000000, 1.000000);
   efl_gfx_visible_set(profile_ok_button, EINA_TRUE);
   efl_gfx_size_set(profile_ok_button, 73, 30);
   elm_obj_widget_part_text_set(profile_ok_button, NULL, "Ok");
   elm_obj_widget_disabled_set(profile_ok_button, EINA_TRUE);
   efl_event_callback_add(profile_ok_button, EFL_UI_EVENT_CLICKED, profile_ok_button_clicked, NULL);
   profile_cancel_button = efl_add(ELM_BUTTON_CLASS, elm_box3);
   pub_widgets->profile_cancel_button = profile_cancel_button;
   evas_object_size_hint_weight_set(profile_cancel_button, 1.000000, 1.000000);
   efl_gfx_visible_set(profile_cancel_button, EINA_TRUE);
   efl_gfx_size_set(profile_cancel_button, 73, 30);
   elm_obj_widget_part_text_set(profile_cancel_button, NULL, "Cancel");
   profile_new_button = efl_add(ELM_BUTTON_CLASS, elm_box3);
   efl_gfx_visible_set(profile_new_button, EINA_TRUE);
   efl_gfx_size_set(profile_new_button, 73, 30);
   elm_obj_widget_part_text_set(profile_new_button, NULL, "New profile");
   evas_object_size_hint_weight_set(profile_new_button, 1.000000, 1.000000);
   evas_object_size_hint_align_set(profile_new_button, 0.500000, 0.500000);
   efl_event_callback_add(profile_new_button, EFL_UI_EVENT_CLICKED, profile_new_button_clicked, NULL);
   elm_box_pack_end(elm_box3, profile_ok_button);
   elm_box_pack_end(elm_box3, profile_cancel_button);
   elm_box_pack_end(elm_box3, profile_new_button);
   elm_box_pack_end(elm_box3, profile_delete_button);
   profiles_list = efl_add(ELM_GENLIST_CLASS, elm_box2);
   pub_widgets->profiles_list = profiles_list;
   evas_object_size_hint_weight_set(profiles_list, 1.000000, 1.000000);
   efl_gfx_visible_set(profiles_list, EINA_TRUE);
   evas_object_size_hint_align_set(profiles_list, -1.000000, -1.000000);
   elm_box_pack_end(elm_box2, profiles_list);
   elm_box_pack_end(elm_box2, elm_box3);
   efl_gfx_visible_set(profiles_win, EINA_TRUE);
   efl_event_callback_add(profiles_win, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   return pub_widgets;
}

static void
new_profile_save_button_clicked(void *data, const Efl_Event *event)
{
   _new_profile_save_cb(data, event);
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
   Eo *new_profile_type_selector;
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
   efl_event_callback_add(new_profile_save_button, EFL_UI_EVENT_CLICKED, new_profile_save_button_clicked, NULL);
   elm_box_pack_end(elm_box4, new_profile_save_button);
   elm_box_pack_end(elm_box4, new_profile_cancel_button);
   elm_box6 = elm_box_add(elm_box5);
   elm_box_padding_set(elm_box6, 7, 0);
   evas_object_size_hint_weight_set(elm_box6, 1.000000, 1.000000);
   evas_object_size_hint_align_set(elm_box6, -1.000000, -1.000000);
   efl_gfx_size_set(elm_box6, 200, 200);
   efl_gfx_visible_set(elm_box6, EINA_TRUE);
   elm_box_horizontal_set(elm_box6, EINA_TRUE);
   new_profile_type_selector = elm_hoversel_add(elm_box6);
   pub_widgets->new_profile_type_selector = new_profile_type_selector;
   evas_object_size_hint_weight_set(new_profile_type_selector, 1.000000, 1.000000);
   efl_gfx_visible_set(new_profile_type_selector, EINA_TRUE);
   efl_gfx_size_set(new_profile_type_selector, 60, 40);
   evas_object_size_hint_align_set(new_profile_type_selector, 0.000000, 0.500000);
   elm_obj_widget_part_text_set(new_profile_type_selector, NULL, "Choose the profile type");
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
   elm_box_pack_end(elm_box6, new_profile_type_selector);
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
   elm_obj_widget_disabled_set(new_profile_command, EINA_TRUE);
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
   elm_obj_widget_disabled_set(new_profile_script, EINA_TRUE);
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

static void
screenshot_button_clicked(void *data, const Efl_Event *event)
{
   screenshot_req_cb(data, event);
}

Gui_Screenshot_Button_Widgets *
gui_screenshot_button_create(Eo *__main_parent)
{
   Gui_Screenshot_Button_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *screenshot_button;
   Eo *elm_icon1;


   screenshot_button = efl_add(ELM_BUTTON_CLASS, __main_parent);
   pub_widgets->screenshot_button = screenshot_button;
   evas_object_size_hint_weight_set(screenshot_button, 1.000000, 1.000000);
   efl_gfx_visible_set(screenshot_button, EINA_TRUE);
   efl_gfx_size_set(screenshot_button, 73, 30);
   efl_event_callback_add(screenshot_button, EFL_UI_EVENT_CLICKED, screenshot_button_clicked, NULL);
   elm_icon1 = elm_icon_add(screenshot_button);
   evas_object_size_hint_weight_set(elm_icon1, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_icon1, EINA_TRUE);
   efl_gfx_size_set(elm_icon1, 40, 40);
   efl_file_set(elm_icon1, SCREENSHOT_ICON, NULL);
   elm_object_part_content_set(screenshot_button, "icon", elm_icon1);
   efl_event_callback_add(screenshot_button, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

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
