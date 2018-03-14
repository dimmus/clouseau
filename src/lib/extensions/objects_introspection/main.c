#include <Eina.h>
#include <Eolian.h>

#include "../../Clouseau_Debug.h"

#include "gui.h"
#include "../../Clouseau.h"

#define _EET_ENTRY "config"

#define STORE(_buf, pval, sz) \
{ \
   memcpy(_buf, pval, sz); \
   _buf += sz; \
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define SWAP_64(x) x
#define SWAP_32(x) x
#define SWAP_16(x) x
#define SWAP_DBL(x) x
#else
#define SWAP_64(x) eina_swap64(x)
#define SWAP_32(x) eina_swap32(x)
#define SWAP_16(x) eina_swap16(x)
#define SWAP_DBL(x) SWAP_64(x)
#endif

static int _eoids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _klids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _obj_info_op = EINA_DEBUG_OPCODE_INVALID;
static int _obj_highlight_op = EINA_DEBUG_OPCODE_INVALID;
static int _win_screenshot_op = EINA_DEBUG_OPCODE_INVALID;
static int _snapshot_do_op = EINA_DEBUG_OPCODE_INVALID;
static int _snapshot_done_op = EINA_DEBUG_OPCODE_INVALID;

typedef struct
{
   uint64_t id;
   Eina_Stringshare *name;
} Class_Info;

typedef struct
{
   uint64_t obj;
   uint64_t parent;
   uint64_t kl_id;
   Eina_List *children;
   Eina_List *screenshots;
   Eo *glitem;
   Eo *screenshots_menu;
   Eolian_Debug_Object_Information *eolian_info;
} Obj_Info;

typedef struct
{
   void *buffer;
   int cur_len;
   int max_len;
} Snapshot_Buffer;

typedef struct
{
   Snapshot_Buffer klids_buf;
   Snapshot_Buffer eoids_buf;
   Snapshot_Buffer obj_infos_buf;
   Snapshot_Buffer screenshots_buf;
} Snapshot;

typedef struct
{
   int wdgs_show_type;
   Eina_Bool highlight;
} Config;

typedef struct
{
   Gui_Win_Widgets *wdgs;
   Snapshot snapshot;
   Obj_Info *selected_obj;
   Eina_Hash *classes_hash_by_id;
   Eina_Hash *classes_hash_by_name;
   Eina_Hash *objs_hash;
   Eina_List *objs_list_tree;
   Eina_Debug_Dispatch_Cb old_disp_cb;
} Instance;

static Eet_Data_Descriptor *_config_edd = NULL;
static Config *_config = NULL;

static Elm_Genlist_Item_Class *_objs_itc = NULL;
static Elm_Genlist_Item_Class *_obj_kl_info_itc = NULL;
static Elm_Genlist_Item_Class *_obj_func_info_itc = NULL;



static Evas_Object * _obj_info_tootip(void *, Evas_Object *, Evas_Object *, void *);

static Eina_Bool _eoids_get(Eina_Debug_Session *, int, void *, int);
static Eina_Bool _klids_get(Eina_Debug_Session *, int, void *, int);
static Eina_Bool _obj_info_get(Eina_Debug_Session *, int, void *, int);
static Eina_Bool _snapshot_done_cb(Eina_Debug_Session *, int, void *, int);
static Eina_Bool _win_screenshot_get(Eina_Debug_Session *, int, void *, int);

EINA_DEBUG_OPCODES_ARRAY_DEFINE(_ops,
     {"Clouseau/Eo/objects_ids_get",     &_eoids_get_op, &_eoids_get},
     {"Clouseau/Eo/classes_ids_get",     &_klids_get_op, &_klids_get},
     {"Clouseau/Evas/object/highlight",  &_obj_highlight_op, NULL},
     {"Clouseau/Evas/window/screenshot", &_win_screenshot_op, &_win_screenshot_get},
     {"Clouseau/Eolian/object/info_get", &_obj_info_op, &_obj_info_get},
     {"Clouseau/Object_Introspection/snapshot_start",&_snapshot_do_op, NULL},
     {"Clouseau/Object_Introspection/snapshot_done", &_snapshot_done_op, &_snapshot_done_cb},
     {NULL, NULL, NULL}
);

static Clouseau_Extension *
_ext_get(Eo *obj)
{
   if (!obj) return NULL;
   Clouseau_Extension *ext = efl_key_data_get(obj, "__extension");
   if (!ext) ext = _ext_get(efl_parent_get(obj));
   return ext;
}

static void
_config_eet_load()
{
   if (_config_edd) return;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Config);
   _config_edd = eet_data_descriptor_stream_new(&eddc);

#define CFG_ADD_BASIC(member, eet_type)\
   EET_DATA_DESCRIPTOR_ADD_BASIC\
   (_config_edd, Config, # member, member, eet_type)

   CFG_ADD_BASIC(wdgs_show_type, EET_T_INT);
   CFG_ADD_BASIC(highlight, EET_T_INT);

#undef CFG_ADD_BASIC
}

static void
_config_save(Clouseau_Extension *ext)
{
   char path[1024];
   sprintf(path, "%s/objects_introspection_config", ext->path_to_config);
   _config_eet_load();
   Eet_File *file = eet_open(path, EET_FILE_MODE_WRITE);
   eet_data_write(file, _config_edd, _EET_ENTRY, _config, EINA_TRUE);
   eet_close(file);
}

static void
_config_load(Clouseau_Extension *ext)
{
   char path[1024];
   sprintf(path, "%s/objects_introspection_config", ext->path_to_config);
   _config_eet_load();
   Eet_File *file = eet_open(path, EET_FILE_MODE_READ);
   if (!file)
     {
        _config = calloc(1, sizeof(Config));
        _config->wdgs_show_type = 0;
        _config->highlight = EINA_TRUE;
        _config_save(ext);
     }
   else
     {
        _config = eet_data_read(file, _config_edd, _EET_ENTRY);
        eet_close(file);
     }
}

static void
_objs_tree_free(Eina_List *parents)
{
   Obj_Info *info;
   EINA_LIST_FREE(parents, info)
     {
        eolian_debug_object_information_free(info->eolian_info);
        _objs_tree_free(info->children);
        free(info);
     }
}

static void
_obj_highlight(Clouseau_Extension *ext, uint64_t obj)
{
   if (!_config->highlight) return;
   obj = SWAP_64(obj);
   eina_debug_session_send(ext->session, ext->app_id,
         _obj_highlight_op, &obj, sizeof(uint64_t));
}

static void
_app_snapshot_request(Clouseau_Extension *ext)
{
   const char *obj_kl_name = _config->wdgs_show_type == 0 ? "Efl.Canvas.Object" : "Efl.Ui.Widget";
   const char *canvas_kl_name = "Efl.Canvas";
   int size = strlen(obj_kl_name) + 1 + strlen(canvas_kl_name) + 1;
   char *buf = alloca(size);

   ext->ui_freeze_cb(ext, EINA_TRUE);
   memcpy(buf, obj_kl_name, strlen(obj_kl_name) + 1);
   memcpy(buf + strlen(obj_kl_name) + 1, canvas_kl_name, strlen(canvas_kl_name) + 1);
   eina_debug_session_send(ext->session, ext->app_id, _snapshot_do_op, buf, size);
}

static void
_app_changed(Clouseau_Extension *ext)
{
   static int app_id = -1;
   Instance *inst = ext->data;
   Snapshot *s = &(inst->snapshot);

   elm_genlist_clear(inst->wdgs->objects_list);
   elm_genlist_clear(inst->wdgs->object_infos_list);
   if (inst->objs_list_tree)
     {
        _objs_tree_free(inst->objs_list_tree);
        inst->objs_list_tree = NULL;
     }
   inst->selected_obj = NULL;
   eina_hash_free_buckets(inst->classes_hash_by_id);
   eina_hash_free_buckets(inst->classes_hash_by_name);

   eina_hash_free_buckets(inst->objs_hash);

   s->klids_buf.cur_len = 0;
   s->eoids_buf.cur_len = 0;
   s->obj_infos_buf.cur_len = 0;
   s->screenshots_buf.cur_len = 0;
   app_id = ext->app_id;
   if (app_id)
     {
        elm_object_item_disabled_set(inst->wdgs->reload_button, EINA_FALSE);
        _app_snapshot_request(ext);
     }
}

static void
_snapshot_buffer_append(Snapshot_Buffer *sb, void *buffer)
{
   Eina_Debug_Packet_Header *hdr = (Eina_Debug_Packet_Header *)buffer;
   int size = hdr->size;
   if (sb->max_len < sb->cur_len + size)
     {
        /* Realloc with addition of 1MB+size */
        sb->max_len += size + 1000000;
        sb->buffer = realloc(sb->buffer, sb->max_len);
     }
   memcpy((char *)sb->buffer + sb->cur_len, buffer, size);
   sb->cur_len += size;
}

Eina_Bool
_disp_cb(Eina_Debug_Session *session, void *buffer)
{
   Clouseau_Extension *ext = eina_debug_session_data_get(session);
   if (!ext) return EINA_TRUE;
   Instance *inst = ext->data;
   if (inst)
     {
        Snapshot *s = &inst->snapshot;
        Eina_Debug_Packet_Header *hdr = (Eina_Debug_Packet_Header *)buffer;
        Snapshot_Buffer *sb = NULL;
        if (hdr->opcode == _eoids_get_op) sb = &(s->eoids_buf);
        else if (hdr->opcode == _klids_get_op) sb = &(s->klids_buf);
        else if (hdr->opcode == _obj_info_op) sb = &(s->obj_infos_buf);
        else if (hdr->opcode == _win_screenshot_op) sb = &(s->screenshots_buf);
        if (sb) _snapshot_buffer_append(sb, buffer);
        return inst->old_disp_cb(session, buffer);
     }
   return EINA_FALSE;
}

static void
_post_register_handle(void *data, Eina_Bool flag)
{
   Clouseau_Extension *ext = data;
   if(!flag) return;
   if (ext->app_id)
      _app_snapshot_request(ext);
}

static void
_session_changed(Clouseau_Extension *ext)
{
   int i = 0;
   Instance *inst = ext->data;
   Eina_Debug_Opcode *ops = _ops();
   _app_changed(ext);
   while (ops[i].opcode_name)
     {
        if (ops[i].opcode_id) *(ops[i].opcode_id) = EINA_DEBUG_OPCODE_INVALID;
        i++;
     }
   if (ext->session)
     {
        eina_debug_session_data_set(ext->session, ext);
        inst->old_disp_cb = eina_debug_session_dispatch_get(ext->session);
        eina_debug_session_dispatch_override(ext->session, _disp_cb);
        eina_debug_opcodes_register(ext->session, ops, _post_register_handle, ext);
     }
   elm_object_item_disabled_set(inst->wdgs->reload_button, EINA_TRUE);
}


static void
_snapshot_load(Clouseau_Extension *ext, void *buffer, int size, int version EINA_UNUSED)
{
   char *buf = buffer;
   int klids_op, eoids_op, obj_info_op, screenshot_op;
   if (!ext) return;

#define EXTRACT(_buf, pval, sz) \
     { \
        memcpy(pval, _buf, sz); \
        _buf += sz; \
     }
   EXTRACT(buf, &klids_op, sizeof(int));
   EXTRACT(buf, &eoids_op, sizeof(int));
   EXTRACT(buf, &obj_info_op, sizeof(int));
   EXTRACT(buf, &screenshot_op, sizeof(int));
#undef EXTRACT

   _session_changed(ext);

   while (size > 0)
     {
        Eina_Debug_Packet_Header *hdr = (Eina_Debug_Packet_Header *)buf;
        void *payload = buf + sizeof(*hdr);
        int psize = hdr->size - sizeof(*hdr);
        if (hdr->opcode == eoids_op)
           _eoids_get((Eina_Debug_Session *)ext, -1, payload, psize);
        else if (hdr->opcode == klids_op)
           _klids_get((Eina_Debug_Session *)ext, -1, payload, psize);
        else if (hdr->opcode == obj_info_op)
           _obj_info_get((Eina_Debug_Session *)ext, -1, payload, psize);
        else if (hdr->opcode == screenshot_op)
           _win_screenshot_get((Eina_Debug_Session *)ext, -1, payload, psize);
        else return;
        buf += hdr->size;
        size -= hdr->size;
     }
}

static void
_obj_info_expand_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);
}

