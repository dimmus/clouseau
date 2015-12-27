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
#include <Eo_Debug.h>

#define SHOW_SCREENSHOT     "/images/show-screenshot.png"

#define EXTRACT(_buf, pval, sz) \
{ \
   memcpy(pval, _buf, sz); \
   _buf += sz; \
}

#define _PROFILE_EET_ENTRY "config"

static Evas_Object *
_obj_info_tootip(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      Evas_Object *tt, void *item   EINA_UNUSED);

static int _cl_stat_reg_opcode = EINA_DEBUG_OPCODE_INVALID;
static int _module_init_opcode = EINA_DEBUG_OPCODE_INVALID;
static int _eo_list_opcode = EINA_DEBUG_OPCODE_INVALID;
static int _obj_info_opcode = EINA_DEBUG_OPCODE_INVALID;
static int _obj_highlight_opcode = EINA_DEBUG_OPCODE_INVALID;

static Gui_Main_Win_Widgets *_main_widgets = NULL;
static Gui_Profiles_Win_Widgets *_profiles_wdgs = NULL;

typedef struct
{
   int *opcode; /* address to the opcode */
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
   CLOUSEAU_PROFILE_LOCAL = 1,
   CLOUSEAU_PROFILE_SHELL_REMOTE
} Clouseau_Profile_Type;

typedef struct
{
   const char *file_name;
   const char *name;
   const char *command;
   const char *script;
   Clouseau_Profile_Type type;
   Elm_Object_Item *item;
} Clouseau_Profile;

static Eina_List *_pending = NULL;
static Eina_Debug_Session *_session = NULL;
static int _selected_app = -1;
static Elm_Genlist_Item_Class *_objs_itc = NULL;
static Elm_Genlist_Item_Class *_obj_info_itc = NULL;
static Elm_Genlist_Item_Class *_profiles_itc = NULL;
static Eina_List *_objs_list_tree = NULL;
static Eolian_Debug_Object_Information *_obj_info = NULL;

static Eet_Data_Descriptor *_profile_edd = NULL;
static Eina_List *_profiles = NULL;
static Clouseau_Profile *_selected_profile = NULL;

static void
_consume(int opcode)
{
   if (!_pending) return;
   _pending_request *req;
   Eina_List *itr;
   EINA_LIST_FOREACH(_pending, itr, req)
     {
        if (*(req->opcode) != EINA_DEBUG_OPCODE_INVALID &&
           (opcode == EINA_DEBUG_OPCODE_INVALID || *(req->opcode) == opcode))
          {
             eina_debug_session_send(_session, _selected_app, *(req->opcode), req->buffer, req->size);
             _pending = eina_list_remove_list(_pending, itr);
             free(req->buffer);
             free(req);
             return;
          }
     }
}

static void
_pending_add(int *opcode, void *buffer, int size)
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
_debug_obj_info_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED,
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
   eina_debug_session_send(_session, _selected_app, _obj_info_opcode, &ptr, sizeof(uint64_t));
   eina_debug_session_send(_session, _selected_app, _obj_highlight_opcode, &ptr, sizeof(uint64_t));
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

Eina_Bool
screenshot_req_cb(void *data EINA_UNUSED, Eo *obj, const Eo_Event_Description *desc EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _Obj_list_node *info_node = NULL;
   eo_do(obj, info_node = eo_key_data_get("__info_node"));

   printf("show screenshot of obj %s %p\n", info_node->info->kl_name, info_node->info->ptr);
   return EINA_TRUE;
}

static Evas_Object *
_objs_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   if(!strcmp(part, "elm.swallow.end"))
   {
      Gui_Screenshot_Button_Widgets *wdgs = gui_screenshot_button_create(obj);
      elm_object_tooltip_text_set(wdgs->screenshot_button, "Show App Screenshot");
      eo_do(wdgs->screenshot_button, eo_key_data_set("__info_node", data));
      return wdgs->screenshot_button;
   }
   return NULL;
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

   eina_debug_session_send(_session, _selected_app, _module_init_opcode, "eo", 11);
   eina_debug_session_send(_session, _selected_app, _module_init_opcode, "eolian", 7);
   eina_debug_session_send(_session, _selected_app, _module_init_opcode, "evas", 5);
   _pending_add(&_eo_list_opcode, NULL, 0);
}

static Eina_Bool
_clients_info_added_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   while(size)
     {
        int cid, pid, len;
        EXTRACT(buf, &cid, sizeof(int));
        EXTRACT(buf, &pid, sizeof(int));
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
        size -= (2 * sizeof(int) + len);
     }
   return EINA_TRUE;
}

