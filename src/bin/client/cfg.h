#ifndef _CFG_H
#define _CFG_H

#include <Elementary.h>

#define _CLOUSEAU_CFG_VERSION 1

void clouseau_cfg_init(const char *file);
void clouseau_cfg_shutdown(void);

Eina_Bool clouseau_cfg_save(void);
Eina_Bool clouseau_cfg_load(void);

typedef struct
{
   unsigned int version;

   Eina_Bool show_hidden;
   Eina_Bool show_clippers;
   Eina_Bool show_elm_only;
} Clouseau_Cfg;

extern Clouseau_Cfg *_clouseau_cfg;

#endif
