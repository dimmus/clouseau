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

   relation->vis = vis;

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

   tree_view_relation_display(inst, RELATION_NEXT);
}

static void
_geom_change(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Eina_Rectangle pos1, pos2;
   Eina_Position2D calc_from, calc_to, from, to;
   Evas_Object *line, *vis1, *vis2;

   line = data;
   vis1 = evas_object_data_get(line, "__from");
   vis2 = evas_object_data_get(line, "__to");

   evas_object_geometry_get(vis1, &pos1.x, &pos1.y, &pos1.w, &pos1.h);
   evas_object_geometry_get(vis2, &pos2.x, &pos2.y, &pos2.w, &pos2.h);

   from.x = pos1.x + pos1.w / 2;
   from.y = pos1.y + pos1.h / 2;
   to.x = pos2.x + pos2.w / 2;
   to.y = pos2.y + pos2.h / 2;

   evas_object_line_xy_set(line, from.x, from.y, to.x, to.y);
}

static void
_relation_display(Instance *inst, Evas_Object *vis1, Evas_Object *vis2)
{
   Evas_Object *line;

   line = evas_object_line_add(evas_object_evas_get(vis1));
   evas_object_anti_alias_set(line, EINA_FALSE);
   evas_object_geometry_set(line, 0, 0, 1000, 1000);
   evas_object_pass_events_set(line, EINA_TRUE);
   evas_object_data_set(line, "__from", vis1);
   evas_object_data_set(line, "__to", vis2);
   evas_object_show(line);

   PUSH_CLEANUP(inst, line);

   evas_object_event_callback_add(vis1, EVAS_CALLBACK_MOVE, _geom_change, line);
   evas_object_event_callback_add(vis1, EVAS_CALLBACK_RESIZE, _geom_change, line);
   evas_object_event_callback_add(vis2, EVAS_CALLBACK_MOVE, _geom_change, line);
   evas_object_event_callback_add(vis2, EVAS_CALLBACK_RESIZE, _geom_change, line);
}

EAPI void
tree_view_relation_display(Instance *inst, Relations rel_type)
{
   if (rel_type == RELATION_TREE || rel_type == RELATION_NEXT || rel_type == RELATION_PREV)
     {
        Clouseau_Focus_Relation *rel;
        Eina_List *n;

        EINA_LIST_FOREACH(inst->realized.data->relations, n, rel)
          {
             Eo *relation_partner;

             if (rel_type == RELATION_TREE)
               {
                  relation_partner = rel->relation.parent;
                  if (!rel->relation.parent) continue;
               }
             else if (rel_type == RELATION_NEXT)
               {
                  relation_partner = rel->relation.next;
               }
             else if (rel_type == RELATION_PREV)
               {
                  relation_partner = rel->relation.prev;
               }

             Clouseau_Focus_Relation *c = eina_hash_find(inst->realized.focusable_to_cfr, &relation_partner);

             EINA_SAFETY_ON_NULL_GOTO(c, next);

             _relation_display(inst, c->vis, rel->vis);
             next:
             n = n;
          }
     }
}
