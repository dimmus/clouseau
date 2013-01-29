#include "Clouseau.h"
#include "clouseau_private.h"
#define ELM_INTERNAL_API_ARGESFSDFEFC
#include <elm_widget.h>

static Evas_Object *prop_list = NULL;
static Elm_Genlist_Item_Class itc;

static void
_clouseau_object_dbg_string_build(Clouseau_Eo_Dbg_Info *eo,
      char *buf, int buf_size);

static void
_gl_selected(void *data EINA_UNUSED, Evas_Object *pobj EINA_UNUSED,
      void *event_info EINA_UNUSED)
{
   /* Currently do nothing */
   return;
}

static void
gl_exp(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   Eina_List *itr;

   Clouseau_Eo_Dbg_Info *eo = elm_object_item_data_get(glit);
   Clouseau_Eo_Dbg_Info *child;
   EINA_LIST_FOREACH(eo->un_dbg_info.dbg.list, itr, child)
     {
        Elm_Genlist_Item_Type iflag = (child->type == EO_DBG_INFO_TYPE_LIST) ?
           ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE;
        elm_genlist_item_append(prop_list, &itc, child, glit,
              iflag, _gl_selected, NULL);
     }
}

static void
gl_con(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_subitems_clear(glit);
}

static void
gl_exp_req(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);
}

static void
gl_con_req(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);
}

static Evas_Object *
item_icon_get(void *data EINA_UNUSED, Evas_Object *parent EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   return NULL;
}

static char *
item_text_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   Clouseau_Eo_Dbg_Info *eo = data;
   char buf[1024];
   _clouseau_object_dbg_string_build(eo, (char*)buf, 1024);
   return strdup(buf);
}

EAPI Evas_Object *
clouseau_object_information_list_add(Evas_Object *parent)
{
   prop_list = elm_genlist_add(parent);
   itc.item_style = "default";
   itc.func.text_get = item_text_get;
   itc.func.content_get = item_icon_get;
   itc.func.state_get = NULL;
   itc.func.del = NULL;

   evas_object_smart_callback_add(prop_list, "expand,request", gl_exp_req,
         prop_list);
   evas_object_smart_callback_add(prop_list, "contract,request", gl_con_req,
         prop_list);
   evas_object_smart_callback_add(prop_list, "expanded", gl_exp, prop_list);
   evas_object_smart_callback_add(prop_list, "contracted", gl_con, prop_list);
   evas_object_smart_callback_add(prop_list, "selected", _gl_selected, NULL);

   return prop_list;
}

EAPI void
clouseau_object_information_free(Clouseau_Object *oinfo)
{
   if (!oinfo)
     return;

   eina_stringshare_del(oinfo->evas_props.name);
   eina_stringshare_del(oinfo->evas_props.bt);

   if (oinfo->evas_props.points)
     free(oinfo->evas_props.points);

   switch (oinfo->extra_props.type)
     {
      case CLOUSEAU_OBJ_TYPE_ELM:
         eina_stringshare_del(oinfo->extra_props.u.elm.type);
         eina_stringshare_del(oinfo->extra_props.u.elm.style);
         break;
      case CLOUSEAU_OBJ_TYPE_TEXT:
         eina_stringshare_del(oinfo->extra_props.u.text.font);
         eina_stringshare_del(oinfo->extra_props.u.text.source);
         eina_stringshare_del(oinfo->extra_props.u.text.text);
         break;
      case CLOUSEAU_OBJ_TYPE_IMAGE:
         eina_stringshare_del(oinfo->extra_props.u.image.file);
         eina_stringshare_del(oinfo->extra_props.u.image.key);
         eina_stringshare_del(oinfo->extra_props.u.image.load_err);
         break;
      case CLOUSEAU_OBJ_TYPE_EDJE:
         eina_stringshare_del(oinfo->extra_props.u.edje.file);
         eina_stringshare_del(oinfo->extra_props.u.edje.group);

         eina_stringshare_del(oinfo->extra_props.u.edje.load_err);
         break;
      case CLOUSEAU_OBJ_TYPE_TEXTBLOCK:
         eina_stringshare_del(oinfo->extra_props.u.textblock.style);
         eina_stringshare_del(oinfo->extra_props.u.textblock.text);
         break;
      case CLOUSEAU_OBJ_TYPE_UNKNOWN:
      case CLOUSEAU_OBJ_TYPE_OTHER:
         break;
      default:
         break;
     }

   free(oinfo);
}

