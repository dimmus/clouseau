#ifndef EFL_BETA_API_SUPPORT
#define EFL_BETA_API_SUPPORT
#endif
#ifndef EFL_EO_API_SUPPORT
#define EFL_EO_API_SUPPORT
#endif
#ifndef ELM_INTERNAL_API_ARGESFSDFEFC
#define ELM_INTERNAL_API_ARGESFSDFEFC
#endif
#include <getopt.h>

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

#define _EET_ENTRY "config"

static Evas_Object *
_obj_info_tootip(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
      Evas_Object *tt, void *item   EINA_UNUSED);

static int _cl_stat_reg_op = EINA_DEBUG_OPCODE_INVALID;
static int _module_init_op = EINA_DEBUG_OPCODE_INVALID;
static int _eoids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _klids_get_op = EINA_DEBUG_OPCODE_INVALID;
static int _obj_info_op = EINA_DEBUG_OPCODE_INVALID;
static int _obj_highlight_op = EINA_DEBUG_OPCODE_INVALID;

static Gui_Main_Win_Widgets *_main_widgets = NULL;

#if 0
typedef struct
{
   int *opcode; /* address to the opcode */
   void *buffer;
   int size;
} _future;
#endif

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
   int thread_id;
   Eina_List *children;
   Eo *glitem;
} Obj_Info;

typedef enum
{
   OBJ_CLASS = 0,
   OBJ_FUNC,
   OBJ_PARAM,
   OBJ_RET
} Obj_Info_Type;

typedef struct
{
   Obj_Info_Type type;
   void *data;
} _Obj_info_node;

typedef enum
{
   OFFLINE = 0,
   LOCAL_CONNECTION,
   REMOTE_CONNECTION,
   LAST_CONNECTION
} Connection_Type;

static const char *_conn_strs[] =
{
   "Offline",
   "Local",
   "Remote"
};

typedef struct
{
   const char *file_name;
   const char *name;
   const char *command;
   const char *script;
   /* Not eet */
   Eo *menu_item;
} Profile;

typedef struct
{
   int wdgs_show_type;
} Config;

static Connection_Type _conn_type = OFFLINE;
static Eina_Debug_Session *_session = NULL;
static int _selected_app = -1;

static Elm_Genlist_Item_Class *_objs_itc = NULL;
static Elm_Genlist_Item_Class *_obj_info_itc = NULL;
static Eolian_Debug_Object_Information *_obj_info = NULL;

static Eet_Data_Descriptor *_profile_edd = NULL, *_config_edd = NULL;
static Eina_List *_profiles = NULL;
static Config *_config = NULL;

static Eina_Hash *_classes_hash_by_id = NULL, *_classes_hash_by_name = NULL;

static Eina_Hash *_objs_hash = NULL;
static Eina_List *_objs_list_tree = NULL;

static Eina_Bool _eo_init_done = EINA_FALSE;
static Eina_Bool _eolian_init_done = EINA_FALSE;
static Eina_Bool _evas_init_done = EINA_FALSE;

static Eo *_menu_remote_item = NULL;
static Profile *_selected_profile = NULL;
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

#undef CFG_ADD_BASIC
}

static void
_config_save()
{
   char path[1024];
   sprintf(path, "%s/clouseau/config", efreet_config_home_get());
   _config_eet_load();
   Eet_File *file = eet_open(path, EET_FILE_MODE_WRITE);
   eet_data_write(file, _config_edd, _EET_ENTRY, _config, EINA_TRUE);
   eet_close(file);
}

static void
_profile_eet_load()
{
   if (_profile_edd) return;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Profile);
   _profile_edd = eet_data_descriptor_stream_new(&eddc);

#define CFG_ADD_BASIC(member, eet_type)\
   EET_DATA_DESCRIPTOR_ADD_BASIC\
   (_profile_edd, Profile, # member, member, eet_type)

   CFG_ADD_BASIC(name, EET_T_STRING);
   CFG_ADD_BASIC(command, EET_T_STRING);
   CFG_ADD_BASIC(script, EET_T_STRING);

#undef CFG_ADD_BASIC
}

static Profile *
_profile_find(const char *name)
{
   Eina_List *itr;
   Profile *p;
   EINA_LIST_FOREACH(_profiles, itr, p)
      if (p->name == name || !strcmp(p->name, name)) return p;
   return NULL;
}

