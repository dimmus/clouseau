#ifndef EFL_BETA_API_SUPPORT
#define EFL_BETA_API_SUPPORT
#endif
#ifndef EFL_EO_API_SUPPORT
#define EFL_EO_API_SUPPORT
#endif
#ifndef ELM_INTERNAL_API_ARGESFSDFEFC
#define ELM_INTERNAL_API_ARGESFSDFEFC
#endif
#include <Efreet.h>
#include <Elementary.h>
#include <Evas.h>
#include <Eolian.h>
#include <Ecore_File.h>
#include "gui.h"

#include <Eolian_Debug.h>

#define EXTRACT(_buf, pval, sz) \
{ \
   memcpy(pval, _buf, sz); \
   _buf += sz; \
}

#define _PROFILE_EET_ENTRY "config"

static Evas_Object *
_obj_info_tootip(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      Evas_Object *tt, void *item   EINA_UNUSED);

static uint32_t _cl_stat_reg_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _module_init_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _poll_on_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _poll_off_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _evlog_on_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _evlog_off_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _elm_list_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _obj_info_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _obj_highlight_opcode = EINA_DEBUG_OPCODE_INVALID;

static Gui_Main_Win_Widgets *_main_widgets = NULL;
static Gui_Profiles_Win_Widgets *_profiles_wdgs = NULL;

typedef struct
{
   uint32_t *opcode; /* address to the opcode */
   void *buffer;
   int size;
} _pending_request;

typedef struct
{
   Obj_Info *info;
   Eina_List *children;
} _Obj_list_node;

typedef enum
{
   CLOUSEAU_OBJ_CLASS = 0,
   CLOUSEAU_OBJ_FUNC,
   CLOUSEAU_OBJ_PARAM,
   CLOUSEAU_OBJ_RET
} Clouseau_Obj_Info_Type;

typedef struct
{
   Clouseau_Obj_Info_Type type;
   void *data;
} _Obj_info_node;

typedef enum
{
   CLOUSEAU_PROFILE_LOCAL,
   CLOUSEAU_PROFILE_SDB
} Clouseau_Profile_Type;

typedef struct
{
   const char *name;
   const char *script;
   Clouseau_Profile_Type type;
} Clouseau_Profile;

static Eina_List *_pending = NULL;
static Eina_Debug_Session *_session = NULL;
static int _selected_app = -1;
static Elm_Genlist_Item_Class *_objs_itc = NULL;
static Elm_Genlist_Item_Class *_obj_info_itc = NULL;
static Elm_Genlist_Item_Class *_profiles_itc = NULL;
static Eina_List *_objs_list_tree = NULL;
static Eolian_Debug_Object_Information *_obj_info = NULL;
static Eina_Debug_Client *_current_client = NULL;

static Eet_Data_Descriptor *_profile_edd = NULL;
static Eina_List *_profiles = NULL;
static Clouseau_Profile *_selected_profile = NULL;

static void
_consume(uint32_t opcode)
{
   if (!_pending) return;
   _pending_request *req;
   Eina_List *itr;
   EINA_LIST_FOREACH(_pending, itr, req)
     {
        if (*(req->opcode) != EINA_DEBUG_OPCODE_INVALID &&
           (opcode == EINA_DEBUG_OPCODE_INVALID || *(req->opcode) == opcode))
          {
             eina_debug_session_send(_current_client, *(req->opcode), req->buffer, req->size);
             _pending = eina_list_remove_list(_pending, itr);
             free(req->buffer);
             free(req);
             return;
          }
     }
}

static void
_pending_add(uint32_t *opcode, void *buffer, int size)
{
   _pending_request *req = calloc(1, sizeof(*req));
   req->opcode = opcode;
   req->buffer = buffer;
   req->size = size;
   _pending = eina_list_append(_pending, req);
}

static Eina_Bool
_obj_info_expand_request_cb(void *data EINA_UNUSED,
      Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED,
      void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);

   return EINA_TRUE;
}