static Eina_List *
_clouseau_eo_list_convert(Eo_Dbg_Info *root)
{  /* This function converts a list of Eo_Dbg_Info
    * to a list of Clouseau_Eo_Dbg_Info.
    * This is required because we would like to keep the def of
    * Eo_Dbg_Info in EO code. Thus, avoiding API/ABI error if user
    * does not do a full update of Clouseau and EO                 */
   Eina_List *l;
   Eina_List *new_list = NULL;
   void *eo;

   if (!root && eo_dbg_type_get(root) != EO_DBG_INFO_TYPE_LIST)
     return new_list;
   Eina_List *root_list = eo_dbg_union_get(root)->list;

   EINA_LIST_FOREACH(root_list, l, eo)
     {
        Eo_Dbg_Info_Union *un = eo_dbg_union_get(eo);
        Clouseau_Eo_Dbg_Info *info = calloc(1, sizeof(*info));
        info->type = eo_dbg_type_get(eo);
        info->name = eo_dbg_name_get(eo);

        switch(info->type)
          {
           case EO_DBG_INFO_TYPE_STRING:
              info->un_dbg_info.text.s = un->text;
              break;

           case EO_DBG_INFO_TYPE_INT:
              info->un_dbg_info.intg.i = un->i;
              break;

           case EO_DBG_INFO_TYPE_BOOL:
              info->un_dbg_info.bl.b = un->b;
              break;

           case EO_DBG_INFO_TYPE_PTR:
              info->un_dbg_info.ptr.p = (unsigned long long) (unsigned long)
                 un->ptr;
              break;

           case EO_DBG_INFO_TYPE_DOUBLE:
              info->un_dbg_info.dbl.d = un->dbl;
              break;

           case EO_DBG_INFO_TYPE_LIST:
              info->un_dbg_info.dbg.list =
                 _clouseau_eo_list_convert(eo);
              break;

           default:  /* Unknown Type, keep zero */
              break;
          }

        new_list = eina_list_append(new_list, info);
     }

   return new_list;
}

EAPI Clouseau_Object *
clouseau_object_information_get(Clouseau_Tree_Item *treeit)
{
   Evas_Object *obj = (void*) (uintptr_t) treeit->ptr;
   Eo_Dbg_Info *eo_dbg_info = EO_DBG_INFO_LIST_APPEND(NULL, "");

   if (!treeit->is_obj)
     return NULL;

   eo_do(obj, eo_dbg_info_get(eo_dbg_info));
   treeit->eo_info = _clouseau_eo_list_convert(eo_dbg_info);

   eo_dbg_info_free(eo_dbg_info); /* Free original list */

   return NULL;
}

static const struct {
   const char *text;
   Evas_Object_Pointer_Mode mode;
} pointer_mode[3] = {
# define POINTER_MODE(Value) { #Value, Value }
  POINTER_MODE(EVAS_OBJECT_POINTER_MODE_AUTOGRAB),
  POINTER_MODE(EVAS_OBJECT_POINTER_MODE_NOGRAB),
  POINTER_MODE(EVAS_OBJECT_POINTER_MODE_NOGRAB_NO_REPEAT_UPDOWN)
# undef POINTER_MODE
};

static const struct {
   Clouseau_Object_Type type;
   const char *name;
} _clouseau_types[] = {
  { CLOUSEAU_OBJ_TYPE_ELM, "Elementary" },
  { CLOUSEAU_OBJ_TYPE_TEXT, "Text" },
  { CLOUSEAU_OBJ_TYPE_IMAGE, "Image" },
  { CLOUSEAU_OBJ_TYPE_EDJE, "Edje" },
  { CLOUSEAU_OBJ_TYPE_TEXTBLOCK, "Textblock" }
};