static void
_profile_save(const Profile *p)
{
   char path[1024];
   if (!p) return;
   sprintf(path, "%s/clouseau/profiles/%s", efreet_config_home_get(), p->file_name);
   Eet_File *file = eet_open(path, EET_FILE_MODE_WRITE);
   _profile_eet_load();
   eet_data_write(file, _profile_edd, _EET_ENTRY, p, EINA_TRUE);
   eet_close(file);
   _profiles = eina_list_append(_profiles, p);
}

static void
_configs_load()
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
        Profile *p = eet_data_read(file, _profile_edd, _EET_ENTRY);
        p->file_name = eina_stringshare_add(filename);
        eet_close(file);
        _profiles = eina_list_append(_profiles, p);
     }
   sprintf(path, "%s/clouseau/config", efreet_config_home_get());
   _config_eet_load();
   Eet_File *file = eet_open(path, EET_FILE_MODE_READ);
   if (!file)
     {
        _config = calloc(1, sizeof(Config));
        _config->wdgs_show_type = 0;
        _config_save();
     }
   else
     {
        _config = eet_data_read(file, _config_edd, _EET_ENTRY);
        eet_close(file);
     }
}

#if 0
static Eina_List *_pending = NULL;

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
#endif

static void
_obj_info_expand_request_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *glit = event->info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);
}

static void
_obj_info_contract_request_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *glit = event->info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);
}

static void
_obj_info_expanded_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *glit = event->info;
   _Obj_info_node *node = elm_object_item_data_get(glit);
   Eina_List *itr;
   if(node->type == OBJ_CLASS)
     {
        Eolian_Debug_Class *kl = node->data;
        Eolian_Debug_Function *func;
        EINA_LIST_FOREACH(kl->functions, itr, func)
          {
             _Obj_info_node *node_itr = calloc(1, sizeof(*node_itr));
             node_itr->type = OBJ_FUNC;
             node_itr->data = func;

            Elm_Genlist_Item *glist =  elm_genlist_item_append(
                   event->object, _obj_info_itc, node_itr, glit,
                   ELM_GENLIST_ITEM_NONE, NULL, NULL);
            elm_genlist_item_tooltip_content_cb_set(glist, _obj_info_tootip, node_itr, NULL);
          }
     }
}

static void
_obj_info_contracted_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *glit = event->info;
   elm_genlist_item_subitems_clear(glit);
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
#if 0
        else if (eolian_type_type_get(base) == EOLIAN_TYPE_ENUM)
          {
             sprintf(c_type, "%s",  eolian_type_full_name_get(param_eolian_type));
          }
        else if (eolian_type_type_get(base) == EOLIAN_TYPE_ALIAS)
          {
             sprintf(c_type, "%s", eolian_type_full_name_get(base));
          }
#endif
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

   if(node->type == OBJ_CLASS)
     {
        return strdup(eolian_class_full_name_get(((Eolian_Debug_Class *)(node->data))->ekl));
     }
   else if(node->type == OBJ_FUNC)
     {
        char buffer[_MAX_LABEL];
        _obj_info_params_to_string(node, buffer, EINA_FALSE);
        return strdup(buffer);
     }
   return NULL;
}
#undef _MAX_LABEL

static Eina_Debug_Error
_obj_info_get(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED,
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
        node->type = OBJ_CLASS;
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

static void
_objs_expand_request_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *glit = event->info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);
}

static void
_objs_contract_request_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *glit = event->info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);
}

static void
_objs_sel_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   Obj_Info *info = elm_object_item_data_get(glit);

   elm_genlist_clear(_main_widgets->object_infos_list);
   eina_debug_session_send_to_thread(_session, _selected_app, info->thread_id,
         _obj_highlight_op, &(info->obj), sizeof(uint64_t));
   eina_debug_session_send_to_thread(_session, _selected_app, info->thread_id,
         _obj_info_op, &(info->obj), sizeof(uint64_t));
}

static void
_objs_expanded_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eina_List *itr;
   Elm_Object_Item *glit = event->info;
   Obj_Info *info = elm_object_item_data_get(glit), *it_data;
   EINA_LIST_FOREACH(info->children, itr, it_data)
     {
        Elm_Object_Item *nitem = elm_genlist_item_append(event->object, _objs_itc,
              it_data, glit,
              it_data->children ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
              _objs_sel_cb, NULL);
        elm_genlist_item_expanded_set(nitem, EINA_FALSE);
     }
}

static void
_objs_contracted_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Object_Item *glit = event->info;
   elm_genlist_item_subitems_clear(glit);
}