static Eina_Bool
_obj_info_contract_request_cb(void *data EINA_UNUSED,
      Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED,
      void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);

   return EINA_TRUE;
}

static Eina_Bool
_obj_info_expanded_cb(void *data EINA_UNUSED, Eo *obj,
      const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   _Obj_info_node *node = elm_object_item_data_get(glit);
   Eina_List *itr;
   if(node->type == CLOUSEAU_OBJ_CLASS)
     {
        Eolian_Debug_Class *kl = node->data;
        Eolian_Debug_Function *func;
        EINA_LIST_FOREACH(kl->functions, itr, func)
          {
             _Obj_info_node *node_itr = calloc(1, sizeof(*node_itr));
             node_itr->type = CLOUSEAU_OBJ_FUNC;
             node_itr->data = func;

            Elm_Genlist_Item *glist =  elm_genlist_item_append(
                   obj, _obj_info_itc, node_itr, glit,
                   ELM_GENLIST_ITEM_NONE, NULL, NULL);
            elm_genlist_item_tooltip_content_cb_set(glist, _obj_info_tootip, node_itr, NULL);
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_obj_info_contracted_cb(void *data EINA_UNUSED,
      Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED,
      void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_subitems_clear(glit);

   return EINA_TRUE;
}

static void
_obj_info_item_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   _Obj_info_node *node = data;
   free(node);
}

static void
_eolian_type_to_string(const Eolian_Type *param_eolian_type, char *c_type)
{
   c_type[0] = '\0';
   if ((eolian_type_type_get(param_eolian_type) == EOLIAN_TYPE_REGULAR)//if its one of the base type or alias
         && !eolian_type_base_type_get(param_eolian_type))
     {
        sprintf(c_type, "%s", eolian_type_name_get(param_eolian_type));
     }
   else
     {
        const Eolian_Type *base = eolian_type_base_type_get(param_eolian_type);
        if ((eolian_type_type_get(base) == EOLIAN_TYPE_REGULAR) ||
              (eolian_type_type_get(base) == EOLIAN_TYPE_CLASS))
          {
             sprintf(c_type, "%s *", eolian_type_full_name_get(base));
          }
        else if (eolian_type_type_get(base) == EOLIAN_TYPE_VOID)
          {
             sprintf(c_type, "void *");
          }
        else if (eolian_type_type_get(base) == EOLIAN_TYPE_ENUM)
          {
             sprintf(c_type, "%s",  eolian_type_full_name_get(param_eolian_type));
          }
        else if (eolian_type_type_get(base) == EOLIAN_TYPE_ALIAS)
          {
             sprintf(c_type, "%s", eolian_type_full_name_get(base));
          }
        else
          {
             sprintf(c_type, "%s *", eolian_type_is_const(base) ? "const " : "");
          }
     }
}

static int
_eolian_value_to_string(Eolian_Debug_Value *value, char *buffer, int max)
{
   switch(value->type)
     {
      case EOLIAN_DEBUG_STRING: return snprintf(buffer, max, "%s ",
                                      (char *)value->value.value);
      case EOLIAN_DEBUG_POINTER: return snprintf(buffer, max, "%p ",
                                       (void *)value->value.value);
      case EOLIAN_DEBUG_CHAR: return snprintf(buffer, max, "%c ",
                                    (char)value->value.value);
      case EOLIAN_DEBUG_INT: return snprintf(buffer, max, "%d ",
                                   (int)value->value.value);
      case EOLIAN_DEBUG_SHORT: return snprintf(buffer, max, "%u ",
                                     (unsigned int)value->value.value);
      case EOLIAN_DEBUG_DOUBLE: return snprintf(buffer, max, "%f ",
                                      (double)value->value.value);
      case EOLIAN_DEBUG_BOOLEAN: return snprintf(buffer, max, "%s ",
                                       (value->value.value ? "true" : "false"));
      case EOLIAN_DEBUG_LONG: return snprintf(buffer, max, "%ld ",
                                       (long)value->value.value);
      case EOLIAN_DEBUG_UINT: return snprintf(buffer, max, "%u ",
                                       (unsigned int)value->value.value);
      default: return snprintf(buffer, max, "%lX ", value->value.value);
     }
}

#define _MAX_LABEL 2000
static void
_obj_info_params_to_string(_Obj_info_node *node, char *buffer, Eina_Bool full)
{
   Eina_List *itr;
   int buffer_size = 0;
   buffer_size += snprintf(buffer + buffer_size,
         _MAX_LABEL - buffer_size,  "%s:  ",
         eolian_function_name_get(((Eolian_Debug_Function *)(node->data))->efunc));
   buffer[0] = toupper(buffer[0]);

   Eolian_Debug_Function *func =  (Eolian_Debug_Function *)(node->data);
   Eolian_Debug_Parameter *param;
   EINA_LIST_FOREACH(func->params, itr, param)
     {
        if(full)
          {
             char c_type[_MAX_LABEL];
             _eolian_type_to_string(eolian_parameter_type_get(param->etype), c_type);
             buffer_size += snprintf(buffer + buffer_size,
                   _MAX_LABEL - buffer_size, "%s ", c_type);
          }
        buffer_size += snprintf(buffer + buffer_size,
              _MAX_LABEL - buffer_size, "%s: ", eolian_parameter_name_get(param->etype));
        buffer_size += _eolian_value_to_string(&(param->value),
              buffer + buffer_size,  _MAX_LABEL - buffer_size);
        if(full)
           buffer_size += snprintf(buffer + buffer_size,
                 _MAX_LABEL - buffer_size, "(%lX) ", param->value.value.value);

     }
   if(func->params == NULL)
     {
        if(full)
          {
             char c_type[_MAX_LABEL];
             _eolian_type_to_string(func->ret.etype, c_type);
             buffer_size += snprintf(buffer + buffer_size,
                   _MAX_LABEL - buffer_size, "%s ", c_type);
          }
        buffer_size += snprintf(buffer + buffer_size,
              _MAX_LABEL - buffer_size, "%s: ", "");
        buffer_size += _eolian_value_to_string(&(func->ret.value),
              buffer + buffer_size,  _MAX_LABEL - buffer_size);
        if(full)
           buffer_size += snprintf(buffer + buffer_size,
                 _MAX_LABEL - buffer_size, "(%lX) ", func->ret.value.value.value);
     }
}

static Evas_Object *
_obj_info_tootip(void *data   EINA_UNUSED,
              Evas_Object *obj EINA_UNUSED,
              Evas_Object *tt,
              void *item   EINA_UNUSED)
{
   Evas_Object *l = elm_label_add(tt);
   char buffer[_MAX_LABEL];
   _obj_info_params_to_string(data, buffer, EINA_TRUE);
   elm_object_text_set(l, buffer);
   elm_label_line_wrap_set(l, ELM_WRAP_NONE);

   return l;
}

static char *
_obj_info_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   _Obj_info_node *node = data;

   if(node->type == CLOUSEAU_OBJ_CLASS)
     {
        return strdup(eolian_class_full_name_get(((Eolian_Debug_Class *)(node->data))->ekl));
     }
   else if(node->type == CLOUSEAU_OBJ_FUNC)
     {
        char buffer[_MAX_LABEL];
        _obj_info_params_to_string(node, buffer, EINA_FALSE);
        return strdup(buffer);
     }
   return NULL;
}
#undef _MAX_LABEL

static Eina_Bool
_debug_obj_info_cb(Eina_Debug_Client *src EINA_UNUSED,
      void *buffer, int size)
{
   if(_obj_info)
     {
        elm_genlist_clear(_main_widgets->object_infos_list);
        eolian_debug_object_information_free(_obj_info);
        _obj_info = NULL;
     }
   _obj_info = eolian_debug_object_information_decode(buffer, size);

   Eolian_Debug_Class *kl;
   Eina_List *kl_itr;
   EINA_LIST_FOREACH(_obj_info->classes, kl_itr, kl)
     {
        Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_TREE;
        _Obj_info_node *node = NULL;
        node = calloc(1, sizeof(*node));
        node->type = CLOUSEAU_OBJ_CLASS;
        node->data = kl;

        Elm_Object_Item  *glg = elm_genlist_item_append(
              _main_widgets->object_infos_list, _obj_info_itc,
              (void *)node, NULL,
              type,
              NULL, NULL);
        elm_genlist_item_expanded_set(glg, EINA_FALSE);
     }

   return EINA_TRUE;
}

static Eina_Bool
_objs_expand_request_cb(void *data EINA_UNUSED,
      Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED,
      void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);

   return EINA_TRUE;
}

