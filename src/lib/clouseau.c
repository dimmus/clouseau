#include "clouseau_private.h"

/* Highlight functions. */
static Eina_Bool
_clouseau_highlight_fade(void *_rect)
{
   Evas_Object *rect = _rect;
   int r, g, b, a;
   double na;

   evas_object_color_get(rect, &r, &g, &b, &a);
   if (a < 20)
     {
        evas_object_del(rect);
        /* The del callback of the object will destroy the animator */
        return EINA_TRUE;
     }

   na = a - 5;
   r = na / a * r;
   g = na / a * g;
   b = na / a * b;
   evas_object_color_set(rect, r, g, b, na);

   return EINA_TRUE;
}

static Evas_Object *
_clouseau_verify_e_children(Evas_Object *obj, Evas_Object *ptr)
{
   /* Verify that obj still exists (can be found on evas canvas) */
   Evas_Object *child;
   Evas_Object *p = NULL;
   Eina_List *children;

   if (ptr == obj)
     return ptr;

   children = evas_object_smart_members_get(obj);
   EINA_LIST_FREE(children, child)
     {
        p = _clouseau_verify_e_children(child, ptr);
        if (p) break;
     }
   eina_list_free(children);

   return p;
}

static Evas_Object *
_clouseau_verify_e_obj(Evas_Object *obj)
{
   /* Verify that obj still exists (can be found on evas canvas) */
   Evas_Object *o;
   Eina_List *ees;
   Ecore_Evas *ee;
   Evas_Object *rt = NULL;

   ees = ecore_evas_ecore_evas_list_get();
   EINA_LIST_FREE(ees, ee)
     {
        Evas *e;
        Eina_List *objs;

        e = ecore_evas_get(ee);
        objs = evas_objects_in_rectangle_get(e, SHRT_MIN, SHRT_MIN,
                                             USHRT_MAX, USHRT_MAX,
                                             EINA_TRUE, EINA_TRUE);

        EINA_LIST_FREE(objs, o)
          {
             rt = _clouseau_verify_e_children(o, obj);
             if (rt) break;
          }
        eina_list_free(objs);

        if (rt) break;
     }
   eina_list_free(ees);

   return rt;
}

static void
_clouseau_highlight_del(void *data,
                        EINA_UNUSED Evas *e,
                        EINA_UNUSED Evas_Object *obj,
                        EINA_UNUSED void *event_info)
{  /* Delete timer for this rect */
   ecore_animator_del(data);
}

EAPI void
clouseau_data_object_highlight(Evas_Object *obj, Clouseau_Evas_Props *props, bmp_info_st *view)
{
   Ecore_Animator *t;
   Evas_Object *r;
   int x, y, wd, ht;
   Evas *e = NULL;

   if (props)
     {
        /* When working offline grab info from struct */
        Evas_Coord x_bmp, y_bmp;

        evas_object_geometry_get(view->o, &x_bmp, &y_bmp, NULL, NULL);
        x =  (view->zoom_val * props->x) + x_bmp;
        y =  (view->zoom_val * props->y) + y_bmp;
        wd = (view->zoom_val * props->w);
        ht = (view->zoom_val * props->h);

        e = evas_object_evas_get(view->win);
     }
   else
     {
        /* Check validity of object when working online */
        if (!_clouseau_verify_e_obj(obj))
          {
             printf("<%s> Evas Object not found <%p> (probably deleted)\n",
                   __func__, obj);
             return;
          }

        evas_object_geometry_get(obj, &x, &y, &wd, &ht);

        /* Take evas from object if working online */
        e = evas_object_evas_get(obj);
        if (!e) return;
     }

   /* Continue and do the Highlight */
   r = evas_object_rectangle_add(e);
   evas_object_move(r, x - PADDING, y - PADDING);
   evas_object_resize(r, wd + (2 * PADDING), ht + (2 * PADDING));
   evas_object_color_set(r,
                         HIGHLIGHT_R, HIGHLIGHT_G, HIGHLIGHT_B, HIGHLIGHT_A);
   evas_object_show(r);

   /* Add Timer for fade and a callback to delete timer on obj del */
   t = ecore_animator_add(_clouseau_highlight_fade, r);
   evas_object_event_callback_add(r, EVAS_CALLBACK_DEL,
                                  _clouseau_highlight_del, t);
/* Print backtrace info, saved for future ref
   tmp = evas_object_data_get(obj, ".clouseau.bt");
   fprintf(stderr, "Creation backtrace :\n%s*******\n", tmp); */
}