static char *
_objs_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   char buf[128];
   Obj_Info *oinfo = data;
   Class_Info *kinfo = eina_hash_find(_classes_hash_by_id, &(oinfo->kl_id));
   sprintf(buf, "%s %lX", kinfo ? kinfo->name : "(Unknown)", oinfo->obj);
   return strdup(buf);
}

Eina_Bool
screenshot_req_cb(void *data EINA_UNUSED, Eo *obj, const Efl_Event *event EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Obj_Info *info = efl_key_data_get(obj, "__info_node");

   printf("show screenshot of obj %lX\n", info->obj);
   return EINA_TRUE;
}

static void
_config_objs_type_sel_selected(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_key_data_set(event->object, "show_type_item", event->info);
}

void
gui_config_win_widgets_done(Gui_Config_Win_Widgets *wdgs)
{
   elm_win_modal_set(wdgs->win, EINA_TRUE);
   elm_object_text_set(wdgs->objs_types_sel, objs_types_strings[_config->wdgs_show_type]);
   efl_event_callback_add(wdgs->objs_types_sel, EFL_UI_EVENT_SELECTED, _config_objs_type_sel_selected, NULL);
}

void
config_ok_button_clicked(void *data, const Efl_Event *event EINA_UNUSED)
{
   Gui_Config_Win_Widgets *wdgs = data;
   Eo *item = efl_key_data_get(wdgs->objs_types_sel, "show_type_item");
   if (item)
     {
        _config->wdgs_show_type = (uintptr_t)elm_object_item_data_get(item);
     }
   _config_save();
   efl_del(wdgs->win);
}

static Evas_Object *
_objs_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   if(!strcmp(part, "elm.swallow.end"))
   {
      Gui_Screenshot_Button_Widgets *wdgs = gui_screenshot_button_create(obj);
      elm_object_tooltip_text_set(wdgs->screenshot_button, "Show App Screenshot");
      efl_key_data_set(wdgs->screenshot_button, "__info_node", data);
      return wdgs->screenshot_button;
   }
   return NULL;
}

static void
_objs_tree_free(Eina_List *parents)
{
   Obj_Info *info;

   EINA_LIST_FREE(parents, info)
     {
        _objs_tree_free(info->children);
        free(info);
     }
}

static void
_hoversel_selected_app(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _selected_app = (int)(long)data;

   if (_objs_list_tree)
     {
        _objs_tree_free(_objs_list_tree);
        _objs_list_tree = NULL;
     }
   eina_hash_free_buckets(_classes_hash_by_id);
   eina_hash_free_buckets(_classes_hash_by_name);

   elm_genlist_clear(_main_widgets->objects_list);
   elm_genlist_clear(_main_widgets->object_infos_list);
   eina_hash_free_buckets(_objs_hash);

   eina_debug_session_send(_session, _selected_app, _module_init_op, "eo", 3);
}

static Eina_Debug_Error
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
   return EINA_DEBUG_OK;
}

static void
_connection_reset()
{
   _selected_profile = NULL;
   _eo_init_done = EINA_FALSE;
   _eolian_init_done = EINA_FALSE;
   _evas_init_done = EINA_FALSE;
}

static Eina_Debug_Error
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

        if (cid == _selected_app) _connection_reset();
     }
   return EINA_DEBUG_OK;
}

static Eina_Debug_Error
_module_initted_cb(Eina_Debug_Session *session, int src, void *buffer, int size)
{
   if (size <= 0) return EINA_DEBUG_ERROR;
   Eina_Bool ret = !!((char *)buffer)[size - 1];
   if (!ret)
     {
        printf("Error loading module %s in the target\n", (char *)buffer);
     }
   if (!strcmp(buffer, "eo")) _eo_init_done = ret;
   if (!strcmp(buffer, "eolian")) _eolian_init_done = ret;
   if (!strcmp(buffer, "evas")) _evas_init_done = ret;

   if (!_eo_init_done)
     {
        eina_debug_session_send(_session, _selected_app, _module_init_op, "eo", 3);
        return EINA_DEBUG_OK;
     }
   if (!_eolian_init_done)
     {
        eina_debug_session_send(_session, _selected_app, _module_init_op, "eolian", 7);
        return EINA_DEBUG_OK;
     }
   if (!_evas_init_done)
     {
        eina_debug_session_send(_session, _selected_app, _module_init_op, "evas", 5);
        return EINA_DEBUG_OK;
     }
   eina_debug_session_send(session, src, _klids_get_op, NULL, 0);
   return EINA_DEBUG_OK;
}