static void
_obj_info_contract_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);
}

static void
_ptr_highlight(Clouseau_Extension *ext, Eolian_Debug_Value *v)
{
   switch (v->type)
     {
      case EOLIAN_DEBUG_POINTER:
           {
              _obj_highlight(ext, v->value.value);
              break;
           }
      case EOLIAN_DEBUG_LIST:
           {
              Eina_List *itr;
              EINA_LIST_FOREACH(v->complex_type_values, itr, v)
                {
                   _obj_highlight(ext, v->value.value);
                }
              break;
           }
      default: break;
     }
}

static void
_obj_info_gl_selected(void *data EINA_UNUSED, Evas_Object *pobj,
      void *event_info)
{
   Clouseau_Extension *ext = _ext_get(pobj);
   Eolian_Debug_Function *func = elm_object_item_data_get(event_info);
   Eolian_Debug_Parameter *p;

   if (eina_list_count(func->params) == 1)
     {
        p = eina_list_data_get(func->params);
        _ptr_highlight(ext, &(p->value));
     }
   _ptr_highlight(ext, &(func->ret.value));
}

static void
_obj_info_expanded_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   const Elm_Genlist_Item_Class *itc = elm_genlist_item_item_class_get(glit);
   if (itc == _obj_kl_info_itc)
     {
        Eolian_Debug_Class *kl = elm_object_item_data_get(glit);
        Eolian_Debug_Function *func;
        Eina_List *itr;
        EINA_LIST_FOREACH(kl->functions, itr, func)
          {
            Elm_Genlist_Item *glist =  elm_genlist_item_append(
                   obj, _obj_func_info_itc, func, glit,
                   ELM_GENLIST_ITEM_NONE, _obj_info_gl_selected, NULL);
            elm_genlist_item_tooltip_content_cb_set(glist, _obj_info_tootip, func, NULL);
          }
     }
}