static void
_clouseau_object_dbg_string_build(Clouseau_Eo_Dbg_Info *eo,
      char *buf, int buf_size)
{  /* Build a string from dbg-info in buffer, or return empty buffer */
   int i;
   *buf = '\0';
   switch(eo->type)
     {
      case EO_DBG_INFO_TYPE_STRING:
           {  /* First set flags to say if got info from eo */
              snprintf(buf, buf_size, "%s",
                    eo->un_dbg_info.text.s);

              for(i = 0; buf[i]; i++)
                buf[i] = tolower(buf[i]);

              snprintf(buf, buf_size, "%s: %s",
                    eo->name, eo->un_dbg_info.text.s);
           }
         break;

      case EO_DBG_INFO_TYPE_INT:
           {
              snprintf(buf, buf_size, "%s: %d",
                    eo->name, eo->un_dbg_info.intg.i);
           }
         break;

      case EO_DBG_INFO_TYPE_BOOL:
         snprintf(buf, buf_size, "%s: %s",
               eo->name, (eo->un_dbg_info.bl.b) ?
               "TRUE" : "FALSE");
         break;

      case EO_DBG_INFO_TYPE_PTR:
         snprintf(buf, buf_size, "%s: %llx",
               eo->name, eo->un_dbg_info.ptr.p);
         break;

      case EO_DBG_INFO_TYPE_DOUBLE:
         snprintf(buf, buf_size, "%s: %.2f",
               eo->name, eo->un_dbg_info.dbl.d);
         break;

      case EO_DBG_INFO_TYPE_LIST:  /* Just copy class-name */
         snprintf(buf, buf_size, "%s", eo->name);
         break;

      default:
         break;
     }
}

