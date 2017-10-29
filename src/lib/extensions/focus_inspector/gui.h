#ifndef GUI_H
#define GUI_H

#include "../../Clouseau.h"
#include "../../Clouseau_Debug.h"

typedef struct {
   Clouseau_Extension *ext;
   struct {
      Eina_List *objects;
      Clouseau_Focus_Manager_Data *data;
   } realized;
} Instance;

#define PUSH_CLEANUP(inst, o) inst->realized.objects = eina_list_append(inst->realized.objects, o)

EAPI void tree_view_update(Instance *inst, Evas_Object *scroller);

EAPI void ui_managers_add(Instance *inst, Efl_Ui_Focus_Manager **manager, int size);
EAPI void ui_manager_data_arrived(Instance *inst, Clouseau_Focus_Manager_Data *data);
EAPI Evas_Object* ui_create(Instance *inst, Evas_Object *obj);

EAPI void com_refresh_managers(Instance *inst);
EAPI void com_defailt_manager(Instance *inst, Efl_Ui_Focus_Manager *manager);
#endif