static void
_obj_info_contracted_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_subitems_clear(glit);
}

static void
_eolian_type_to_string(const Eolian_Type *param_eolian_type, Eina_Strbuf *buf)
{
   Eolian_Type_Type type = eolian_type_type_get(param_eolian_type);
   //if its one of the base type or alias
   if ((type == EOLIAN_TYPE_REGULAR || type == EOLIAN_TYPE_CLASS) &&
         !eolian_type_base_type_get(param_eolian_type))
     {
        eina_strbuf_append_printf(buf, "%s", eolian_type_name_get(param_eolian_type));
     }
   else
     {
        const Eolian_Type *base = eolian_type_base_type_get(param_eolian_type);
        if ((eolian_type_type_get(base) == EOLIAN_TYPE_REGULAR) ||
              (eolian_type_type_get(base) == EOLIAN_TYPE_CLASS))
          {
             eina_strbuf_append_printf(buf, "%s *", eolian_type_name_get(base));
          }
        else if (eolian_type_type_get(base) == EOLIAN_TYPE_VOID)
          {
             eina_strbuf_append(buf, "void *");
          }
        else
          {
             eina_strbuf_append_printf(buf, "%s *", eolian_type_is_const(base) ? "const " : "");
          }
     }
}

static void
_eolian_value_to_string(Eolian_Debug_Value *value, Eina_Strbuf *buf)
{
   switch(value->type)
     {
      case EOLIAN_DEBUG_STRING:
           {
              eina_strbuf_append_printf(buf, "\"%s\" ",
                    (char *)value->value.value);
              break;
           }
      case EOLIAN_DEBUG_POINTER:
           {
              eina_strbuf_append_printf(buf, "%p ",
                    (void *)value->value.value);
              break;
           }
      case EOLIAN_DEBUG_CHAR:
           {
              eina_strbuf_append_printf(buf, "%c ",
                    (char)value->value.value);
              break;
           }
      case EOLIAN_DEBUG_INT:
           {
              eina_strbuf_append_printf(buf, "%d ",
                    (int)value->value.value);
              break;
           }
      case EOLIAN_DEBUG_SHORT:
           {
              eina_strbuf_append_printf(buf, "%u ",
                    (unsigned int)value->value.value);
              break;
           }
      case EOLIAN_DEBUG_DOUBLE:
           {
              eina_strbuf_append_printf(buf, "%f ",
                    (double)value->value.value);
              break;
           }
      case EOLIAN_DEBUG_BOOLEAN:
           {
              eina_strbuf_append_printf(buf, "%s ",
                    (value->value.value ? "true" : "false"));
              break;
           }
      case EOLIAN_DEBUG_LONG:
           {
              eina_strbuf_append_printf(buf, "%ld ",
                    (long)value->value.value);
              break;
           }
      case EOLIAN_DEBUG_UINT:
           {
              eina_strbuf_append_printf(buf, "%u ",
                    (unsigned int)value->value.value);
              break;
           }
      case EOLIAN_DEBUG_LIST:
           {
              Eina_List *l = value->complex_type_values, *itr;
              eina_strbuf_append_printf(buf, "%lX [", value->value.value);
              EINA_LIST_FOREACH(l, itr, value)
                {
                   eina_strbuf_append_printf(buf, "%s%lX",
                         l != itr ? ", " : "",
                         value->value.value);
                }
              eina_strbuf_append(buf, "]");
              break;
           }
      default: eina_strbuf_append_printf(buf, "%lX ", value->value.value);
     }
}