static void
_klid_walk(void *data EINA_UNUSED, uint64_t kl, char *name)
{
   Class_Info *info = calloc(1, sizeof(*info));
   info->id = kl;
   info->name = eina_stringshare_add(name);
   eina_hash_add(_classes_hash_by_id, &(info->id), info);
   eina_hash_add(_classes_hash_by_name, info->name, info);
}

static Eina_Debug_Error
_klids_get(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   uint64_t obj_kl, canvas_kl;
   void *buf;
   eo_debug_klids_extract(buffer, size, _klid_walk, NULL);
   Class_Info *info = eina_hash_find(_classes_hash_by_name,
         _config->wdgs_show_type == 0 ? "Efl.Canvas.Object" : "Elm.Widget");
   if (info) obj_kl = info->id;
   info = eina_hash_find(_classes_hash_by_name, "Efl.Canvas");
   if (info) canvas_kl = info->id;
   buf = eo_debug_eoids_request_prepare(&size, obj_kl, canvas_kl, NULL);
   eina_debug_session_send_to_thread(session, src, 0xFFFFFFFF, _eoids_get_op, buf, size);
   free(buf);
   return EINA_DEBUG_OK;
}

static void
_eoid_walk(void *data, uint64_t obj, uint64_t kl_id, uint64_t parent)
{
   if (eina_hash_find(_objs_hash, &obj)) return;
   Eina_List **objs = data;
   Obj_Info *info = calloc(1, sizeof(*info));
   info->obj = obj;
   info->kl_id = kl_id;
   info->parent = parent;
   *objs = eina_list_append(*objs, info);
}

static Eina_Debug_Error
_eoids_get(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   Eina_List *objs = NULL, *l;
   Obj_Info *info;
   int thread_id;

   eo_debug_eoids_extract(buffer, size, _eoid_walk, &objs, &thread_id);

   EINA_LIST_FOREACH(objs, l, info)
     {
        info->thread_id = thread_id;
        eina_hash_add(_objs_hash, &(info->obj), info);
     }

   /* Fill children lists */
   EINA_LIST_FREE(objs, info)
     {
        Obj_Info *info_parent =  eina_hash_find(_objs_hash, &(info->parent));

        if (info_parent)
           info_parent->children = eina_list_append(info_parent->children, info);
        else
           _objs_list_tree = eina_list_append(_objs_list_tree, info);
     }

   /* Add to Genlist */
   EINA_LIST_FOREACH(_objs_list_tree, l, info)
     {
        if (!info->glitem)
          {
             info->glitem = elm_genlist_item_append(
                   _main_widgets->objects_list, _objs_itc, info, NULL,
                   info->children ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                   _objs_sel_cb, NULL);
             if (info->children)
                elm_genlist_item_expanded_set(info->glitem, EINA_FALSE);
          }
     }

   return EINA_DEBUG_OK;
}

static void
_ecore_thread_dispatcher(void *data)
{
   eina_debug_dispatch(_session, data);
}

Eina_Debug_Error
_disp_cb(Eina_Debug_Session *session EINA_UNUSED, void *buffer)
{
   ecore_main_loop_thread_safe_call_async(_ecore_thread_dispatcher, buffer);
   return EINA_DEBUG_OK;
}

static void
_post_register_handle(Eina_Bool flag)
{
   if(!flag) return;
   eina_debug_session_dispatch_override(_session, _disp_cb);
   eina_debug_session_send(_session, 0, _cl_stat_reg_op, NULL, 0);
}

static const Eina_Debug_Opcode ops[] =
{
     {"daemon/observer/client/register", &_cl_stat_reg_op, NULL},
     {"daemon/observer/slave_added", NULL, _clients_info_added_cb},
     {"daemon/observer/slave_deleted", NULL, _clients_info_deleted_cb},
     {"module/init",            &_module_init_op, &_module_initted_cb},
     {"Eo/objects_ids_get",     &_eoids_get_op, &_eoids_get},
     {"Eo/classes_ids_get",     &_klids_get_op, &_klids_get},
     {"Evas/object/highlight",  &_obj_highlight_op,  NULL},
     {"Eolian/object/info_get", &_obj_info_op, &_obj_info_get},
     {NULL, NULL, NULL}
};

#if 0
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
#endif

