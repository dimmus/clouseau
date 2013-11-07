#ifndef _CONFIG_DIALOG_H
#define _CONFIG_DIALOG_H

typedef void (*Clouseau_Config_Changed_Cb)(void *data);
void clouseau_settings_dialog_open(Evas_Object *parent, Clouseau_Config_Changed_Cb callback, const void *callback_data);

#endif