static void
_func_params_to_string(Eolian_Debug_Function *func, Eina_Strbuf *buffer, Eina_Bool full)
{
   Eina_List *itr;
   eina_strbuf_append_printf(buffer, "%s:  ",
         eolian_function_name_get(func->efunc));

   Eolian_Debug_Parameter *param;
   EINA_LIST_FOREACH(func->params, itr, param)
     {
        Eina_Stringshare *pname = eolian_parameter_name_get(param->eparam);
        if (full)
          {
             _eolian_type_to_string(eolian_parameter_type_get(param->eparam), buffer);
          }
        if (pname && eina_list_count(func->params) != 1)
          {
             eina_strbuf_append_printf(buffer, "%s: ", pname);
          }
        _eolian_value_to_string(&(param->value), buffer);
     }
   if (func->params == NULL)
     {
        if(full)
          {
             _eolian_type_to_string(func->ret.etype, buffer);
          }
        _eolian_value_to_string(&(func->ret.value), buffer);
     }
}

static Evas_Object *
_obj_info_tootip(void *data,
              Evas_Object *obj EINA_UNUSED,
              Evas_Object *tt,
              void *item   EINA_UNUSED)
{
   Evas_Object *l = elm_label_add(tt);
   Eina_Strbuf *buf = eina_strbuf_new();
   _func_params_to_string(data, buf, EINA_TRUE);
   elm_object_text_set(l, eina_strbuf_string_get(buf));
   elm_label_line_wrap_set(l, ELM_WRAP_NONE);

   eina_strbuf_free(buf);
   return l;
}

static char *
_obj_kl_info_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   Eolian_Debug_Class *kl = data;
   return strdup(eolian_class_name_get(kl->ekl));
}

static char *
_obj_func_info_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   char *ret;
   Eina_Strbuf *buf = eina_strbuf_new();
   Eolian_Debug_Function *func = data;
   _func_params_to_string(func, buf, EINA_FALSE);
   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

