#ifndef _gui_h_
#define _gui_h_
#include <Eo.h>
#include <Elementary.h>

typedef struct
{
   Eo *main_win;
   Eo *apps_selector;
   Eo *object_infos_list;
   Eo *objects_list;
} Gui_Main_Win_Widgets;


typedef struct
{
   Eo *profiles_win;
   Eo *profile_delete_button;
   Eo *profile_ok_button;
   Eo *profile_cancel_button;
   Eo *profiles_list;
} Gui_Profiles_Win_Widgets;


typedef struct
{
   Eo *new_profile_win;
   Eo *new_profile_cancel_button;
   Eo *new_profile_save_button;
   Eo *new_profile_type_selector;
   Eo *new_profile_name;
   Eo *new_profile_command;
   Eo *new_profile_script;
} Gui_New_Profile_Win_Widgets;


typedef struct
{
   Eo *screenshot_button;
} Gui_Screenshot_Button_Widgets;


typedef struct
{
   Eo *screenshot_win;
   Eo *img;
} Gui_Screenshot_Win_Widgets;


typedef struct {
     Gui_Main_Win_Widgets *main_win;
} Gui_Widgets;

Gui_Main_Win_Widgets *gui_main_win_create(Eo *parent);

Gui_Profiles_Win_Widgets *gui_profiles_win_create(Eo *parent);

Gui_New_Profile_Win_Widgets *gui_new_profile_win_create(Eo *parent);

Gui_Screenshot_Button_Widgets *gui_screenshot_button_create(Eo *parent);

Gui_Screenshot_Win_Widgets *gui_screenshot_win_create(Eo *parent);

Gui_Widgets *gui_gui_get();
#endif