static Eina_Bool
_objs_contract_request_cb(void *data EINA_UNUSED,
      Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED,
      void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);

   return EINA_TRUE;
}

static void
_objs_sel_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   _Obj_list_node *info_node = elm_object_item_data_get(glit);

   uint64_t ptr = (uint64_t)info_node->info->ptr;

   printf("Sending Eolian get request for Eo object[%p]\n", info_node->info->ptr);
   elm_genlist_clear(_main_widgets->object_infos_list);
   eina_debug_session_send(_current_client, _obj_info_opcode, &ptr, sizeof(uint64_t));
   eina_debug_session_send(_current_client, _obj_highlight_opcode, &ptr, sizeof(uint64_t));
}

static Eina_Bool
_objs_expanded_cb(void *data EINA_UNUSED, Eo *obj,
      const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Eina_List *itr;
   Elm_Object_Item *glit = event_info;
   _Obj_list_node *info_node = elm_object_item_data_get(glit), *it_data;
   EINA_LIST_FOREACH(info_node->children, itr, it_data)
     {
        Elm_Object_Item *nitem = elm_genlist_item_append(obj, _objs_itc,
              it_data, glit,
              it_data->children ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
              _objs_sel_cb, NULL);
        elm_genlist_item_expanded_set(nitem, EINA_FALSE);
     }

   return EINA_TRUE;
}