static void
_obj_info_realize(Clouseau_Extension *ext, Eolian_Debug_Object_Information *e_info)
{
   Instance *inst = ext->data;
   Eolian_Debug_Class *kl;
   Eina_List *kl_itr;

   elm_genlist_clear(inst->wdgs->object_infos_list);
   EINA_LIST_FOREACH(e_info->classes, kl_itr, kl)
     {
        Elm_Object_Item  *glg = elm_genlist_item_append(
              inst->wdgs->object_infos_list, _obj_kl_info_itc,
              kl, NULL, ELM_GENLIST_ITEM_TREE, NULL, NULL);
        elm_genlist_item_expanded_set(glg, EINA_FALSE);
     }
}

static Eina_Bool
_obj_info_get(Eina_Debug_Session *session, int src, void *buffer, int size)
{
   Clouseau_Extension *ext = NULL;
   Instance *inst = NULL;
   if (src == -1)
     {
        ext = (Clouseau_Extension *)session;
        session = NULL;
     }
   else
     {
        ext = eina_debug_session_data_get(session);
     }
   inst = ext->data;
   Eolian_Debug_Object_Information *e_info =
      eolian_debug_object_information_decode(buffer, size);
   Obj_Info *o_info = eina_hash_find(inst->objs_hash, &(e_info->obj));
   if (!o_info)
     {
        eolian_debug_object_information_free(e_info);
        return EINA_TRUE;
     }

   if (o_info->eolian_info)
      eolian_debug_object_information_free(o_info->eolian_info);
   o_info->eolian_info = e_info;

   if (o_info == inst->selected_obj) _obj_info_realize(ext, e_info);

   return EINA_TRUE;
}

static void
_objs_expand_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);
}

static void
_objs_contract_request_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);
}

static void
_objs_sel_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Clouseau_Extension *ext = _ext_get(obj);
   if (!ext) return;
   Instance *inst = ext->data;
   if (!ext->app_id) return;
   Elm_Object_Item *glit = event_info;
   Obj_Info *info = elm_object_item_data_get(glit);

   inst->selected_obj = info;

   elm_genlist_clear(inst->wdgs->object_infos_list);
   _obj_highlight(ext, info->obj);
   if (info->eolian_info) _obj_info_realize(ext, info->eolian_info);
}

static void
_objs_expanded_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Eina_List *itr;
   Clouseau_Extension *ext = _ext_get(obj);
   Elm_Object_Item *glit = event_info;
   Obj_Info *info = elm_object_item_data_get(glit), *it_data;
   if (!ext) return;
   EINA_LIST_FOREACH(info->children, itr, it_data)
     {
        it_data->glitem = elm_genlist_item_append(obj, _objs_itc,
              it_data, glit,
              it_data->children ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
              _objs_sel_cb, ext);
        elm_genlist_item_expanded_set(it_data->glitem, EINA_FALSE);
        efl_wref_add(it_data->glitem, &(it_data->glitem));
     }
}

static void
_objs_contracted_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_subitems_clear(glit);
}

static char *
_objs_item_label_get(void *data, Evas_Object *obj,
      const char *part EINA_UNUSED)
{
   char buf[128];
   Clouseau_Extension *ext = _ext_get(obj);
   Instance *inst = ext->data;
   Obj_Info *oinfo = data;
   Class_Info *kinfo = eina_hash_find(inst->classes_hash_by_id, &(oinfo->kl_id));
   sprintf(buf, "%s %lX", kinfo ? kinfo->name : "(Unknown)", oinfo->obj);
   return strdup(buf);
}

static Eina_Bool
_win_screenshot_get(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED,
      void *buffer, int size)
{
   Clouseau_Extension *ext = NULL;
   Instance *inst = NULL;
   if (src == -1)
     {
        ext = (Clouseau_Extension *)session;
        session = NULL;
     }
   else
     {
        ext = eina_debug_session_data_get(session);
     }
   inst = ext->data;
   Evas_Debug_Screenshot *s = evas_debug_screenshot_decode(buffer, size);
   if (!s) return EINA_FALSE;
   Obj_Info *info = eina_hash_find(inst->objs_hash, &(s->obj));
   if (!info)
     {
        free(s->img);
        free(s);
        return EINA_TRUE;
     }
   info->screenshots = eina_list_append(info->screenshots, s);
   if (info->glitem) elm_genlist_item_update(info->glitem);
   return EINA_TRUE;
}

void
take_screenshot_button_clicked(void *data EINA_UNUSED, const Efl_Event *event)
{
   Obj_Info *info = efl_key_data_get(event->object, "__info_node");
   Clouseau_Extension *ext = _ext_get(event->object);

   if (!ext || !ext->app_id) return;

   eina_debug_session_send(ext->session, ext->app_id,
         _win_screenshot_op, &(info->obj), sizeof(uint64_t));
}

