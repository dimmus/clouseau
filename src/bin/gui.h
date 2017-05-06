#ifndef _gui_h_
#define _gui_h_
#include <Eo.h>
#include <Elementary.h>

typedef struct
{
   Eo *main_win;
   Eo *tb;
   Eo *conn_selector;
   Eo *conn_selector_menu;
   Eo *reload_button;
   Eo *apps_selector;
   Eo *apps_selector_menu;
   Eo *objs_type_radio;
   Eo *highlight_ck;
   Eo *save_load_bt;
   Eo *object_infos_list;
   Eo *objects_list;
   Eo *freeze_pulse;
   Eo *freeze_inwin;
} Gui_Main_Win_Widgets;

typedef struct
{
   Eo *inwin;
   Eo *cancel_button;
   Eo *save_button;
   Eo *name_entry;
   Eo *command_entry;
} Gui_New_Profile_Win_Widgets;

typedef struct
{
   Eo *bt;
} Gui_Take_Screenshot_Button_Widgets;

typedef struct
{
   Eo *bt;
} Gui_Show_Screenshot_Button_Widgets;

typedef struct
{
   Eo *win;
   Eo *bg;
} Gui_Show_Screenshot_Win_Widgets;

typedef struct
{
   Gui_Main_Win_Widgets *main_win;
} Gui_Widgets;

Gui_Main_Win_Widgets *gui_main_win_create(Eo *parent);

Gui_New_Profile_Win_Widgets *gui_new_profile_win_create(Eo *parent);

Gui_Take_Screenshot_Button_Widgets *gui_take_screenshot_button_create(Eo *parent);
Gui_Show_Screenshot_Button_Widgets *gui_show_screenshot_button_create(Eo *parent);
Gui_Show_Screenshot_Win_Widgets *gui_show_screenshot_win_create(Eo *parent);

Gui_Widgets *gui_gui_get();
#endif