static Eina_Bool
_objs_contracted_cb(void *data EINA_UNUSED,
      Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED,
      void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_subitems_clear(glit);

   return EINA_TRUE;
}

static char *
_objs_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   _Obj_list_node *info_node = data;
   char buf[128];
   sprintf(buf, "%s %p", info_node->info->kl_name, info_node->info->ptr);
   return strdup(buf);
}

static void
_objs_nodes_free(Eina_List *parents)
{
  _Obj_list_node *info_node;

   EINA_LIST_FREE(parents, info_node)
     {
        if (info_node->info) free(info_node->info->kl_name);
        free(info_node->info);
        _objs_nodes_free(info_node->children);
        free(info_node);
     }
}

static void
_hoversel_selected_app(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _selected_app = (int)(long)data;

   if(_objs_list_tree)
     {
        _objs_nodes_free(_objs_list_tree);
        _objs_list_tree = NULL;
        elm_genlist_clear(_main_widgets->objects_list);
        elm_genlist_clear(_main_widgets->object_infos_list);
     }

   if (_current_client) eina_debug_client_free(_current_client);
   _current_client = eina_debug_client_new(_session, _selected_app);
   eina_debug_session_send(_current_client, _module_init_opcode, "elementary", 11);
   eina_debug_session_send(_current_client, _module_init_opcode, "eolian", 7);
   eina_debug_session_send(_current_client, _module_init_opcode, "evas", 5);
   _pending_add(&_elm_list_opcode, NULL, 0);
}