static void
_screenshot_display(Evas_Debug_Screenshot *s)
{
   Gui_Show_Screenshot_Win_Widgets *wdgs = gui_show_screenshot_win_create(NULL);

   Eo *img = evas_object_image_filled_add(evas_object_evas_get(wdgs->win));

   evas_object_size_hint_min_set(img, s->w, s->h);
   elm_object_content_set(wdgs->bg, img);

   evas_object_image_colorspace_set(img, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(img, EINA_FALSE);
   evas_object_image_size_set(img, s->w, s->h);
   evas_object_image_data_copy_set(img, s->img);
   evas_object_image_data_update_add(img, 0, 0, s->w, s->h);
   evas_object_show(img);

   evas_object_resize(wdgs->win, s->w, s->h);
}

static void
_menu_screenshot_selected(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _screenshot_display(data);
}

void
show_screenshot_button_clicked(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eo *bt = event->object;
   Obj_Info *info = efl_key_data_get(event->object, "__info_node");
   if (eina_list_count(info->screenshots) == 1)
     {
        _screenshot_display(eina_list_data_get(info->screenshots));
     }
   else
     {
        Eina_List *itr;
        Evas_Debug_Screenshot *s;
        Clouseau_Extension *ext = _ext_get(bt);
        Instance *inst = ext->data;
        Eina_Position2D bt_pos = {.x = 0, .y = 0};
        Eina_Size2D bt_size = {.w = 0, .h = 0};

        if (info->screenshots_menu) efl_del(info->screenshots_menu);
        info->screenshots_menu = elm_menu_add(inst->wdgs->main);
        efl_wref_add(info->screenshots_menu, &info->screenshots_menu);

        bt_pos = efl_gfx_position_get(bt);
        bt_size = efl_gfx_size_get(bt);
        elm_menu_move(info->screenshots_menu, bt_pos.x, bt_pos.y + bt_size.w);
        efl_gfx_visible_set(info->screenshots_menu, EINA_TRUE);
        EINA_LIST_FOREACH(info->screenshots, itr, s)
          {
             char str[200];
             sprintf(str, "%.2d:%.2d:%.2d",
                   s->tm_hour, s->tm_min, s->tm_sec);
             elm_menu_item_add(info->screenshots_menu,
                   NULL, NULL, str, _menu_screenshot_selected, s);
          }
     }
}

static Eina_Bool
_snapshot_done_cb(Eina_Debug_Session *session, int src EINA_UNUSED,
      void *buffer EINA_UNUSED, int size EINA_UNUSED)
{
   Clouseau_Extension *ext = eina_debug_session_data_get(session);
   ext->ui_freeze_cb(ext, EINA_FALSE);
   return EINA_TRUE;
}

static void *
_snapshot_save(Clouseau_Extension *ext, int *size, int *version)
{
   Instance *inst = ext->data;
   Snapshot *s = &(inst->snapshot);
   void *buffer = malloc(4 * sizeof(int) +
         s->klids_buf.cur_len + s->eoids_buf.cur_len +
         s->obj_infos_buf.cur_len + s->screenshots_buf.cur_len);
   char *tmp = buffer;

   STORE(tmp, &_klids_get_op, sizeof(int));
   STORE(tmp, &_eoids_get_op, sizeof(int));
   STORE(tmp, &_obj_info_op, sizeof(int));
   STORE(tmp, &_win_screenshot_op, sizeof(int));
   STORE(tmp, s->klids_buf.buffer, s->klids_buf.cur_len);
   STORE(tmp, s->eoids_buf.buffer, s->eoids_buf.cur_len);
   STORE(tmp, s->obj_infos_buf.buffer, s->obj_infos_buf.cur_len);
   STORE(tmp, s->screenshots_buf.buffer, s->screenshots_buf.cur_len);

   *version = 1;
   *size = tmp - (char *)buffer;
   return buffer;
}

void
objs_type_changed(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   int type = (uintptr_t) data;
   Clouseau_Extension *ext = _ext_get(obj);
   Instance *inst = ext->data;
   elm_radio_value_set(inst->wdgs->objs_type_radio, type);
   _config->wdgs_show_type = type;
   _config_save(ext);
   _app_changed(ext);
}

void
highlight_changed(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = _ext_get(obj);
   Instance *inst = ext->data;
   _config->highlight = !_config->highlight;
   elm_check_state_set(inst->wdgs->highlight_ck, _config->highlight);
   _config_save(ext);
}

static Evas_Object *
_objs_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   static uint64_t canvas_id = 0;
   Obj_Info *info = data;
   Clouseau_Extension *ext = _ext_get(obj);
   if (!ext) return NULL;
   Instance *inst = ext->data;
   if (!canvas_id)
     {
        Class_Info *kl_info = eina_hash_find(inst->classes_hash_by_name, "Evas.Canvas");
        if (kl_info) canvas_id = kl_info->id;
     }
   if (info->kl_id == canvas_id && !strcmp(part, "elm.swallow.end"))
   {
      Eo *box = elm_box_add(obj);
      evas_object_size_hint_weight_set(box, 1.000000, 1.000000);
      elm_box_horizontal_set(box, EINA_TRUE);
      efl_gfx_visible_set(box, EINA_TRUE);

      if (info->screenshots)
        {
           Gui_Show_Screenshot_Button_Widgets *swdgs = gui_show_screenshot_button_create(box);
           if (eina_list_count(info->screenshots) == 1)
             {
                elm_object_tooltip_text_set(swdgs->bt, "Show screenshot");
             }
           else
             {
                elm_object_tooltip_text_set(swdgs->bt, "List screenshots");
             }
           efl_key_data_set(swdgs->bt, "__info_node", info);
           efl_key_data_set(swdgs->bt, "__extension", ext);
           elm_box_pack_end(box, swdgs->bt);
        }

      if (ext->session)
        {
           Gui_Take_Screenshot_Button_Widgets *twdgs = gui_take_screenshot_button_create(box);
           elm_object_tooltip_text_set(twdgs->bt, "Take screenshot");
           efl_key_data_set(twdgs->bt, "__info_node", info);
           efl_key_data_set(twdgs->bt, "__extension", ext);
           elm_box_pack_end(box, twdgs->bt);
        }

      return box;
   }
   return NULL;
}

