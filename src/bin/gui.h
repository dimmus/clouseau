#ifndef _gui_h_
#define _gui_h_
#include <Eo.h>
#include <Elementary.h>

static const char* objs_types_strings[] =
{
   "Show all canvas objects",
   "Only show Elementary widgets"
};

typedef struct
{
   Eo *main_win;
   Eo *bar_box;
   Eo *conn_selector;
   Eo *conn_selector_menu;
   Eo *load_button;
   Eo *apps_selector;
   Eo *save_bt;
   Eo *object_infos_list;
   Eo *objects_list;
} Gui_Main_Win_Widgets;

typedef struct
{
   Eo *new_profile_win;
   Eo *new_profile_cancel_button;
   Eo *new_profile_save_button;
   Eo *new_profile_name;
   Eo *new_profile_command;
   Eo *new_profile_script;
} Gui_New_Profile_Win_Widgets;

typedef struct
{
   Eo *win;
   Eo *ok_button;
   Eo *cancel_button;
   Eo *objs_types_sel;
   Eo *highlight_ck;
} Gui_Config_Win_Widgets;

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

Gui_Config_Win_Widgets *gui_config_win_create(Eo *__main_parent);

Gui_Widgets *gui_gui_get();
#endif
