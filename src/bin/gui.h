#ifndef _gui_h_
#define _gui_h_
#include <Eo.h>
#include <Elementary.h>

typedef struct
{
   Eo *main_win;
   Eo *ext_box;
   Eo *conn_selector;
   Eo *conn_selector_menu;
   Eo *apps_selector;
   Eo *apps_selector_menu;
   Eo *save_load_bt;
   Eo *ext_selector_menu;
   Eo *freeze_pulse;
   Eo *freeze_inwin;
} Gui_Main_Win_Widgets;

typedef struct
{
   Eo *inwin;
   Eo *cancel_button;
   Eo *save_button;
   Eo *name_entry;
   Eo *port_entry;
} Gui_Remote_Port_Win_Widgets;

Gui_Main_Win_Widgets *gui_main_win_create(Eo *parent);

Gui_Remote_Port_Win_Widgets *gui_remote_port_win_create(Eo *parent);

#endif