static void
_klid_walk(void *data, uint64_t kl, char *name)
{
   Instance *inst = data;
   Class_Info *info = calloc(1, sizeof(*info));
   info->id = kl;
   info->name = eina_stringshare_add(name);
   eina_hash_add(inst->classes_hash_by_id, &(info->id), info);
   eina_hash_add(inst->classes_hash_by_name, info->name, info);
}

static Eina_Bool
_klids_get(Eina_Debug_Session *session, int src, void *buffer, int size)
{
   Clouseau_Extension *ext = NULL;
   Instance *inst = NULL;
   if (src == -1)
     {
        ext = (Clouseau_Extension *)session;
        session = NULL;
     }
   else
     {
        ext = eina_debug_session_data_get(session);
     }
   inst = ext->data;
   eo_debug_klids_extract(buffer, size, _klid_walk, inst);

   return EINA_TRUE;
}

static void
_eoid_walk(void *data, uint64_t obj, uint64_t kl_id, uint64_t parent)
{
   Eina_List **objs = data;
   Obj_Info *info = calloc(1, sizeof(*info));
   info->obj = obj;
   info->kl_id = kl_id;
   info->parent = parent;
   *objs = eina_list_append(*objs, info);
}

static Eina_Bool
_eoids_get(Eina_Debug_Session *session, int src, void *buffer, int size)
{
   Eina_List *objs = NULL, *l;
   Clouseau_Extension *ext = NULL;
   Instance *inst = NULL;
   Obj_Info *info;
   if (src == -1)
     {
        ext = (Clouseau_Extension *)session;
        session = NULL;
     }
   else
     {
        ext = eina_debug_session_data_get(session);
     }
   inst = ext->data;

   eo_debug_eoids_extract(buffer, size, _eoid_walk, &objs);

   EINA_LIST_FOREACH(objs, l, info)
     {
        eina_hash_add(inst->objs_hash, &(info->obj), info);
     }

   /* Fill children lists */
   EINA_LIST_FREE(objs, info)
     {
        Obj_Info *info_parent =  eina_hash_find(inst->objs_hash, &(info->parent));

        if (info_parent)
           info_parent->children = eina_list_append(info_parent->children, info);
        else
           inst->objs_list_tree = eina_list_append(inst->objs_list_tree, info);
     }

   /* Add to Genlist */
   EINA_LIST_FOREACH(inst->objs_list_tree, l, info)
     {
        if (!info->glitem)
          {
             info->glitem = elm_genlist_item_append(
                   inst->wdgs->objects_list, _objs_itc, info, NULL,
                   info->children ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                   _objs_sel_cb, ext);
             efl_wref_add(info->glitem, &(info->glitem));
             if (info->children)
                elm_genlist_item_expanded_set(info->glitem, EINA_FALSE);
          }
     }

   return EINA_TRUE;
}

static void
_item_realize(Instance *inst, Obj_Info *info)
{
   if (info->parent)
      {
         Obj_Info *pinfo =  eina_hash_find(inst->objs_hash, &(info->parent));
         if (pinfo)
           {
              if (!pinfo->glitem) _item_realize(inst, pinfo);
              elm_genlist_item_expanded_set(pinfo->glitem, EINA_TRUE);
           }
      }
}

