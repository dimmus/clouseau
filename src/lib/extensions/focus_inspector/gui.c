#include <Elementary.h>
#include "../../Clouseau_Debug.h"
#include "gui.h"

static Evas_Object *table, *managers, *redirect, *history, *scroller;
static Elm_Genlist_Item_Class *itc;

static char*
_text_get(void *data, Elm_Genlist *list EINA_UNUSED, const char *part EINA_UNUSED)
{
   Clouseau_Focus_List_Item *it = data;
   Eina_Strbuf *res = eina_strbuf_new();

   eina_strbuf_append_printf(res, "%s - %p", it->helper_name, (void*)it->ptr);
   return eina_strbuf_release(res);
}

static void
_sel_relation_func(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Instance *inst = evas_object_data_get(obj, "__instance");
   tree_view_relation_display(inst, elm_radio_state_value_get(obj));
   elm_radio_value_set(obj, elm_radio_state_value_get(obj));
}

EAPI Evas_Object*
ui_create(Instance *inst, Evas_Object *obj)
{
   Evas_Object *o, *table2, *group = NULL;

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

   o = table2 = elm_table_add(obj);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(o);

   char *text[] = {"Tree","Next","Prev","None"};

   for (int i = 0; i <= RELATION_NONE; ++i)
     {
        o = elm_radio_add(table);
        evas_object_data_set(o, "__instance", inst);
        evas_object_smart_callback_add(o, "changed", _sel_relation_func, NULL);
        evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_object_text_set(o, text[i]);
        evas_object_show(o);
        elm_table_pack(table2, o, i % 4, i/4, 1, 1);

        if (!group)
          group = o;
        else
          elm_radio_group_add(o, group);

        elm_radio_state_value_set(o, i);
     }

   elm_radio_value_set(group, RELATION_NONE);
   elm_table_pack(table, table2, 0, 2, 4, 1);

   return table;
}

static void
_sel(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Clouseau_Focus_List_Item *it = elm_object_item_data_get(event_info);
   com_defailt_manager(data, (void*)it->ptr);
}

EAPI void
ui_managers_add(Instance *inst, Clouseau_Focus_Managers *clouseau_managers)
{
   Clouseau_Focus_List_Item *it;
   Eina_List *n;

   elm_genlist_clear(managers);

   EINA_LIST_FOREACH(clouseau_managers->managers, n, it)
     {
        elm_genlist_item_append(managers, itc, it, NULL, 0, _sel, inst);
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
   Evas_Object *o;
   Eina_List *n, *sorted = NULL;

   EINA_LIST_FREE(inst->realized.objects, o)
      evas_object_del(o);

   inst->realized.focusable_to_cfr = eina_hash_pointer_new(NULL);

   inst->realized.data = data;

   elm_hoversel_clear(history);

   EINA_LIST_FOREACH(data->relations, n, rel)
     {
        if (rel->relation.position_in_history != -1)
          sorted = eina_list_sorted_insert(sorted, _sort, rel);
        eina_hash_add(inst->realized.focusable_to_cfr, &rel->relation.node, rel);
     }

   EINA_LIST_FOREACH(sorted, n, rel)
     {
        elm_hoversel_item_add(history, rel->class_name, NULL, 0, NULL, NULL);
     }

   tree_view_update(inst, scroller);

}
