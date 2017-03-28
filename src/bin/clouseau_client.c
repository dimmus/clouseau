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
#include <Evas_Debug.h>

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
static int _win_screenshot_op = EINA_DEBUG_OPCODE_INVALID;
static int _snapshot_do_op = EINA_DEBUG_OPCODE_INVALID;
static int _snapshot_done_op = EINA_DEBUG_OPCODE_INVALID;

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
   int cid;
   int pid;
   Eina_Stringshare *name;
   Eo *hs_item;
} App_Info;

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
   Eina_List *screenshots;
   Eo *glitem;
   Eo *screenshots_menu;
} Obj_Info;

typedef struct
{
   Eina_Stringshare *app_name;
   int app_pid;
   Eina_Stringshare *out_file;

   char *buffer;
   unsigned int max_len;
   unsigned int cur_len;

   int klids_op;
   int eoids_op;
   int obj_info_op;
} Snapshot;

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
   Eina_Bool highlight;
} Config;

static Connection_Type _conn_type = OFFLINE;
static Eina_Debug_Session *_session = NULL;
static int _selected_app = -1;

static Elm_Genlist_Item_Class *_objs_itc = NULL;
static Elm_Genlist_Item_Class *_obj_kl_info_itc = NULL;
static Elm_Genlist_Item_Class *_obj_func_info_itc = NULL;
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
static Eina_Bool _clouseau_init_done = EINA_FALSE;

static Eo *_menu_remote_item = NULL;
static Profile *_selected_profile = NULL;

static Snapshot *_snapshot = NULL;
static Eet_Data_Descriptor *_snapshot_edd = NULL;

static Eina_List *_apps = NULL;

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
   CFG_ADD_BASIC(highlight, EET_T_INT);

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
        _config->highlight = EINA_TRUE;
        _config_save();
     }
   else
     {
        _config = eet_data_read(file, _config_edd, _EET_ENTRY);
        eet_close(file);
     }
}

static App_Info *
_app_find_by_cid(int cid)
{
   Eina_List *itr;
   App_Info *info;
   EINA_LIST_FOREACH(_apps, itr, info)
     {
        if (info->cid == cid) return info;
     }
   return NULL;
}

static void
_app_del(int cid)
{
   App_Info *ai = _app_find_by_cid(cid);
   if (!ai) return;
   _apps = eina_list_remove(_apps, ai);
   eina_stringshare_del(ai->name);
   if (ai->hs_item) efl_del(ai->hs_item);
   free(ai);
}

static App_Info *
_app_add(int cid, int pid, const char *name)
{
   App_Info *ai = calloc(1, sizeof(*ai));
   ai->cid = cid;
   ai->pid = pid;
   ai->name = eina_stringshare_add(name);
   _app_del(cid);
   _apps = eina_list_append(_apps, ai);
   return ai;
}

static void
_snapshot_buffer_append(void *buffer)
{
   Eina_Debug_Packet_Header *hdr = (Eina_Debug_Packet_Header *)buffer;
   unsigned int size = hdr->size;
   if (_snapshot->max_len < _snapshot->cur_len + size)
     {
        /* Realloc with addition of 1MB+size */
        _snapshot->max_len += size + 1000000;
        _snapshot->buffer = realloc(_snapshot->buffer, _snapshot->max_len);
     }
   memcpy(_snapshot->buffer + _snapshot->cur_len, buffer, size);
   _snapshot->cur_len += size;
}

static void
_snapshot_eet_load()
{
   if (_snapshot_edd) return;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Snapshot);
   _snapshot_edd = eet_data_descriptor_stream_new(&eddc);

#define SNP_ADD_BASIC(member, eet_type)\
   EET_DATA_DESCRIPTOR_ADD_BASIC\
   (_snapshot_edd, Snapshot, # member, member, eet_type)

   SNP_ADD_BASIC(app_name, EET_T_STRING);
   SNP_ADD_BASIC(app_pid, EET_T_UINT);
   SNP_ADD_BASIC(eoids_op, EET_T_INT);
   SNP_ADD_BASIC(klids_op, EET_T_INT);
   SNP_ADD_BASIC(obj_info_op, EET_T_INT);

#undef SNP_ADD_BASIC
}