static void
_jump_entry_changed(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Eo *en = obj;
   Eo *inwin = data;
   const char *ptr = elm_entry_entry_get(en);
   Clouseau_Extension *ext = efl_key_data_get(en, "__extension");
   Instance *inst = ext->data;
   uint64_t id = 0;
   Eina_Bool err = EINA_FALSE;

   printf("Ptr %s\n", ptr);
   while (*ptr && !err)
     {
        char c = *ptr;
        id <<= 4;
        if (c >= '0' && c <= '9') id |= (*ptr - '0');
        else if (c >= 'a' && c <= 'f') id |= (*ptr - 'a' + 0xA);
        else if (c >= 'A' && c <= 'F') id |= (*ptr - 'A' + 0xA);
        else err = EINA_TRUE;
        ptr++;
     }
   efl_del(inwin);
   if (!err)
     {
        Obj_Info *info =  eina_hash_find(inst->objs_hash, &id);
        if (!info) return;
        if (!info->glitem) _item_realize(inst, info);
        elm_genlist_item_show(info->glitem, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
        elm_genlist_item_selected_set(info->glitem, EINA_TRUE);
     }
}

void
jump_to_ptr_inwin_show(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = _ext_get(obj);
   Eo *inwin = ext->inwin_create_cb();
   Eo *entry = elm_entry_add(inwin);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_object_part_text_set(entry, "guide", "Jump To Pointer");
   evas_object_smart_callback_add(entry, "activated", _jump_entry_changed, inwin);
   efl_key_data_set(entry, "__extension", ext);
   evas_object_show(entry);
   elm_win_inwin_content_set(inwin, entry);
   elm_win_inwin_activate(inwin);
}

void
reload_perform(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = _ext_get(obj);
   if (ext->session)
     {
        /* Reload */
        if (!ext->app_id) return;
        _app_changed(ext);
     }
}

static Eo *
_ui_get(Clouseau_Extension *ext, Eo *parent)
{
   Instance *inst = ext->data;
   inst->wdgs = gui_win_create(parent);

   efl_key_data_set(inst->wdgs->main, "__extension", ext);
   efl_key_data_set(inst->wdgs->tb, "__extension", ext);
   efl_key_data_set(inst->wdgs->settings_menu, "__extension", ext);

   elm_radio_value_set(inst->wdgs->objs_type_radio, _config->wdgs_show_type);
   elm_check_state_set(inst->wdgs->highlight_ck, _config->highlight);
   //Init objects Genlist
   if (!_objs_itc)
     {
        _objs_itc = elm_genlist_item_class_new();
        _objs_itc->item_style = "default";
        _objs_itc->func.text_get = _objs_item_label_get;
        _objs_itc->func.content_get = _objs_item_content_get;
     }
   evas_object_smart_callback_add(inst->wdgs->objects_list, "expand,request", _objs_expand_request_cb, NULL);
   evas_object_smart_callback_add(inst->wdgs->objects_list, "contract,request", _objs_contract_request_cb, NULL);
   evas_object_smart_callback_add(inst->wdgs->objects_list, "expanded", _objs_expanded_cb, NULL);
   evas_object_smart_callback_add(inst->wdgs->objects_list, "contracted", _objs_contracted_cb, NULL);

   //Init object class info itc
   if (!_obj_kl_info_itc)
     {
        _obj_kl_info_itc = elm_genlist_item_class_new();
        _obj_kl_info_itc->item_style = "default";
        _obj_kl_info_itc->func.text_get = _obj_kl_info_item_label_get;
     }
   //Init object function info itc
   if (!_obj_func_info_itc)
     {
        _obj_func_info_itc = elm_genlist_item_class_new();
        _obj_func_info_itc->item_style = "default";
        _obj_func_info_itc->func.text_get = _obj_func_info_item_label_get;
     }
   evas_object_smart_callback_add(inst->wdgs->object_infos_list, "expand,request", _obj_info_expand_request_cb, NULL);
   evas_object_smart_callback_add(inst->wdgs->object_infos_list, "contract,request", _obj_info_contract_request_cb, NULL);
   evas_object_smart_callback_add(inst->wdgs->object_infos_list, "expanded", _obj_info_expanded_cb, NULL);
   evas_object_smart_callback_add(inst->wdgs->object_infos_list, "contracted", _obj_info_contracted_cb, NULL);
   return inst->wdgs->main;
}

EAPI const char *
extension_name_get()
{
   return "Objects introspection";
}

EAPI Eina_Bool
extension_start(Clouseau_Extension *ext, Eo *parent)
{
   Instance *inst = calloc(1, sizeof(*inst));

   eina_init();
   eolian_init();

   ext->data = inst;
   ext->session_changed_cb = _session_changed;
   ext->app_changed_cb = _app_changed;
   ext->import_data_cb = _snapshot_load;
   ext->export_data_cb = _snapshot_save;

   inst->classes_hash_by_id = eina_hash_pointer_new(NULL);
   inst->classes_hash_by_name = eina_hash_string_small_new(NULL);

   inst->objs_hash = eina_hash_pointer_new(NULL);

   memset(&(inst->snapshot), 0, sizeof(inst->snapshot));

   _config_load(ext);

   ext->ui_object = _ui_get(ext, parent);
   return !!ext->ui_object;
}

EAPI Eina_Bool
extension_stop(Clouseau_Extension *ext)
{
   Instance *inst = ext->data;

   efl_del(ext->ui_object);

   _objs_tree_free(inst->objs_list_tree);

   eina_hash_free(inst->objs_hash);

   eina_hash_free(inst->classes_hash_by_name);
   eina_hash_free(inst->classes_hash_by_id);

   free(inst);

   eolian_shutdown();
   eina_shutdown();

   return EINA_TRUE;
}