Eina_Bool
_new_profile_save_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Gui_New_Profile_Win_Widgets *wdgs = NULL;
   Profile *p = NULL;
   Eo *save_bt = event->object;
   wdgs = efl_key_data_get(save_bt, "_wdgs");
   const char *name = elm_object_text_get(wdgs->new_profile_name);
   const char *cmd = elm_object_text_get(wdgs->new_profile_command);
   const char *script = elm_entry_markup_to_utf8(elm_object_text_get(wdgs->new_profile_script));
   if (!name || !*name) return EINA_TRUE;
   if (!cmd || !*cmd) return EINA_TRUE;
   p = calloc(1, sizeof(*p));
   p->file_name = eina_stringshare_add(name); /* FIXME: Have to format name to conform to file names convention */
   p->name = eina_stringshare_add(name);
   p->command = eina_stringshare_add(cmd);
   p->script = eina_stringshare_add(script);
   _profile_save(p);
   efl_del(wdgs->new_profile_win);
   return EINA_TRUE;
}

void
gui_new_profile_win_create_done(Gui_New_Profile_Win_Widgets *wdgs)
{
   efl_key_data_set(wdgs->new_profile_save_button, "_wdgs", wdgs);
   efl_key_data_set(wdgs->new_profile_cancel_button, "_wdgs", wdgs);
}

static void
_connection_type_change(Connection_Type conn_type)
{
   /* FIXME disconnection if connected */
   if (_session) eina_debug_session_terminate(_session);
   _session = NULL;
   _connection_reset();
   elm_hoversel_clear(_main_widgets->apps_selector);
   switch (conn_type)
     {
      case OFFLINE:
           {
              efl_gfx_visible_set(_main_widgets->save_bt, EINA_FALSE);
              elm_box_unpack(_main_widgets->bar_box, _main_widgets->save_bt);
              elm_object_text_set(_main_widgets->load_button, "Load file...");
              break;
           }
      case LOCAL_CONNECTION:
           {
              efl_gfx_visible_set(_main_widgets->save_bt, EINA_TRUE);
              elm_box_pack_end(_main_widgets->bar_box, _main_widgets->save_bt);
              elm_object_text_set(_main_widgets->load_button, "Reload");
              _session = eina_debug_local_connect(EINA_TRUE);
              break;
           }
      case REMOTE_CONNECTION:
           {
              efl_gfx_visible_set(_main_widgets->save_bt, EINA_TRUE);
              elm_box_pack_end(_main_widgets->bar_box, _main_widgets->save_bt);
              elm_object_text_set(_main_widgets->load_button, "Reload");
#if 0
         eina_debug_session_basic_codec_add(_session, EINA_DEBUG_CODEC_SHELL);
         Eina_List *script_lines = _parse_script(_selected_profile->script);
         if (!eina_debug_shell_remote_connect(_session, _selected_profile->command, script_lines))
           {
              fprintf(stderr, "ERROR: Cannot connect to shell remote debug daemon.\n");
           }
#endif
              break;
           }
      default: return;
     }
   if (_session) eina_debug_opcodes_register(_session, ops, _post_register_handle);
   elm_object_text_set(_main_widgets->conn_selector, _conn_strs[conn_type]);
   _conn_type = conn_type;
}

static void
_menu_selected_conn(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _connection_type_change((uintptr_t)data);
}

static void
_menu_profile_selected(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _selected_profile = data;
   _connection_type_change(REMOTE_CONNECTION);
}

void
conn_menu_show(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   Eina_List *itr;
   Profile *p;
   int x = 0, y = 0, h = 0;
   efl_gfx_position_get(_main_widgets->conn_selector, &x, &y);
   efl_gfx_size_get(_main_widgets->conn_selector, NULL, &h);
   elm_menu_move(_main_widgets->conn_selector_menu, x, y + h);
   efl_gfx_visible_set(_main_widgets->conn_selector_menu, EINA_TRUE);

   EINA_LIST_FOREACH(_profiles, itr, p)
     {
        if (p->menu_item) continue;
        p->menu_item = elm_menu_item_add(_main_widgets->conn_selector_menu,
              _menu_remote_item, NULL, p->name,
              _menu_profile_selected, p);
        efl_wref_add(p->menu_item, &p->menu_item);
     }
}