static Eina_Bool
_clients_info_added_cb(Eina_Debug_Client *src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   while(size)
     {
        int cid, pid, len;
        EXTRACT(buf, &cid, sizeof(uint32_t));
        EXTRACT(buf, &pid, sizeof(uint32_t));
        if(pid != getpid())
          {
             char option[100];
             snprintf(option, 90, "%s [%d]", buf, pid);
             elm_hoversel_item_add(_main_widgets->apps_selector,
                   option, "home", ELM_ICON_STANDARD, _hoversel_selected_app,
                   (void *)(long)cid);
          }
        len = strlen(buf) + 1;
        buf += len;
        size -= (2 * sizeof(uint32_t) + len);
     }
   return EINA_TRUE;
}

static Eina_Bool
_clients_info_deleted_cb(Eina_Debug_Client *src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   if(size >= (int)sizeof(uint32_t))
     {
        int cid;
        EXTRACT(buf, &cid, sizeof(uint32_t));

        const Eina_List *items = elm_hoversel_items_get(_main_widgets->apps_selector);
        const Eina_List *l;
        Elm_Object_Item *hoversel_it;

        EINA_LIST_FOREACH(items, l, hoversel_it)
          {
             if((int)(long)elm_object_item_data_get(hoversel_it) == cid)
               {
                  elm_object_item_del(hoversel_it);
                  break;
               }
          }
     }
   return EINA_TRUE;
}

static Eina_Bool
_elm_objects_list_cb(Eina_Debug_Client *src EINA_UNUSED, void *buffer, int size)
{
   Eina_List *objs = eo_debug_list_response_decode(buffer, size);
   Obj_Info *info;

   Eina_Hash *objects_hash = NULL;
   Eina_List *l = NULL;
   objects_hash = eina_hash_pointer_new(NULL);
   _Obj_list_node *info_node;

   /* Add all objects to hash table */
   EINA_LIST_FOREACH(objs, l, info)
     {
        info_node = calloc(1, sizeof(_Obj_list_node));
        info_node->info = info;
        info_node->children = NULL;
        eina_hash_add(objects_hash, &(info_node->info->ptr), info_node);
     }

   /* Fill children lists */
   EINA_LIST_FOREACH(objs, l, info)
     {
        _Obj_list_node *info_parent =  eina_hash_find(objects_hash, &(info->parent));
        info_node =  eina_hash_find(objects_hash, &(info->ptr));

        if(info_parent)
           info_parent->children = eina_list_append(info_parent->children, info_node);
        else
           _objs_list_tree = eina_list_append(_objs_list_tree, info_node);
     }

   /* Add to Genlist */
   EINA_LIST_FOREACH(_objs_list_tree, l, info_node)
     {
        Elm_Object_Item  *glg = elm_genlist_item_append(
              _main_widgets->objects_list, _objs_itc,
              (void *)info_node, NULL,
              info_node->children ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
              _objs_sel_cb, NULL);
        if (info_node->children) elm_genlist_item_expanded_set(glg, EINA_FALSE);
     }

   /* Free allocated memory */
   eina_hash_free(objects_hash);
   eina_list_free(objs);

   return EINA_TRUE;
}

static void
_ecore_thread_dispatcher(void *data)
{
   eina_debug_dispatch(_session, data);
}

Eina_Bool
_disp_cb(Eina_Debug_Session *session EINA_UNUSED, void *buffer)
{
   ecore_main_loop_thread_safe_call_async(_ecore_thread_dispatcher, buffer);
   return EINA_TRUE;
}

static Eina_Bool
_module_initted(Eina_Debug_Client *src EINA_UNUSED, void *buffer, int size)
{
   if (size > 0)
     {
        if (!strcmp(buffer, "elementary")) _consume(_elm_list_opcode);
     }
   return EINA_TRUE;
}