static Eina_Bool
_clients_info_deleted_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   if(size >= (int)sizeof(int))
     {
        int cid;
        EXTRACT(buf, &cid, sizeof(int));

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
_eo_objects_list_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
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
_module_initted(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   if (size > 0)
     {
        if (!strcmp(buffer, "eo")) _consume(_eo_list_opcode);
     }
   return EINA_TRUE;
}

static void
_post_register_handle(Eina_Bool flag)
{
   if(!flag) return;
   eina_debug_session_dispatch_override(_session, _disp_cb);
   eina_debug_session_send(_session, 0, _cl_stat_reg_opcode, NULL, 0);
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
   CFG_ADD_BASIC(command, EET_T_STRING);
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
        p->file_name = eina_stringshare_add(filename);
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
_profile_save(const Clouseau_Profile *p)
{
   char path[1024];
   if (!p) return;
   sprintf(path, "%s/clouseau/profiles/%s", efreet_config_home_get(), p->file_name);
   Eet_File *file = eet_open(path, EET_FILE_MODE_WRITE);
   _profile_eet_load();
   eet_data_write(file, _profile_edd, _PROFILE_EET_ENTRY, p, EINA_TRUE);
   eet_close(file);
   _profiles = eina_list_append(_profiles, p);
}

static const Eina_Debug_Opcode ops[] =
{
     {"daemon/observer/client/register", &_cl_stat_reg_opcode, NULL},
     {"daemon/observer/slave_added", NULL, _clients_info_added_cb},
     {"daemon/observer/slave_deleted", NULL, _clients_info_deleted_cb},
     {"module/init",            &_module_init_opcode,    &_module_initted},
     {"eo/objects_list",        &_eo_list_opcode,        &_eo_objects_list_cb},
     {"eolian/object/info_get", &_obj_info_opcode,       &_debug_obj_info_cb},
     {"evas/object/highlight",  &_obj_highlight_opcode,  NULL},
     {NULL, NULL, NULL}
};

static void
_profile_sel_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   _selected_profile = glit ? elm_object_item_data_get(glit) : NULL;
   elm_object_disabled_set(_profiles_wdgs->profile_ok_button, !_selected_profile);
   elm_object_disabled_set(_profiles_wdgs->profile_delete_button, !_selected_profile);
}

static Eina_List *
_parse_script(const char *script)
{
   Eina_List *lines = NULL;
   while (script && *script)
     {
        char *tmp = strchr(script, '\n');
        Eina_Stringshare *line;
        if (tmp)
          {
             line = eina_stringshare_add_length(script, tmp - script);
             script = tmp + 1;
          }
        else
          {
             line = eina_stringshare_add(script);
             script = NULL;
          }
        lines = eina_list_append(lines, line);
     }
   return lines;
}

static void
_profile_load()
{
   _session = eina_debug_session_new();

   switch (_selected_profile->type)
     {
      case CLOUSEAU_PROFILE_LOCAL:
         if (!eina_debug_local_connect(_session, EINA_DEBUG_SESSION_MASTER))
           {
              fprintf(stderr, "ERROR: Cannot connect to debug daemon.\n");
              elm_exit();
           }
         break;
      case CLOUSEAU_PROFILE_SHELL_REMOTE:
         eina_debug_session_basic_codec_add(_session, EINA_DEBUG_CODEC_SHELL);
         Eina_List *script_lines = _parse_script(_selected_profile->script);
         if (!eina_debug_shell_remote_connect(_session, _selected_profile->command, script_lines))
           {
              fprintf(stderr, "ERROR: Cannot connect to shell remote debug daemon.\n");
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

static void _profile_type_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   Gui_New_Profile_Win_Widgets *wdgs = NULL;
   eo_do(obj, wdgs = eo_key_data_get("_wdgs"));
   elm_object_text_set(obj, elm_object_item_text_get(event_info));
   Clouseau_Profile_Type type = (Clouseau_Profile_Type) data;
   if (type == CLOUSEAU_PROFILE_SHELL_REMOTE)
     {
        elm_object_disabled_set(wdgs->new_profile_command, EINA_FALSE);
        elm_object_disabled_set(wdgs->new_profile_script, EINA_FALSE);
     }
   else
     {
        elm_object_text_set(wdgs->new_profile_command, NULL);
        elm_object_text_set(wdgs->new_profile_script, NULL);
        elm_object_disabled_set(wdgs->new_profile_command, EINA_TRUE);
        elm_object_disabled_set(wdgs->new_profile_script, EINA_TRUE);
     }
   eo_do(wdgs->new_profile_type_selector, eo_key_data_set("_current_type", data));
}

Eina_Bool
_new_profile_save_cb(void *data, Eo *save_bt, const Eo_Event_Description *desc EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Gui_New_Profile_Win_Widgets *wdgs = NULL;
   Clouseau_Profile *p = NULL;
   eo_do(save_bt, wdgs = eo_key_data_get("_wdgs"));
   data = NULL;
   eo_do(wdgs->new_profile_type_selector, data = eo_key_data_get("_current_type"));
   if (!data) return EINA_TRUE; /* No type selected yet -> nothing done */
   Clouseau_Profile_Type type = (Clouseau_Profile_Type) data;
   const char *name = elm_object_text_get(wdgs->new_profile_name);
   const char *cmd = elm_object_text_get(wdgs->new_profile_command);
   const char *script = elm_entry_markup_to_utf8(elm_object_text_get(wdgs->new_profile_script));
   if (!name || !*name) return EINA_TRUE;
   if (type == CLOUSEAU_PROFILE_SHELL_REMOTE)
     {
        if (!cmd || !*cmd) return EINA_TRUE;
     }
   p = calloc(1, sizeof(*p));
   p->file_name = eina_stringshare_add(name); /* FIXME: Have to format name to conform to file names convention */
   p->name = eina_stringshare_add(name);
   p->type = type;
   p->command = eina_stringshare_add(cmd);
   p->script = eina_stringshare_add(script);
   _profile_save(p);
   eo_del(wdgs->new_profile_win);
   p->item = elm_genlist_item_append(_profiles_wdgs->profiles_list, _profiles_itc, p,
         NULL, ELM_GENLIST_ITEM_NONE, _profile_sel_cb, NULL);
   return EINA_TRUE;
}

void
gui_new_profile_win_create_done(Gui_New_Profile_Win_Widgets *wdgs)
   {
      eo_do(wdgs->new_profile_type_selector,
            eo_key_data_set("_wdgs", wdgs),
            elm_obj_hoversel_hover_parent_set(wdgs->new_profile_win),
            elm_obj_hoversel_item_add("Local connection", NULL, ELM_ICON_NONE, _profile_type_selected_cb, (void *)CLOUSEAU_PROFILE_LOCAL),
            elm_obj_hoversel_item_add("Shell remote", NULL, ELM_ICON_NONE, _profile_type_selected_cb, (void *)CLOUSEAU_PROFILE_SHELL_REMOTE));

      eo_do(wdgs->new_profile_save_button, eo_key_data_set("_wdgs", wdgs));
      eo_do(wdgs->new_profile_cancel_button, eo_key_data_set("_wdgs", wdgs));
}

static char *_profile_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   Clouseau_Profile *p = data;
   return strdup(p->name);
}

Eina_Bool
_profile_win_close_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info EINA_UNUSED)
{
   eo_del(_profiles_wdgs->profiles_win);
   _profiles_wdgs = NULL;
   _profile_load();
   return EINA_TRUE;
}

static void
_profile_item_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   Clouseau_Profile *p = data;
   p->item = NULL;
}

