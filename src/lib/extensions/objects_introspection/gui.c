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

static const char* objs_types_strings[] =
{
   "Show all canvas objects",
   "Only show Elementary widgets",
   NULL
};

#ifdef GUI_IMAGES_PATH
  const char *SHOW_SCREENSHOT_ICON = GUI_IMAGES_PATH"/show-screenshot.png";
  const char *TAKE_SCREENSHOT_ICON = GUI_IMAGES_PATH"/take-screenshot.png";
#else
   #error "Please define GUI_IMAGES_PATH"
#endif

extern void
screenshot_req_cb(void *data, const Efl_Event *event);
extern void
reload_perform(void *data, Evas_Object *obj, void *event_info);
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
extern void
jump_to_ptr_inwin_show(void *data, Evas_Object *obj, void *event_info);

static void
_pubs_free_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   free(data);
}

Gui_Win_Widgets *
gui_win_create(Eo *__main_parent)
{
   Gui_Win_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *box, *tb;
   Eo *panes;
   Eo *object_infos_list;
   Eo *objects_list;

   box = elm_box_add(__main_parent);
   pub_widgets->main = box;
   evas_object_size_hint_weight_set(box, 1, 1);
   evas_object_size_hint_align_set(box, -1, -1);
   efl_gfx_visible_set(box, EINA_TRUE);
   efl_event_callback_add(box, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   tb = elm_toolbar_add(__main_parent);
   pub_widgets->tb = tb;
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_MENU);
   elm_toolbar_menu_parent_set(tb, __main_parent);
   evas_object_size_hint_weight_set(tb, 0, 0);
   evas_object_size_hint_align_set(tb, -1, 0);
   efl_gfx_visible_set(tb, EINA_TRUE);

   pub_widgets->reload_button = elm_toolbar_item_append(tb, "view-refresh", "Reload", reload_perform, NULL);

   elm_toolbar_item_append(tb, "edit-find", "Jump To Pointer", jump_to_ptr_inwin_show, NULL);

     {
        int i = 0;
        Eo *settings_it = elm_toolbar_item_append(tb, "system-run", "Settings", NULL, NULL);
        elm_toolbar_item_menu_set(settings_it, EINA_TRUE);

        Eo *settings_menu = elm_toolbar_item_menu_get(settings_it);
        pub_widgets->settings_menu = settings_menu;
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

   panes = efl_add(ELM_PANES_CLASS, box);
   elm_obj_panes_content_right_size_set(panes, 0.600000);
   evas_object_size_hint_weight_set(panes, 1.000000, 1.000000);
   efl_gfx_size_set(panes, 75, 75);
   efl_gfx_visible_set(panes, EINA_TRUE);
   evas_object_size_hint_weight_set(panes, 1.000000, 1.000000);
   evas_object_size_hint_align_set(panes, -1.000000, -1.000000);
   elm_box_pack_end(box, tb);
   elm_box_pack_end(box, panes);
   object_infos_list = elm_genlist_add(panes);
   pub_widgets->object_infos_list = object_infos_list;
   evas_object_size_hint_weight_set(object_infos_list, 1.000000, 1.000000);
   efl_gfx_visible_set(object_infos_list, EINA_TRUE);
   objects_list = efl_add(ELM_GENLIST_CLASS, panes);
   pub_widgets->objects_list = objects_list;
   evas_object_size_hint_weight_set(objects_list, 1.000000, 1.000000);
   efl_gfx_visible_set(objects_list, EINA_TRUE);
   elm_object_part_content_set(panes, "left", objects_list);
   elm_object_part_content_set(panes, "right", object_infos_list);

   return pub_widgets;
}

Gui_Take_Screenshot_Button_Widgets *
gui_take_screenshot_button_create(Eo *__main_parent)
{
   Gui_Take_Screenshot_Button_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *bt;
   Eo *elm_icon1;

   bt = efl_add(EFL_UI_BUTTON_CLASS, __main_parent);
   pub_widgets->bt = bt;
   evas_object_size_hint_weight_set(bt, 1.000000, 1.000000);
   efl_gfx_visible_set(bt, EINA_TRUE);
   efl_event_callback_add(bt, EFL_UI_EVENT_CLICKED, take_screenshot_button_clicked, NULL);

   elm_icon1 = elm_icon_add(bt);
   evas_object_size_hint_weight_set(elm_icon1, 1.000000, 1.000000);
   efl_gfx_visible_set(elm_icon1, EINA_TRUE);
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

   bt = efl_add(EFL_UI_BUTTON_CLASS, __main_parent);
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
   bg = efl_add(ELM_BG_CLASS, win);
   pub_widgets->bg = bg;
   evas_object_size_hint_weight_set(bg, 1.000000, 1.000000);
   efl_gfx_visible_set(bg, EINA_TRUE);
   elm_win_resize_object_add(win, bg);
   efl_gfx_visible_set(win, EINA_TRUE);
   efl_event_callback_add(win, EFL_EVENT_DEL, _pubs_free_cb, pub_widgets);

   return pub_widgets;
}
