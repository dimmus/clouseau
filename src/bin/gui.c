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

extern void gui_main_win_create_done(Gui_Main_Win_Widgets *wdgs);

extern Eina_Bool
_profile_win_close_cb(void *data, Eo *obj, const Eo_Event_Description *desc, void *event_info);

static Eina_Bool
_pubs_free_cb(void *data, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info EINA_UNUSED)
{
   free(data);
   return EINA_TRUE;
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


   main_win = eo_add(ELM_WIN_CLASS, __main_parent, elm_obj_win_type_set(ELM_WIN_BASIC));
   pub_widgets->main_win = main_win;
   eo_do(main_win, elm_obj_widget_part_text_set(NULL, "Window"));
   eo_do(main_win, elm_obj_win_autodel_set(EINA_TRUE));
   eo_do(main_win, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(main_win, efl_gfx_size_set(478, 484));
   elm_bg1 = eo_add(ELM_BG_CLASS, main_win);
   eo_do(elm_bg1, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(elm_bg1, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_bg1, efl_gfx_position_set(0, 0));
   elm_box1 = eo_add(ELM_BOX_CLASS, main_win);
   eo_do(elm_box1, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(elm_box1, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_box1, efl_gfx_size_set(643, 598));
   eo_do(elm_box1, efl_gfx_position_set(-7, -2));
   eo_do(elm_box1, elm_obj_box_padding_set(0, 0));
   eo_do(elm_box1, elm_obj_box_align_set(0.000000, 0.000000));
   eo_do(main_win, elm_obj_win_resize_object_add(elm_bg1));
   eo_do(main_win, elm_obj_win_resize_object_add(elm_box1));
   apps_selector = eo_add(ELM_HOVERSEL_CLASS, elm_box1);
   pub_widgets->apps_selector = apps_selector;
   eo_do(apps_selector, evas_obj_size_hint_weight_set(1.000000, 0.000000));
   eo_do(apps_selector, evas_obj_size_hint_align_set(0.500000, 0.000000));
   eo_do(apps_selector, efl_gfx_visible_set(EINA_TRUE));
   eo_do(apps_selector, efl_gfx_size_set(1174, 643));
   eo_do(apps_selector, efl_gfx_position_set(-8, -2));
   eo_do(apps_selector, elm_obj_hoversel_horizontal_set(EINA_FALSE));
   eo_do(apps_selector, elm_obj_hoversel_auto_update_set(EINA_TRUE));
   eo_do(apps_selector, elm_obj_widget_part_text_set(NULL, "Select App"));
   elm_panes1 = eo_add(ELM_PANES_CLASS, elm_box1);
   eo_do(elm_panes1, elm_obj_panes_content_right_size_set(0.600000));
   eo_do(elm_panes1, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(elm_panes1, efl_gfx_size_set(75, 75));
   eo_do(elm_panes1, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_panes1, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(elm_panes1, evas_obj_size_hint_align_set(-1.000000, -1.000000));
   eo_do(elm_box1, elm_obj_box_pack_end(apps_selector));
   eo_do(elm_box1, elm_obj_box_pack_end(elm_panes1));
   object_infos_list = eo_add(ELM_GENLIST_CLASS, elm_panes1);
   pub_widgets->object_infos_list = object_infos_list;
   eo_do(object_infos_list, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(object_infos_list, efl_gfx_visible_set(EINA_TRUE));
   objects_list = eo_add(ELM_GENLIST_CLASS, elm_panes1);
   pub_widgets->objects_list = objects_list;
   eo_do(objects_list, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(objects_list, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_panes1, elm_obj_container_content_set("left", objects_list));
   eo_do(elm_panes1, elm_obj_container_content_set("right", object_infos_list));
   eo_do(main_win, efl_gfx_visible_set(EINA_TRUE));
   eo_do(main_win, eo_event_callback_add(EO_BASE_EVENT_DEL, _pubs_free_cb, pub_widgets));

   return pub_widgets;
}

static Eina_Bool
profile_ok_button_clicked(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Gui_Main_Win_Widgets *wdgs = gui_main_win_create(NULL);
   gui_main_win_create_done(wdgs);
   _profile_win_close_cb(data, obj, desc, event_info);
   return EINA_TRUE;
}

Gui_Profiles_Win_Widgets *
gui_profiles_win_create(Eo *__main_parent)
{
   Gui_Profiles_Win_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *profiles_win;
   Eo *elm_menu1;
   Eo *elm_bg2;
   Eo *elm_box2;
   Eo *elm_box3;
   Eo *profile_ok_button;
   Eo *profile_cancel_button;
   Eo *profiles_list;


   profiles_win = eo_add(ELM_WIN_CLASS, __main_parent, elm_obj_win_name_set("Win"),
elm_obj_win_type_set(ELM_WIN_BASIC));
   pub_widgets->profiles_win = profiles_win;
   eo_do(profiles_win, elm_obj_win_autodel_set(EINA_TRUE));
   eo_do(profiles_win, elm_obj_widget_part_text_set(NULL, "Window"));
   eo_do(profiles_win, efl_gfx_size_set(347, 362));
   eo_do(profiles_win, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(profiles_win, evas_obj_freeze_events_set(EINA_FALSE));
   eo_do(profiles_win, evas_obj_repeat_events_set(EINA_FALSE));
   eo_do(profiles_win, elm_obj_win_title_set("Profiles"));
   elm_menu1 = eo_add(ELM_MENU_CLASS, profiles_win);
   elm_bg2 = eo_add(ELM_BG_CLASS, profiles_win);
   eo_do(elm_bg2, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(elm_bg2, efl_gfx_visible_set(EINA_TRUE));
   elm_box2 = eo_add(ELM_BOX_CLASS, profiles_win);
   eo_do(elm_box2, elm_obj_box_padding_set(7, 0));
   eo_do(elm_box2, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(elm_box2, efl_gfx_size_set(200, 200));
   eo_do(elm_box2, efl_gfx_visible_set(EINA_TRUE));
   eo_do(profiles_win, elm_obj_win_resize_object_add(elm_bg2));
   eo_do(profiles_win, elm_obj_win_resize_object_add(elm_box2));
   elm_box3 = eo_add(ELM_BOX_CLASS, elm_box2);
   eo_do(elm_box3, elm_obj_box_padding_set(7, 0));
   eo_do(elm_box3, evas_obj_size_hint_align_set(-1.000000, -1.000000));
   eo_do(elm_box3, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_box3, elm_obj_box_horizontal_set(EINA_TRUE));
   eo_do(elm_box3, efl_gfx_size_set(200, 139));
   eo_do(elm_box3, efl_gfx_position_set(289, 742));
   eo_do(elm_box3, evas_obj_size_hint_weight_set(1.000000, 0.200000));
   profile_ok_button = eo_add(ELM_BUTTON_CLASS, elm_box3);
   pub_widgets->profile_ok_button = profile_ok_button;
   eo_do(profile_ok_button, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(profile_ok_button, efl_gfx_visible_set(EINA_TRUE));
   eo_do(profile_ok_button, efl_gfx_size_set(73, 30));
   eo_do(profile_ok_button, elm_obj_widget_part_text_set(NULL, "Ok"));
   eo_do(profile_ok_button, elm_obj_widget_disabled_set(EINA_TRUE));
   eo_do(profile_ok_button, eo_event_callback_add(EVAS_CLICKABLE_INTERFACE_EVENT_CLICKED, profile_ok_button_clicked, NULL));
   profile_cancel_button = eo_add(ELM_BUTTON_CLASS, elm_box3);
   pub_widgets->profile_cancel_button = profile_cancel_button;
   eo_do(profile_cancel_button, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(profile_cancel_button, efl_gfx_visible_set(EINA_TRUE));
   eo_do(profile_cancel_button, efl_gfx_size_set(73, 30));
   eo_do(profile_cancel_button, elm_obj_widget_part_text_set(NULL, "Cancel"));
   eo_do(elm_box3, elm_obj_box_pack_end(profile_ok_button));
   eo_do(elm_box3, elm_obj_box_pack_end(profile_cancel_button));
   profiles_list = eo_add(ELM_GENLIST_CLASS, elm_box2);
   pub_widgets->profiles_list = profiles_list;
   eo_do(profiles_list, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(profiles_list, efl_gfx_visible_set(EINA_TRUE));
   eo_do(profiles_list, evas_obj_size_hint_align_set(-1.000000, -1.000000));
   eo_do(elm_box2, elm_obj_box_pack_end(profiles_list));
   eo_do(elm_box2, elm_obj_box_pack_end(elm_box3));
   eo_do(profiles_win, efl_gfx_visible_set(EINA_TRUE));
   eo_do(profiles_win, eo_event_callback_add(EO_BASE_EVENT_DEL, _pubs_free_cb, pub_widgets));

   return pub_widgets;
}

Gui_Widgets *gui_gui_get()
{
   static Eina_Bool initialized = EINA_FALSE;
   if (!initialized)
     {
        g_pub_widgets.profiles_win = gui_profiles_win_create(NULL);
        initialized = EINA_TRUE;
     }
   return &g_pub_widgets;
}