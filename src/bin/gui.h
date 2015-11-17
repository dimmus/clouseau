#ifndef _gui_h_
#define _gui_h_
#include <Eo.h>
#include <Elementary.h>

typedef struct
{
   Eo *elm_win1;
   Eo *elm_box1;
   Eo *elm_hoversel1;
   Eo *elm_panes1;
   Eo *elm_genlist2;
   Eo *elm_genlist1;
} Gui_Elm_Win1_Widgets;


typedef struct
{
   Eo *profiles_win;
   Eo *profile_ok_button;
   Eo *profile_cancel_button;
   Eo *profiles_list;
} Gui_Profiles_Win_Widgets;


typedef struct {
     Gui_Profiles_Win_Widgets *profiles_win;
} Gui_Widgets;

Gui_Elm_Win1_Widgets *gui_elm_win1_create(Eo *parent);

Gui_Profiles_Win_Widgets *gui_profiles_win_create(Eo *parent);

Gui_Widgets *gui_gui_get();
#endif
