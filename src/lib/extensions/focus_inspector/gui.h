#ifndef GUI_H
#define GUI_H

#include "../../Clouseau.h"
#include "../../Clouseau_Debug.h"

typedef struct {
   Clouseau_Extension *ext;
   struct {
      Eina_Hash *focusable_to_cfr;
      Eina_List *objects;
      Clouseau_Focus_Manager_Data *data;
   } realized;
} Instance;

typedef enum {
   RELATION_TREE = 0,
   RELATION_NEXT = 1,
   RELATION_PREV = 2,
/*   RELATION_RIGHT = 3,
   RELATION_LEFT = 4,
   RELATION_TOP = 5,
   RELATION_DOWN = 6*/
} Relations;

#define PUSH_CLEANUP(inst, o) inst->realized.objects = eina_list_append(inst->realized.objects, o)

EAPI void tree_view_update(Instance *inst, Evas_Object *scroller);
EAPI void tree_view_relation_display(Instance *inst, Relations rel);

EAPI void ui_managers_add(Instance *inst, Efl_Ui_Focus_Manager **manager, int size);
EAPI void ui_manager_data_arrived(Instance *inst, Clouseau_Focus_Manager_Data *data);
EAPI Evas_Object* ui_create(Instance *inst, Evas_Object *obj);

EAPI void com_refresh_managers(Instance *inst);
EAPI void com_defailt_manager(Instance *inst, Efl_Ui_Focus_Manager *manager);
#endif
