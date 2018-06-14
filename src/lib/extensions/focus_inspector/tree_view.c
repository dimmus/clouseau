#include <Elementary.h>
#include "../../Clouseau_Debug.h"
#include "gui.h"

static void
find(Instance *pd, void *parent, void (*found)(void *data, Instance *pd, Clouseau_Focus_Relation *relation), void *data)
{
   for (unsigned int i = 0; pd->realized.data->relations && i < eina_list_count(pd->realized.data->relations); ++i)
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

   tree_view_relation_display(inst, RELATION_NONE);
}

static Evas_Object*
_create_arrow(Evas *e)
{
   Evas_Object *vg;
   Efl_VG *cont, *tail, *front;

   vg = evas_object_vg_add(e);

   cont = evas_vg_container_add(vg);

   tail = evas_vg_shape_add(cont);
   evas_vg_node_color_set(tail, 0, 0, 0, 255);
   evas_vg_shape_stroke_color_set(tail, 128, 10,10, 128);
   evas_vg_shape_stroke_width_set(tail, 2.0);
   evas_vg_shape_stroke_join_set(tail, EFL_GFX_JOIN_MITER);
   evas_vg_shape_append_move_to(tail, 0, 0);
   evas_vg_shape_append_line_to(tail, -100, 0);
   efl_name_set(tail, "tail");

   front = evas_vg_shape_add(cont);
   evas_vg_node_color_set(front, 0, 0, 0, 255);
   evas_vg_shape_stroke_color_set(front, 128, 10,10, 128);
   evas_vg_shape_stroke_width_set(front, 2.0);
   evas_vg_shape_stroke_join_set(front, EFL_GFX_JOIN_MITER);
   evas_vg_shape_append_move_to(front, -6, -6);
   evas_vg_shape_append_line_to(front, 0, 0);
   evas_vg_shape_append_line_to(front, -6, 6);
   evas_vg_shape_append_line_to(front, -6, -6);
   efl_name_set(front, "front");

   evas_object_vg_root_node_set(vg, cont);

   return vg;
}

static void
_arrow_from_to(Evas_Object *vg, Eina_Position2D from, Eina_Position2D to)
{
   Eina_Rect pos;
   Eina_Matrix3 tmp, root_m;
   Efl_VG *shape;
   double distance, deg;

   shape = evas_object_vg_root_node_get(vg);

   EINA_SAFETY_ON_NULL_RETURN(shape);

   pos.x = MIN(from.x, to.x);
   pos.y = MIN(from.y, to.y);
   pos.w = MAX(from.x, to.x) - pos.x;
   pos.h = MAX(from.y, to.y) - pos.y;

   distance = sqrt(pow(pos.w, 2)+pow(pos.h, 2));

   eina_matrix3_identity(&tmp);
   eina_matrix3_scale(&tmp, distance/100, 1.0);

   evas_vg_node_transformation_set(evas_vg_container_child_get(shape, "tail"), &tmp);

   Eina_Size2D size = EINA_SIZE2D((from.x - to.x), (from.y - to.y));

   if (from.y - to.y == 0)
     {
        deg = 0;
     }
   else if (from.x - to.x == 0)
     {
        if (from.y > to.y)
          deg = M_PI_2;
        else
          deg = M_PI + M_PI_2;
     }
   else
     {
        double di = ((double)(double)from.y - to.y) / ((double)from.x - to.x);
        deg = atan(di);
     }

   if (from.x >= to.x)
     {
        size.w = 0;
        deg += M_PI;
     }

   if (to.y <= from.y)
     {
       size.h = 0;
     }


   size.h = MAX(abs(size.h), 0) + 10;
   size.w = MAX(abs(size.w), 0) + 10;

   eina_matrix3_identity(&root_m);
   eina_matrix3_identity(&tmp);
   eina_matrix3_translate(&tmp, size.w, size.h);
   eina_matrix3_multiply_copy(&root_m, &root_m, &tmp);
   eina_matrix3_identity(&tmp);
   eina_matrix3_rotate(&tmp, deg);
   eina_matrix3_multiply_copy(&root_m, &root_m, &tmp);

   evas_vg_node_transformation_set(shape, &root_m);

   evas_object_geometry_set(vg, pos.x - 10, pos.y - 10, pos.w + 20, pos.h + 20);
}