static void
_post_register_handle(Eina_Bool flag)
{
   if(!flag) return;
   eina_debug_session_dispatch_override(_session, _disp_cb);
   Eina_Debug_Client *cl = eina_debug_client_new(_session, 0);
   eina_debug_session_send(cl, _cl_stat_reg_opcode, NULL, 0);
   eina_debug_client_free(cl);
}

static Eina_Bool
_mkdir(const char *dir)
{
   if (!ecore_file_exists(dir))
     {
        Eina_Bool success = ecore_file_mkdir(dir);
        if (!success)
          {
             printf("Cannot create a config folder \"%s\"\n", dir);
             return EINA_FALSE;
          }
     }
   return EINA_TRUE;
}

static void
_profile_eet_load()
{
   if (_profile_edd) return;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Clouseau_Profile);
   _profile_edd = eet_data_descriptor_stream_new(&eddc);

#define CFG_ADD_BASIC(member, eet_type)\
   EET_DATA_DESCRIPTOR_ADD_BASIC\
   (_profile_edd, Clouseau_Profile, # member, member, eet_type)

   CFG_ADD_BASIC(name, EET_T_STRING);
   CFG_ADD_BASIC(script, EET_T_STRING);
   CFG_ADD_BASIC(type, EET_T_INT);

#undef CFG_ADD_BASIC
}

static void
_config_load()
{
   char path[1024], *filename;
   sprintf(path, "%s/clouseau", efreet_config_home_get());
   if (!_mkdir(path)) return;
   sprintf(path, "%s/clouseau/profiles", efreet_config_home_get());
   if (!_mkdir(path)) return;
   Eina_List *files = ecore_file_ls(path), *itr;
   if (files) _profile_eet_load();
   EINA_LIST_FOREACH(files, itr, filename)
     {
        sprintf(path, "%s/clouseau/profiles/%s", efreet_config_home_get(), filename);
        Eet_File *file = eet_open(path, EET_FILE_MODE_READ);
        Clouseau_Profile *p = eet_data_read(file, _profile_edd, _PROFILE_EET_ENTRY);
        eet_close(file);
        _profiles = eina_list_append(_profiles, p);
     }
}

static Clouseau_Profile *
_profile_find(const char *name)
{
   Eina_List *itr;
   Clouseau_Profile *p;
   EINA_LIST_FOREACH(_profiles, itr, p)
      if (p->name == name || !strcmp(p->name, name)) return p;
   return NULL;
}

static void
_profile_save(const Clouseau_Profile *p, const char *filename)
{
   char path[1024];
   if (!p) return;
   sprintf(path, "%s/clouseau/profiles/%s", efreet_config_home_get(), filename);
   Eet_File *file = eet_open(path, EET_FILE_MODE_WRITE);
   _profile_eet_load();
   eet_data_write(file, _profile_edd, _PROFILE_EET_ENTRY, p, EINA_TRUE);
   eet_close(file);
}

static const Eina_Debug_Opcode ops[] =
{
     {"daemon/client_status_register", &_cl_stat_reg_opcode, NULL},
     {"daemon/client_added", NULL, _clients_info_added_cb},
     {"daemon/client_deleted", NULL, _clients_info_deleted_cb},
     {"Module/Init",          &_module_init_opcode,   &_module_initted},
     {"poll/on",              &_poll_on_opcode,       NULL},
     {"poll/off",             &_poll_off_opcode,      NULL},
     {"evlog/on",             &_evlog_on_opcode,      NULL},
     {"evlog/off",            &_evlog_off_opcode,     NULL},
     {"Elementary/objects_list",       &_elm_list_opcode,      &_elm_objects_list_cb},
     {"Eolian/object/info_get", &_obj_info_opcode, &_debug_obj_info_cb},
     {"Evas/object/highlight", &_obj_highlight_opcode, NULL},
     {NULL, NULL, NULL}
};

