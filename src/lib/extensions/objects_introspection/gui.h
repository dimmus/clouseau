#ifndef _gui_h_
#define _gui_h_
#include <Eo.h>
#include <Elementary.h>

typedef struct
{
   Eo *main;
   Eo *tb;
   Eo *reload_button;
   Eo *objs_type_radio;
   Eo *highlight_ck;
   Eo *object_infos_list;
   Eo *objects_list;
} Gui_Win_Widgets;

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

Gui_Win_Widgets *gui_win_create(Eo *parent);

Gui_Take_Screenshot_Button_Widgets *gui_take_screenshot_button_create(Eo *parent);
Gui_Show_Screenshot_Button_Widgets *gui_show_screenshot_button_create(Eo *parent);
Gui_Show_Screenshot_Win_Widgets *gui_show_screenshot_win_create(Eo *parent);

#endif