static void
_geom_change(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eina_Rectangle pos1, pos2;
   Eina_Position2D from, to;
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

   _arrow_from_to(line, from, to);
}

static void
_relation_display(Instance *inst, Evas_Object *vis1, Evas_Object *vis2)
{
   Evas_Object *line;
   Evas_Object *smart;

   line = _create_arrow(evas_object_evas_get(vis1));
   //evas_object_anti_alias_set(line, EINA_FALSE);
   evas_object_pass_events_set(line, EINA_TRUE);
   evas_object_data_set(line, "__from", vis1);
   evas_object_data_set(line, "__to", vis2);
   evas_object_show(line);

   smart = evas_object_smart_parent_get(vis1);
   evas_object_smart_member_add(line, smart);

   _geom_change(line, evas_object_evas_get(vis1), vis1, NULL);

   PUSH_RELAION_CLEANUP(inst, line);

   evas_object_event_callback_add(vis1, EVAS_CALLBACK_MOVE, _geom_change, line);
   evas_object_event_callback_add(vis1, EVAS_CALLBACK_RESIZE, _geom_change, line);
   evas_object_event_callback_add(vis2, EVAS_CALLBACK_MOVE, _geom_change, line);
   evas_object_event_callback_add(vis2, EVAS_CALLBACK_RESIZE, _geom_change, line);

}

EAPI void
tree_view_relation_display(Instance *inst, Relations rel_type)
{
   Evas_Object *o;

   EINA_LIST_FREE(inst->realized.relation_objects, o)
     {
        Evas_Object *vis1, *vis2;
        vis1 = evas_object_data_get(o, "__from");
        vis2 = evas_object_data_get(o, "__to");

        evas_object_event_callback_del_full(vis1, EVAS_CALLBACK_MOVE, _geom_change, o);
        evas_object_event_callback_del_full(vis1, EVAS_CALLBACK_RESIZE, _geom_change, o);
        evas_object_event_callback_del_full(vis2, EVAS_CALLBACK_MOVE, _geom_change, o);
        evas_object_event_callback_del_full(vis2, EVAS_CALLBACK_RESIZE, _geom_change, o);

        evas_object_del(o);
     }

   if (rel_type == RELATION_NONE) return;

   if (!inst->realized.data) return;

   Clouseau_Focus_Relation *rel;
   Eina_List *n;

   EINA_LIST_FOREACH(inst->realized.data->relations, n, rel)
     {
        Eo *from;
        Eo *to;

        if (rel_type == RELATION_TREE)
          {
             if (!rel->relation.parent) continue;
             from = rel->relation.parent;
             to = rel->relation.node;
          }
        else if (rel_type == RELATION_NEXT)
          {
             if (!rel->relation.next) continue;
             to = rel->relation.next;
             from = rel->relation.node;
          }
        else if (rel_type == RELATION_PREV)
          {
             if (!rel->relation.prev) continue;
             to = rel->relation.prev;
             from = rel->relation.node;
          }

        Clouseau_Focus_Relation *c_from, *c_to;

        c_from = eina_hash_find(inst->realized.focusable_to_cfr, &from);
        c_to = eina_hash_find(inst->realized.focusable_to_cfr, &to);
        EINA_SAFETY_ON_NULL_GOTO(c_from, next);
        EINA_SAFETY_ON_NULL_GOTO(c_to, next);

        _relation_display(inst, c_from->vis, c_to->vis);
        next:
        (void) n;
      }
}
