#include <Elementary.h>
#include "../../Clouseau_Debug.h"
#include "gui.h"

static void
find(Instance *pd, void *parent, void (*found)(void *data, Instance *pd, Clouseau_Focus_Relation *relation), void *data)
{
   for (int i = 0; pd->realized.data->relations && i < eina_list_count(pd->realized.data->relations); ++i)
     {
        Clouseau_Focus_Relation *rel;
        rel = eina_list_nth(pd->realized.data->relations, i);
        if (rel->relation.parent == parent)
          found(data, pd, rel);
     }
}

void
tree_level(void *data, Instance *inst, Clouseau_Focus_Relation *relation)
{
   Evas_Object *box, *childbox, *vis;
   int maxw, maxh, minw, minh;
   Eina_Strbuf *buf;
   char group[PATH_MAX];

   box = evas_object_box_add(evas_object_evas_get(data));
   evas_object_box_padding_set(box, 20, 20);
   evas_object_show(box);

   PUSH_CLEANUP(inst, box);

   buf = eina_strbuf_new();
   eina_strbuf_append_printf(buf, "%p", relation->relation.node);

   vis = edje_object_add(evas_object_evas_get(data));
   evas_object_size_hint_weight_set(vis, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(vis, 0.0, EVAS_HINT_FILL);

   if (relation->relation.position_in_history != -1 )
     snprintf(group, sizeof(group), "focus_inspector/history");
   else if (relation->relation.redirect)
     snprintf(group, sizeof(group), "focus_inspector/redirect");
   else if (relation->relation.logical)
     snprintf(group, sizeof(group), "focus_inspector/logical");
   else
     snprintf(group, sizeof(group), "focus_inspector/regular");

   edje_object_file_set(vis, FOCUS_EDJ, group);
   edje_object_part_text_set(vis, "widget_name", relation->class_name);
   evas_object_show(vis);
   evas_object_box_append(box, vis);

   edje_object_size_max_get(vis, &maxw, &maxh);
   edje_object_size_min_get(vis, &minw, &minh);
   if ((minw <= 0) && (minh <= 0))
     edje_object_size_min_calc(vis, &minw, &minh);

   evas_object_size_hint_max_set(vis, maxw, maxh);
   evas_object_size_hint_min_set(vis, minw, minh);

   PUSH_CLEANUP(inst, vis);

   childbox = evas_object_box_add(evas_object_evas_get(data));
   evas_object_box_padding_set(childbox, 20, 20);
   evas_object_box_layout_set(childbox, evas_object_box_layout_vertical, NULL, NULL);
   evas_object_box_append(box, childbox);
   evas_object_show(childbox);

   PUSH_CLEANUP(inst, childbox);

   find(inst, relation->relation.node, tree_level, childbox);

   evas_object_box_append(data, box);
}

EAPI void
tree_view_update(Instance *inst, Evas_Object *scroller)
{
   Evas_Object *box;

   box = evas_object_box_add(evas_object_evas_get(scroller));
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_box_layout_set(box, evas_object_box_layout_vertical, NULL, NULL);
   evas_object_show(box);
   PUSH_CLEANUP(inst, box);

   if (inst->realized.data)
     find(inst, NULL, tree_level, box);

   elm_object_content_set(scroller, box);
}
