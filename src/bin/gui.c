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

static Eina_Bool
_pubs_free_cb(void *data, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info EINA_UNUSED)
{
   free(data);
   return EINA_TRUE;
}

Gui_Elm_Win1_Widgets *
gui_elm_win1_create(Eo *__main_parent)
{
   Gui_Elm_Win1_Widgets *pub_widgets = calloc(1, sizeof(*pub_widgets));

   Eo *elm_win1;
   Eo *elm_bg1;
   Eo *elm_box1;
   Eo *elm_hoversel1;
   Eo *elm_panes1;
   Eo *elm_genlist1;


   elm_win1 = eo_add(ELM_WIN_CLASS, __main_parent, elm_obj_win_type_set(ELM_WIN_BASIC));
   pub_widgets->elm_win1 = elm_win1;
   eo_do(elm_win1, elm_obj_widget_part_text_set(NULL, "Window"));
   eo_do(elm_win1, elm_obj_win_autodel_set(EINA_TRUE));
   eo_do(elm_win1, efl_gfx_size_set(478, 484));
   elm_bg1 = eo_add(ELM_BG_CLASS, elm_win1);
   eo_do(elm_bg1, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_bg1, efl_gfx_position_set(0, 0));
   eo_do(elm_win1, elm_obj_win_resize_object_add(elm_bg1));
   elm_box1 = eo_add(ELM_BOX_CLASS, elm_bg1);
   pub_widgets->elm_box1 = elm_box1;
   eo_do(elm_box1, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_box1, efl_gfx_size_set(643, 598));
   eo_do(elm_box1, efl_gfx_position_set(-7, -2));
   eo_do(elm_box1, elm_obj_box_padding_set(0, 0));
   eo_do(elm_box1, elm_obj_box_align_set(0.000000, 0.000000));
   eo_do(elm_bg1, elm_obj_container_content_set(NULL, elm_box1));
   elm_hoversel1 = eo_add(ELM_HOVERSEL_CLASS, elm_box1);
   pub_widgets->elm_hoversel1 = elm_hoversel1;
   eo_do(elm_hoversel1, elm_obj_widget_part_text_set(NULL, "Hoversel"));
   eo_do(elm_hoversel1, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_hoversel1, efl_gfx_size_set(1174, 643));
   eo_do(elm_hoversel1, efl_gfx_position_set(-8, -2));
   eo_do(elm_hoversel1, elm_obj_hoversel_horizontal_set(EINA_FALSE));
   eo_do(elm_hoversel1, elm_obj_hoversel_auto_update_set(EINA_TRUE));
   elm_panes1 = eo_add(ELM_PANES_CLASS, elm_box1);
   pub_widgets->elm_panes1 = elm_panes1;
   eo_do(elm_panes1, elm_obj_panes_content_right_size_set(0.600000));
   eo_do(elm_panes1, efl_gfx_size_set(75, 75));
   eo_do(elm_panes1, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_panes1, evas_obj_size_hint_weight_set(1.000000, 1.000000));
   eo_do(elm_panes1, evas_obj_size_hint_align_set(-1.000000, -1.000000));
   eo_do(elm_box1, elm_obj_box_pack_end(elm_hoversel1));
   eo_do(elm_box1, elm_obj_box_pack_end(elm_panes1));
   elm_genlist1 = eo_add(ELM_GENLIST_CLASS, elm_panes1);
   pub_widgets->elm_genlist1 = elm_genlist1;
   eo_do(elm_genlist1, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_panes1, elm_obj_container_content_set("left", elm_genlist1));
   eo_do(elm_panes1, elm_obj_container_content_set("right", NULL));
   eo_do(elm_win1, efl_gfx_visible_set(EINA_TRUE));
   eo_do(elm_win1, eo_event_callback_add(EO_BASE_EVENT_DEL, _pubs_free_cb, pub_widgets));

   return pub_widgets;
}

Gui_Widgets *gui_gui_get()
{
   static Eina_Bool initialized = EINA_FALSE;
   if (!initialized)
     {
        g_pub_widgets.elm_win1 = gui_elm_win1_create(NULL);
        initialized = EINA_TRUE;
     }
   return &g_pub_widgets;
}