Eina_Bool
_profile_del_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info EINA_UNUSED)
{
   if (_selected_profile)
     {
        char path[1024];
        sprintf(path, "%s/clouseau/profiles/%s", efreet_config_home_get(), _selected_profile->file_name);
        remove(path);
        elm_object_item_del(_selected_profile->item);
        _profiles = eina_list_remove(_profiles, _selected_profile);
        eina_stringshare_del(_selected_profile->file_name);
        eina_stringshare_del(_selected_profile->name);
        free(_selected_profile);
        _selected_profile = NULL;
     }
   _profile_sel_cb(NULL, NULL, NULL);
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
        p->file_name = "local";
        p->name = eina_stringshare_add("Local connection");
        p->type = CLOUSEAU_PROFILE_LOCAL;
        _profile_save(p);
     }

   if (!_profiles_itc)
     {
        _profiles_itc = elm_genlist_item_class_new();
        _profiles_itc->item_style = "default";
        _profiles_itc->func.text_get = _profile_item_label_get;
        _profiles_itc->func.del = _profile_item_del;
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
        _objs_itc->func.content_get = _objs_item_content_get;
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
        _obj_info_itc->func.del = _obj_info_item_del;
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
           p->item = elm_genlist_item_append(_profiles_wdgs->profiles_list, _profiles_itc, p,
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