EAPI void
clouseau_object_information_list_populate(Clouseau_Tree_Item *treeit, Evas_Object *lb)
{
   Clouseau_Object *oinfo;
   char buf[1024];
   unsigned int i;
   Eo_Dbg_Info *root = EO_DBG_INFO_LIST_APPEND(NULL, "");

   clouseau_object_information_list_clear();

   if (!treeit->is_obj)
      return;

   oinfo = treeit->info;

   /* This code is here only for backward compatibility and will be removed soon */
   if (!treeit->eo_info)
     {
        /* Populate evas properties list */
        Eo_Dbg_Info *group = EO_DBG_INFO_LIST_APPEND(root, "Evas");
        Eo_Dbg_Info *node;

        EO_DBG_INFO_BOOLEAN_APPEND(group, "Visibility", oinfo->evas_props.is_visible);
        EO_DBG_INFO_INTEGER_APPEND(group, "Layer", oinfo->evas_props.layer);

        node = EO_DBG_INFO_LIST_APPEND(group, "Position");
        EO_DBG_INFO_INTEGER_APPEND(node, "x", oinfo->evas_props.x);
        EO_DBG_INFO_INTEGER_APPEND(node, "y", oinfo->evas_props.y);

        node = EO_DBG_INFO_LIST_APPEND(group, "Size");
        EO_DBG_INFO_INTEGER_APPEND(node, "w", oinfo->evas_props.w);
        EO_DBG_INFO_INTEGER_APPEND(node, "h", oinfo->evas_props.h);

        EO_DBG_INFO_DOUBLE_APPEND(group, "Scale", oinfo->evas_props.scale);

#if 0
        if (evas_object_clip_get(obj))
          {
             evas_object_geometry_get(evas_object_clip_get(obj), &x, &y, &w, &h);
             _clouseau_information_geometry_to_tree(main_tit, "Clipper position", x, y);
             _clouseau_information_geometry_to_tree(main_tit, "Clipper size", w, h);
          }
#endif

        node = EO_DBG_INFO_LIST_APPEND(group, "Min size");
        EO_DBG_INFO_INTEGER_APPEND(node, "w", oinfo->evas_props.min_w);
        EO_DBG_INFO_INTEGER_APPEND(node, "h", oinfo->evas_props.min_h);
        node = EO_DBG_INFO_LIST_APPEND(group, "Max size");
        EO_DBG_INFO_INTEGER_APPEND(node, "w", oinfo->evas_props.max_w);
        EO_DBG_INFO_INTEGER_APPEND(node, "h", oinfo->evas_props.max_h);
        node = EO_DBG_INFO_LIST_APPEND(group, "Request size");
        EO_DBG_INFO_INTEGER_APPEND(node, "w", oinfo->evas_props.req_w);
        EO_DBG_INFO_INTEGER_APPEND(node, "h", oinfo->evas_props.req_h);
        node = EO_DBG_INFO_LIST_APPEND(group, "Align");
        EO_DBG_INFO_DOUBLE_APPEND(node, "w", oinfo->evas_props.align_x);
        EO_DBG_INFO_DOUBLE_APPEND(node, "h", oinfo->evas_props.align_y);
        node = EO_DBG_INFO_LIST_APPEND(group, "Weight");
        EO_DBG_INFO_DOUBLE_APPEND(node, "w", oinfo->evas_props.weight_x);
        EO_DBG_INFO_DOUBLE_APPEND(node, "h", oinfo->evas_props.weight_y);

#if 0
        evas_object_size_hint_aspect_get(obj, &w, &h);
        _clouseau_information_geometry_to_tree(main_tit, "Aspect", w, h);
#endif

        node = EO_DBG_INFO_LIST_APPEND(group, "Color");
        EO_DBG_INFO_INTEGER_APPEND(node, "r", oinfo->evas_props.r);
        EO_DBG_INFO_INTEGER_APPEND(node, "g", oinfo->evas_props.g);
        EO_DBG_INFO_INTEGER_APPEND(node, "b", oinfo->evas_props.b);
        EO_DBG_INFO_INTEGER_APPEND(node, "a", oinfo->evas_props.a);

        EO_DBG_INFO_BOOLEAN_APPEND(group, "Has focus", oinfo->evas_props.has_focus);

        for (i = 0; i < sizeof (pointer_mode) / sizeof (pointer_mode[0]); ++i)
           if (pointer_mode[i].mode == oinfo->evas_props.mode)
             {
                EO_DBG_INFO_TEXT_APPEND(group, "Pointer Mode", pointer_mode[i].text);
                break;
             }

        EO_DBG_INFO_BOOLEAN_APPEND(group, "Pass Events", oinfo->evas_props.pass_events);
        EO_DBG_INFO_BOOLEAN_APPEND(group, "Repeat Events", oinfo->evas_props.repeat_events);
        EO_DBG_INFO_BOOLEAN_APPEND(group, "Propagate Events", oinfo->evas_props.propagate_events);
        EO_DBG_INFO_BOOLEAN_APPEND(group, "Has clipees", oinfo->evas_props.is_clipper);
        if (oinfo->evas_props.clipper)
          {
             snprintf(buf, sizeof(buf), "%llx", oinfo->evas_props.clipper);
             EO_DBG_INFO_TEXT_APPEND(group, "Clipper", buf);
          }

        if (oinfo->evas_props.points_count)
          {
             node = EO_DBG_INFO_LIST_APPEND(group, "Evas Map");
             Clouseau_Evas_Map_Point_Props *p;
             for(i = 0 ; (int) i < oinfo->evas_props.points_count; i++)
               {
                  p = &oinfo->evas_props.points[i];

                  Eo_Dbg_Info *point = EO_DBG_INFO_LIST_APPEND(node, "Coords");
                  EO_DBG_INFO_INTEGER_APPEND(point, "x", p->x);
                  EO_DBG_INFO_INTEGER_APPEND(point, "y", p->y);
                  EO_DBG_INFO_INTEGER_APPEND(point, "z", p->z);
               }
          }

        if (oinfo->extra_props.type == CLOUSEAU_OBJ_TYPE_ELM)
          {
             group = EO_DBG_INFO_LIST_APPEND(root, "Elm");
             EO_DBG_INFO_TEXT_APPEND(group, "Wid-Type", oinfo->extra_props.u.elm.type);
#if 0
             /* Extract actual data from theme? */
             _clouseau_information_string_to_tree(main_tit, "Theme", elm_widget_theme_get(obj));
#endif
             EO_DBG_INFO_TEXT_APPEND(group, "Style", oinfo->extra_props.u.elm.style);
             EO_DBG_INFO_DOUBLE_APPEND(group, "Scale", oinfo->extra_props.u.elm.scale);
             EO_DBG_INFO_BOOLEAN_APPEND(group, "Disabled", oinfo->extra_props.u.elm.is_disabled);
             EO_DBG_INFO_BOOLEAN_APPEND(group, "Has focus", oinfo->extra_props.u.elm.has_focus);
             EO_DBG_INFO_BOOLEAN_APPEND(group, "Mirrored", oinfo->extra_props.u.elm.is_mirrored);
             EO_DBG_INFO_BOOLEAN_APPEND(group, "Automatic mirroring", oinfo->extra_props.u.elm.is_mirrored_automatic);
          }
        else if (oinfo->extra_props.type == CLOUSEAU_OBJ_TYPE_TEXT)
          {  /* EVAS_OBJ_TEXT_CLASS */
             group = EO_DBG_INFO_LIST_APPEND(root, "Text");
             EO_DBG_INFO_TEXT_APPEND(group, "Font", oinfo->extra_props.u.text.font);

             EO_DBG_INFO_INTEGER_APPEND(group, "Size", oinfo->extra_props.u.text.size);

             EO_DBG_INFO_TEXT_APPEND(group, "Source", oinfo->extra_props.u.text.source);
             EO_DBG_INFO_TEXT_APPEND(group, "Text", oinfo->extra_props.u.text.text);
          }
        else if (oinfo->extra_props.type == CLOUSEAU_OBJ_TYPE_IMAGE)
          {  /* EVAS_OBJ_IMAGE_CLASS */
             group = EO_DBG_INFO_LIST_APPEND(root, "Image");
             EO_DBG_INFO_TEXT_APPEND(group, "Filename", oinfo->extra_props.u.image.file);
             EO_DBG_INFO_TEXT_APPEND(group, "File key", oinfo->extra_props.u.image.key);
             EO_DBG_INFO_PTR_APPEND(group, "Source", oinfo->extra_props.u.image.source);

             if (oinfo->extra_props.u.image.load_err)
                EO_DBG_INFO_TEXT_APPEND(group, "Load error", oinfo->extra_props.u.image.load_err);
          }
        else if (oinfo->extra_props.type == CLOUSEAU_OBJ_TYPE_EDJE)
          {  /* EDJE_OBJ_CLASS */
             group = EO_DBG_INFO_LIST_APPEND(root, "Edje");
             EO_DBG_INFO_TEXT_APPEND(group, "File", oinfo->extra_props.u.edje.file);
             EO_DBG_INFO_TEXT_APPEND(group, "Group", oinfo->extra_props.u.edje.group);
             if (oinfo->extra_props.u.image.load_err)
                EO_DBG_INFO_TEXT_APPEND(group, "Load error", oinfo->extra_props.u.edje.load_err);
          }
        else if (oinfo->extra_props.type == CLOUSEAU_OBJ_TYPE_TEXTBLOCK)
          {  /* EVAS_OBJ_TEXTBLOCK_CLASS */
             group = EO_DBG_INFO_LIST_APPEND(root, "Text Block");
             EO_DBG_INFO_TEXT_APPEND(group, "Style", oinfo->extra_props.u.textblock.style);
             EO_DBG_INFO_TEXT_APPEND(group, "Text", oinfo->extra_props.u.textblock.text);
          }

        /* Update backtrace text */
        if (oinfo->evas_props.bt)
          {  /* Build backtrace label */
             char *k = malloc(strlen("Creation backtrace:\n\n") +
                   strlen(oinfo->evas_props.bt) + 1);

             sprintf(k, "Creation backtrace:\n\n%s", oinfo->evas_props.bt);
             char *p = elm_entry_utf8_to_markup(k);
             elm_object_text_set(lb, p);
             free(p);
             free(k);
          }
        else
           elm_object_text_set(lb, NULL);

        /* Convert Old format to Clouseau_eo */
        treeit->eo_info = _clouseau_eo_list_convert(root);
        eo_dbg_info_free(root);
     }

     {
        /* Fetch properties of eo object */
        Clouseau_Eo_Dbg_Info *eo;
        Eina_List *expand_list = NULL, *l, *l_prev;
        Elm_Object_Item *eo_it;
        EINA_LIST_FOREACH(treeit->eo_info,l, eo)
          {
             Elm_Genlist_Item_Type iflag = (eo->type == EO_DBG_INFO_TYPE_LIST) ?
                ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE;
             eo_it = elm_genlist_item_append(prop_list, &itc, eo, NULL,
                   iflag, _gl_selected, NULL);
             expand_list = eina_list_append(expand_list, eo_it);
          }
        EINA_LIST_REVERSE_FOREACH_SAFE(expand_list, l, l_prev, eo_it)
          {
             elm_genlist_item_expanded_set(eo_it, EINA_TRUE);
             expand_list = eina_list_remove_list(expand_list, l);
          }
     }
}

EAPI void
clouseau_object_information_list_clear(void)
{
   elm_genlist_clear(prop_list);
}