static void
_profile_load()
{
   _session = eina_debug_session_new();

   switch (_selected_profile->type)
     {
      case CLOUSEAU_PROFILE_LOCAL:
         if (!eina_debug_local_connect(_session))
           {
              fprintf(stderr, "ERROR: Cannot connect to debug daemon.\n");
              elm_exit();
           }
         break;
      default:
           {
              printf("Profile type %d not supported\n", _selected_profile->type);
              elm_exit();
           }
     }

   eina_debug_opcodes_register(_session, ops, _post_register_handle);
}

static char *_profile_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   Clouseau_Profile *p = data;
   return strdup(p->name);
}

static void
_profile_sel_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   _selected_profile = elm_object_item_data_get(glit);
   elm_object_disabled_set(_profiles_wdgs->profile_ok_button, EINA_FALSE);
}

Eina_Bool
_profile_win_close_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info EINA_UNUSED)
{
   eo_del(_profiles_wdgs->profiles_win);
   _profiles_wdgs = NULL;
   _profile_load();
   return EINA_TRUE;
}

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   eina_init();
   eolian_init();

   _config_load();
   if (!_profile_find("Local connection"))
     {
        Clouseau_Profile *p = calloc(1, sizeof(*p));
        p->name = eina_stringshare_add("Local connection");
        p->type = CLOUSEAU_PROFILE_LOCAL;
        _profile_save(p, "local");
     }

   if (!_profiles_itc)
     {
        _profiles_itc = elm_genlist_item_class_new();
        _profiles_itc->item_style = "default";
        _profiles_itc->func.text_get = _profile_item_label_get;
     }

   eolian_directory_scan(EOLIAN_EO_DIR);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   _main_widgets = gui_gui_get()->main_win;

   //Init objects Genlist
   if (!_objs_itc)
     {
        _objs_itc = elm_genlist_item_class_new();
        _objs_itc->item_style = "default";
        _objs_itc->func.text_get = _objs_item_label_get;
        _objs_itc->func.content_get = NULL;
        _objs_itc->func.state_get = NULL;
        _objs_itc->func.del = NULL;
     }
   eo_do(_main_widgets->objects_list,
         eo_event_callback_add(ELM_GENLIST_EVENT_EXPAND_REQUEST, _objs_expand_request_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_CONTRACT_REQUEST, _objs_contract_request_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_EXPANDED, _objs_expanded_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_CONTRACTED, _objs_contracted_cb, NULL)
        );

   //Init object info Genlist
   if (!_obj_info_itc)
     {
        _obj_info_itc = elm_genlist_item_class_new();
        _obj_info_itc->item_style = "default";
        _obj_info_itc->func.text_get = _obj_info_item_label_get;
        _obj_info_itc->func.content_get = NULL;
        _obj_info_itc->func.state_get = NULL;
        _obj_info_itc->func.del =  _obj_info_item_del;
     }
   eo_do(_main_widgets->object_infos_list,
         eo_event_callback_add(ELM_GENLIST_EVENT_EXPAND_REQUEST, _obj_info_expand_request_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_CONTRACT_REQUEST, _obj_info_contract_request_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_EXPANDED, _obj_info_expanded_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_CONTRACTED, _obj_info_contracted_cb, NULL)
        );

   if (1) /* if a profile is not given as parameter, we show the profiles window */
     {
        _profiles_wdgs = gui_profiles_win_create(_main_widgets->main_win);
        elm_win_modal_set(_profiles_wdgs->profiles_win, EINA_TRUE);
        Eina_List *itr;
        Clouseau_Profile *p;
        EINA_LIST_FOREACH(_profiles, itr, p)
           elm_genlist_item_append(_profiles_wdgs->profiles_list, _profiles_itc, p,
                 NULL, ELM_GENLIST_ITEM_NONE, _profile_sel_cb, NULL);
     }

   elm_run();

   eolian_debug_object_information_free(_obj_info);
   _objs_nodes_free(_objs_list_tree);
   eina_debug_session_free(_session);
   eina_shutdown();
   eolian_shutdown();
   return 0;
}
ELM_MAIN()
