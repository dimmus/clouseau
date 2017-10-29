#include <Elementary.h>
#include "../../Clouseau_Debug.h"
#include "gui.h"

static Evas_Object *table, *managers, *redirect, *history, *scroller;
static Elm_Genlist_Item_Class *itc;

static char*
_text_get(void *data, Elm_Genlist *list, const char *part)
{
   Efl_Ui_Focus_Manager *manager = data;
   Eina_Strbuf *res = eina_strbuf_new();

   eina_strbuf_append_printf(res, "%p", manager);
   return eina_strbuf_release(res);
}

EAPI Evas_Object*
ui_create(Instance *inst, Evas_Object *obj)
{
   Evas_Object *o;

   o = table = elm_table_add(obj);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(o);

   o = managers = elm_combobox_add(obj);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_text_set(o, "guide", "Manager to inspect");
   evas_object_show(o);
   elm_table_pack(table, o, 0, 0, 1, 1);
   itc = elm_genlist_item_class_new();
   itc->func.text_get = _text_get;

   o = elm_label_add(obj);
   elm_object_text_set(o, "Redirect:");
   evas_object_show(o);
   elm_table_pack(table, o, 1, 0, 1, 1);

   o = redirect = elm_label_add(obj);
   evas_object_show(o);
   elm_table_pack(table, o, 2, 0, 1, 1);

   o = history = elm_hoversel_add(obj);
   elm_object_text_set(o, "History");
   evas_object_show(o);
   elm_table_pack(table, o, 3, 0, 1, 1);

   o = scroller = elm_scroller_add(table);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(o);
   elm_table_pack(table, o, 0, 1, 4, 1);

   return table;
}

static void
_sel(void *data, Evas_Object *obj, void *event_info)
{
   com_defailt_manager(data, elm_object_item_data_get(event_info));
}

EAPI void
ui_managers_add(Instance *inst, Efl_Ui_Focus_Manager **manager, int size)
{
   elm_genlist_clear(managers);

   for (int i = 0; i < size; ++i)
     {
        elm_genlist_item_append(managers, itc, manager[i], NULL, 0, _sel, inst);
     }
}

static int
_sort(const void *a_raw, const void *b_raw)
{
   const Clouseau_Focus_Relation *a = a_raw;
   const Clouseau_Focus_Relation *b = b_raw;

   int val_a, val_b;

   if (a) val_a = a->relation.position_in_history;
   if (b) val_b = b->relation.position_in_history;

   return val_a - val_b;
}

EAPI void
ui_manager_data_arrived(Instance *inst, Clouseau_Focus_Manager_Data *data)
{
   Clouseau_Focus_Relation *rel;
   Evas_Object *box, *o;
   Eina_List *n, *sorted = NULL;

   EINA_LIST_FREE(inst->realized.objects, o)
      evas_object_del(o);

   inst->realized.data = data;

   elm_hoversel_clear(history);

   EINA_LIST_FOREACH(data->relations, n, rel)
     {
        if (rel->relation.position_in_history != -1)
          sorted = eina_list_sorted_insert(sorted, _sort, rel);
     }

   EINA_LIST_FOREACH(sorted, n, rel)
     {
        elm_hoversel_item_add(history, rel->class_name, NULL, 0, NULL, NULL);
     }

   tree_view_update(inst, scroller);

}