static Eina_Bool
_snapshot_save()
{
   FILE *fp = fopen(_snapshot->out_file, "w");
   void *out_buf = NULL;
   int out_size = 0;
   App_Info *ai = _app_find_by_cid(_selected_app);
   if (!ai) return EINA_FALSE;
   _snapshot_eet_load();
   if (!fp)
     {
        printf("Can not open file: \"%s\".\n", _snapshot->out_file);
        return EINA_FALSE;
     }
   _snapshot->app_name = ai->name;
   _snapshot->app_pid = ai->pid;
   _snapshot->eoids_op = _eoids_get_op;
   _snapshot->klids_op = _klids_get_op;
   _snapshot->obj_info_op = _obj_info_op;
   out_buf = eet_data_descriptor_encode(_snapshot_edd, _snapshot, &out_size);
   fwrite(&out_size, sizeof(int), 1, fp);
   fwrite(out_buf, 1, out_size, fp);
   printf("Snapshot buffer size %d max %d\n", _snapshot->cur_len, _snapshot->max_len);
   fwrite(_snapshot->buffer, 1, _snapshot->cur_len, fp);
   fclose(fp);
   return EINA_TRUE;
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
   const Elm_Genlist_Item_Class *itc = elm_genlist_item_item_class_get(glit);
   if (itc == _obj_kl_info_itc)
     {
        Eolian_Debug_Class *kl = elm_object_item_data_get(glit);
        Eolian_Debug_Function *func;
        Eina_List *itr;
        EINA_LIST_FOREACH(kl->functions, itr, func)
          {
            Elm_Genlist_Item *glist =  elm_genlist_item_append(
                   event->object, _obj_func_info_itc, func, glit,
                   ELM_GENLIST_ITEM_NONE, NULL, NULL);
            elm_genlist_item_tooltip_content_cb_set(glist, _obj_info_tootip, func, NULL);
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
_func_params_to_string(Eolian_Debug_Function *func, char *buffer, Eina_Bool full)
{
   Eina_List *itr;
   int buffer_size = 0;
   buffer_size += snprintf(buffer + buffer_size,
         _MAX_LABEL - buffer_size,  "%s:  ",
         eolian_function_name_get(func->efunc));
   buffer[0] = toupper(buffer[0]);

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
_obj_info_tootip(void *data,
              Evas_Object *obj EINA_UNUSED,
              Evas_Object *tt,
              void *item   EINA_UNUSED)
{
   Evas_Object *l = elm_label_add(tt);
   char buffer[_MAX_LABEL];
   _func_params_to_string(data, buffer, EINA_TRUE);
   elm_object_text_set(l, buffer);
   elm_label_line_wrap_set(l, ELM_WRAP_NONE);

   return l;
}

static char *
_obj_kl_info_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   Eolian_Debug_Class *kl = data;
   return strdup(eolian_class_full_name_get(kl->ekl));
}

static char *
_obj_func_info_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   char buffer[_MAX_LABEL];
   Eolian_Debug_Function *func = data;
   _func_params_to_string(func, buffer, EINA_FALSE);
   return strdup(buffer);
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

        Elm_Object_Item  *glg = elm_genlist_item_append(
              _main_widgets->object_infos_list, _obj_kl_info_itc,
              kl, NULL, type, NULL, NULL);
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
   if (_config->highlight)
     {
        eina_debug_session_send_to_thread(_session, _selected_app, info->thread_id,
              _obj_highlight_op, &(info->obj), sizeof(uint64_t));
     }
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
        it_data->glitem = elm_genlist_item_append(event->object, _objs_itc,
              it_data, glit,
              it_data->children ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
              _objs_sel_cb, NULL);
        elm_genlist_item_expanded_set(it_data->glitem, EINA_FALSE);
        efl_wref_add(it_data->glitem, &(it_data->glitem));
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

static Eina_Debug_Error
_win_screenshot_get(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED,
      void *buffer, int size)
{
   Evas_Debug_Screenshot *s = NULL;
   uint64_t id;
   s = evas_debug_screenshot_decode(buffer, size, &id);
   if (!s) return EINA_DEBUG_ERROR;
   Obj_Info *info =  eina_hash_find(_objs_hash, &id);
   if (!info) return EINA_DEBUG_OK;
   info->screenshots = eina_list_append(info->screenshots, s);
   if (info->glitem) elm_genlist_item_update(info->glitem);
   return EINA_DEBUG_OK;
}

void
take_screenshot_button_clicked(void *data EINA_UNUSED, const Efl_Event *event)
{
   Obj_Info *info = efl_key_data_get(event->object, "__info_node");

   eina_debug_session_send_to_thread(_session, _selected_app, info->thread_id,
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
   Obj_Info *info = efl_key_data_get(event->object, "__info_node");
   if (eina_list_count(info->screenshots) == 1)
     {
        _screenshot_display(eina_list_data_get(info->screenshots));
     }
   else
     {
        Eina_List *itr;
        Evas_Debug_Screenshot *s;
        int x = 0, y = 0, h = 0;
        if (!info->screenshots_menu)
          {
             info->screenshots_menu = elm_menu_add(_main_widgets->main_win);
             efl_wref_add(info->screenshots_menu, &info->screenshots_menu);
          }
        efl_gfx_position_get(event->object, &x, &y);
        efl_gfx_size_get(event->object, NULL, &h);
        elm_menu_move(info->screenshots_menu, x, y + h);
        efl_gfx_visible_set(info->screenshots_menu, EINA_TRUE);
        EINA_LIST_FOREACH(info->screenshots, itr, s)
          {
             char str[200];
             if (s->menu_item) continue;
             sprintf(str, "%.2d:%.2d:%.2d",
                   s->time.tm_hour, s->time.tm_min, s->time.tm_sec);
             s->menu_item = elm_menu_item_add(info->screenshots_menu,
                   NULL, NULL, str, _menu_screenshot_selected, s);
             efl_wref_add(s->menu_item, &s->menu_item);
          }
     }
}

static void
_ui_freeze(Eina_Bool on)
{
   elm_progressbar_pulse(_main_widgets->freeze_pulse, on);
   efl_gfx_visible_set(_main_widgets->freeze_pulse, on);
   efl_gfx_visible_set(_main_widgets->freeze_inwin, on);
}

static void *
_eoids_request_prepare(int *size)
{
   uint64_t obj_kl, canvas_kl;
   Class_Info *info = eina_hash_find(_classes_hash_by_name,
         _config->wdgs_show_type == 0 ? "Efl.Canvas.Object" : "Elm.Widget");
   if (info) obj_kl = info->id;
   info = eina_hash_find(_classes_hash_by_name, "Efl.Canvas");
   if (info) canvas_kl = info->id;
   return eo_debug_eoids_request_prepare(size, obj_kl, canvas_kl, NULL);
}

static Eina_Debug_Error
_snapshot_done_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED,
      void *buffer EINA_UNUSED, int size EINA_UNUSED)
{
   if (!_snapshot) return EINA_DEBUG_OK;
   _snapshot_save();
   free(_snapshot->buffer);
   free(_snapshot);
   _snapshot = NULL;
   _ui_freeze(EINA_FALSE);
   return EINA_DEBUG_OK;
}

void
snapshot_do(void *data EINA_UNUSED, Evas_Object *fs EINA_UNUSED, void *ev)
{
   void *buf;
   int size;
   _ui_freeze(EINA_TRUE);
   _snapshot = calloc(1, sizeof(*_snapshot));
   _snapshot->out_file = eina_stringshare_add(ev);
   /* EET app + opcodes */
   buf = _eoids_request_prepare(&size);
   eina_debug_session_send(_session, _selected_app, _snapshot_do_op, buf, size);
   free(buf);
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
   elm_check_state_set(wdgs->highlight_ck, _config->highlight);
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
   _config->highlight = elm_check_state_get(wdgs->highlight_ck);
   _config_save();
   efl_del(wdgs->win);
}

static Evas_Object *
_objs_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   static uint64_t canvas_id = 0;
   Obj_Info *info = data;
   if (!canvas_id)
     {
        Class_Info *kl_info = eina_hash_find(_classes_hash_by_name, "Evas.Canvas");
        if (kl_info) canvas_id = kl_info->id;
     }
   if(info->kl_id == canvas_id && !strcmp(part, "elm.swallow.end"))
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
           elm_box_pack_end(box, swdgs->bt);
        }

      Gui_Take_Screenshot_Button_Widgets *twdgs = gui_take_screenshot_button_create(box);
      elm_object_tooltip_text_set(twdgs->bt, "Take screenshot");
      efl_key_data_set(twdgs->bt, "__info_node", info);
      elm_box_pack_end(box, twdgs->bt);

      return box;
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
_app_objs_clean()
{
   if (_objs_list_tree)
     {
        _objs_tree_free(_objs_list_tree);
        _objs_list_tree = NULL;
     }
   eina_hash_free_buckets(_classes_hash_by_id);
   eina_hash_free_buckets(_classes_hash_by_name);

   elm_object_text_set(_main_widgets->apps_selector, "Select App");
   elm_genlist_clear(_main_widgets->objects_list);
   elm_genlist_clear(_main_widgets->object_infos_list);
   eina_hash_free_buckets(_objs_hash);
}

static void
_hoversel_selected_app(void *data,
      Evas_Object *obj, void *event_info)
{
   const char *label = elm_object_item_part_text_get(event_info, NULL);
   _app_objs_clean();

   _selected_app = (int)(long)data;
   elm_object_text_set(obj, label);
   eina_debug_session_send(_session, _selected_app, _module_init_op, "eo", 3);
}

static Eina_Debug_Error
_clients_info_added_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   while (size)
     {
        int cid, pid, len;
        EXTRACT(buf, &cid, sizeof(int));
        EXTRACT(buf, &pid, sizeof(int));
        if(pid != getpid())
          {
             char hs_name[100];
             App_Info *ai = _app_add(cid, pid, buf);

             snprintf(hs_name, 90, "%s [%d]", buf, pid);
             ai->hs_item = elm_hoversel_item_add(_main_widgets->apps_selector,
                   hs_name, "home", ELM_ICON_STANDARD, _hoversel_selected_app,
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
   _clouseau_init_done = EINA_FALSE;
}

static Eina_Debug_Error
_clients_info_deleted_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   if(size >= (int)sizeof(int))
     {
        int cid;
        EXTRACT(buf, &cid, sizeof(int));
        _app_del(cid);
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
   if (!strcmp(buffer, "clouseau")) _clouseau_init_done = ret;

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
   if (!_clouseau_init_done)
     {
        eina_debug_session_send(_session, _selected_app, _module_init_op, "clouseau", 9);
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
             efl_wref_add(info->glitem, &(info->glitem));
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
   free(data);
}

static Eina_Bool
_snapshot_is_candidate(void *buffer)
{
   Eina_Debug_Packet_Header *hdr = (Eina_Debug_Packet_Header *)buffer;
   if (hdr->opcode == _eoids_get_op ||
         hdr->opcode == _klids_get_op ||
         hdr->opcode == _obj_info_op ||
         hdr->opcode == _win_screenshot_op) return EINA_TRUE;
   else return EINA_FALSE;
}

Eina_Debug_Error
_disp_cb(Eina_Debug_Session *session EINA_UNUSED, void *buffer)
{
   if (_snapshot && _snapshot_is_candidate(buffer))
     {
        _snapshot_buffer_append(buffer);
     }
   else
     {
        ecore_main_loop_thread_safe_call_async(_ecore_thread_dispatcher, buffer);
     }
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
     {"Evas/object/highlight",  &_obj_highlight_op, NULL},
     {"Evas/window/screenshot", &_win_screenshot_op, &_win_screenshot_get},
     {"Eolian/object/info_get", &_obj_info_op, &_obj_info_get},
     {"Clouseau/snapshot_do",   &_snapshot_do_op, NULL},
     {"Clouseau/snapshot_done", &_snapshot_done_op, &_snapshot_done_cb},
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
new_profile_save_cb(void *data EINA_UNUSED, const Efl_Event *event)
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
   _app_objs_clean();
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

static void
_item_realize(Obj_Info *info)
{
   if (info->parent)
      {
         Obj_Info *pinfo =  eina_hash_find(_objs_hash, &(info->parent));
         if (pinfo && !pinfo->glitem) _item_realize(pinfo);
         elm_genlist_item_expanded_set(pinfo->glitem, EINA_TRUE);
      }
}

void
jump_entry_changed(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eo *en = event->object;
   const char *ptr = elm_entry_entry_get(en);
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
   if (!err)
     {
        Obj_Info *info =  eina_hash_find(_objs_hash, &id);
        if (!info) return;
        if (!info->glitem) _item_realize(info);
        elm_genlist_item_show(info->glitem, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
        elm_genlist_item_selected_set(info->glitem, EINA_TRUE);
     }
}

void
load_perform(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   switch (_conn_type)
     {
      case OFFLINE:
           {
              /* FIXME open file selector */
              break;
           }
      case LOCAL_CONNECTION:
      case REMOTE_CONNECTION:
           {
              _app_objs_clean();
              eina_debug_session_send(_session, _selected_app, _klids_get_op, NULL, 0);
              break;
           }
      default: break;
     }
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
     }
   efl_event_callback_add(_main_widgets->objects_list, ELM_GENLIST_EVENT_EXPAND_REQUEST, _objs_expand_request_cb, NULL);
   efl_event_callback_add(_main_widgets->objects_list, ELM_GENLIST_EVENT_CONTRACT_REQUEST, _objs_contract_request_cb, NULL);
   efl_event_callback_add(_main_widgets->objects_list, ELM_GENLIST_EVENT_EXPANDED, _objs_expanded_cb, NULL);
   efl_event_callback_add(_main_widgets->objects_list, ELM_GENLIST_EVENT_CONTRACTED, _objs_contracted_cb, NULL);

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