static void
_profile_new_clicked(void *data EINA_UNUSED,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Gui_New_Profile_Win_Widgets *wdgs = gui_new_profile_win_create(NULL);
   gui_new_profile_win_create_done(wdgs);
}

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Connection_Type conn_type = OFFLINE;
   char *offline_filename = NULL;
   int i, long_index = 0, opt;
   Eina_Bool help = EINA_FALSE;

   eina_init();
   eolian_init();

   _configs_load();

   static struct option long_options[] =
     {
        /* These options set a flag. */
          {"help",      no_argument,        0, 'h'},
          {"local",     no_argument,        0, 'l'},
          {"remote",    required_argument,  0, 'r'},
          {"file",      required_argument,  0, 'f'},
          {0, 0, 0, 0}
     };
   while ((opt = getopt_long(argc, argv,"hlr:f:", long_options, &long_index )) != -1)
     {
        if (conn_type != OFFLINE || offline_filename)
          {
             printf("You cannot use more than one option at a time\n");
             help = EINA_TRUE;
          }
        switch (opt) {
           case 0: break;
           case 'l':
                   {
                      conn_type = LOCAL_CONNECTION;
                      break;
                   }
           case 'r':
                   {
                      conn_type = REMOTE_CONNECTION;
                      _selected_profile = _profile_find(optarg);
                      if (!_selected_profile)
                        {
                           printf("Profile %s not found\n", optarg);
                           help = EINA_TRUE;
                        }
                      break;
                   }
           case 'f':
                   {
                      conn_type = OFFLINE;
                      offline_filename = strdup(optarg);
                      break;
                   }
           case 'h': help = EINA_TRUE; break;
           default: help = EINA_TRUE;
        }
     }
   if (help)
     {
        printf("Usage: %s [-h/--help] [-v/--verbose] [options]\n", argv[0]);
        printf("       --help/-h Print that help\n");
        printf("       --local/-l Create a local connection\n");
        printf("       --remote/-r Create a remote connection by using the given profile name\n");
        printf("       --file/-f Run in offline mode and load the given file\n");
        return 0;
     }

   _classes_hash_by_id = eina_hash_pointer_new(NULL);
   _classes_hash_by_name = eina_hash_string_small_new(NULL);

   _objs_hash = eina_hash_pointer_new(NULL);

   eolian_directory_scan(EOLIAN_EO_DIR);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   _main_widgets = gui_gui_get()->main_win;

   for (i = 0; i < LAST_CONNECTION; i++)
     {
        if (i == REMOTE_CONNECTION)
          {
             _menu_remote_item = elm_menu_item_add(_main_widgets->conn_selector_menu,
                   NULL, NULL, _conn_strs[i], NULL, NULL);
             elm_menu_item_add(_main_widgets->conn_selector_menu,
                   _menu_remote_item, NULL, "New profile...",
                   _profile_new_clicked, NULL);
          }
        else
          {
             elm_menu_item_add(_main_widgets->conn_selector_menu,
                   NULL, NULL, _conn_strs[i],
                   _menu_selected_conn, (void *)(uintptr_t)i);
          }
     }
   _connection_type_change(conn_type);

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
   efl_event_callback_add(_main_widgets->objects_list, ELM_GENLIST_EVENT_EXPAND_REQUEST, _objs_expand_request_cb, NULL);
   efl_event_callback_add(_main_widgets->objects_list, ELM_GENLIST_EVENT_CONTRACT_REQUEST, _objs_contract_request_cb, NULL);
   efl_event_callback_add(_main_widgets->objects_list, ELM_GENLIST_EVENT_EXPANDED, _objs_expanded_cb, NULL);
   efl_event_callback_add(_main_widgets->objects_list, ELM_GENLIST_EVENT_CONTRACTED, _objs_contracted_cb, NULL);

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
   efl_event_callback_add(_main_widgets->object_infos_list, ELM_GENLIST_EVENT_EXPAND_REQUEST, _obj_info_expand_request_cb, NULL);
   efl_event_callback_add(_main_widgets->object_infos_list, ELM_GENLIST_EVENT_CONTRACT_REQUEST, _obj_info_contract_request_cb, NULL);
   efl_event_callback_add(_main_widgets->object_infos_list, ELM_GENLIST_EVENT_EXPANDED, _obj_info_expanded_cb, NULL);
   efl_event_callback_add(_main_widgets->object_infos_list, ELM_GENLIST_EVENT_CONTRACTED, _obj_info_contracted_cb, NULL);

   elm_run();

   eolian_debug_object_information_free(_obj_info);
   _objs_tree_free(_objs_list_tree);
   eina_debug_session_terminate(_session);
   eina_shutdown();
   eolian_shutdown();
   return 0;
}
ELM_MAIN()
