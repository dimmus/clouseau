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
#include <Ecore_File.h>
#include "gui.h"
#include "Clouseau.h"

#define EXTRACT(_buf, pval, sz) \
{ \
   memcpy(pval, _buf, sz); \
   _buf += sz; \
}

#define _EET_ENTRY "config"

static int _cl_stat_reg_op = EINA_DEBUG_OPCODE_INVALID;

static Gui_Main_Win_Widgets *_main_widgets = NULL;

typedef struct
{
   int cid;
   int pid;
   Eina_Stringshare *name;
   Eo *menu_item;
} App_Info;

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
   const char *name;
   const char *command;
   /* Not eet */
   const char *file_name;
   Eo *menu_item;
} Profile;

typedef struct
{
   Eina_Debug_Session *session;
   void *buffer;
} Dispatcher_Info;

typedef Eina_Bool (*Ext_Start_Cb)(Clouseau_Extension *, Eo *);
typedef Eina_Bool (*Ext_Stop_Cb)(Clouseau_Extension *);

struct _Extension_Config
{
   const char *lib_path;
   Eina_Module *module;
   const char *name;
   Ext_Start_Cb start_fn;
   Ext_Stop_Cb stop_fn;
   Eina_Bool ready : 1;
};

typedef struct
{
   Eina_List *extensions_cfgs;
} Config;

static Connection_Type _conn_type = OFFLINE;
static Eina_Debug_Session *_session = NULL;
static Eina_List *_apps = NULL;
static App_Info *_selected_app = NULL;

static Eet_Data_Descriptor *_profile_edd = NULL, *_config_edd = NULL;
static Eina_List *_profiles = NULL;
static Config *_config = NULL;
static Eina_List *_extensions = NULL;

static Eo *_menu_remote_item = NULL;
static Profile *_selected_profile = NULL;

static Eina_Debug_Error _clients_info_added_cb(Eina_Debug_Session *, int, void *, int);
static Eina_Debug_Error _clients_info_deleted_cb(Eina_Debug_Session *, int, void *, int);

static const Eina_Debug_Opcode _ops[] =
{
     {"daemon/observer/client/register", &_cl_stat_reg_op, NULL},
     {"daemon/observer/slave_added", NULL, _clients_info_added_cb},
     {"daemon/observer/slave_deleted", NULL, _clients_info_deleted_cb},
     {NULL, NULL, NULL}
};

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
   Eet_Data_Descriptor *ext_edd;
   if (_config_edd) return;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Extension_Config);
   ext_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(ext_edd, Extension_Config, "lib_path", lib_path, EET_T_STRING);

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Config);
   _config_edd = eet_data_descriptor_stream_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_LIST(_config_edd, Config, "extensions_cfgs", extensions_cfgs, ext_edd);
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
_profile_remove(Profile *p)
{
   char path[1024];
   if (!p) return;
   if (p->menu_item) efl_del(p->menu_item);
   sprintf(path, "%s/clouseau/profiles/%s", efreet_config_home_get(), p->file_name);
   if (remove(path) == -1) perror("Remove");
   _profiles = eina_list_remove(_profiles, p);
   eina_stringshare_del(p->file_name);
   eina_stringshare_del(p->name);
   eina_stringshare_del(p->command);
   free(p);
}

static Eo *
_inwin_create(void)
{
   return elm_win_inwin_add(_main_widgets->main_win);
}

static void
_ui_freeze(Clouseau_Extension *ext EINA_UNUSED, Eina_Bool on)
{
   elm_progressbar_pulse(_main_widgets->freeze_pulse, on);
   efl_gfx_visible_set(_main_widgets->freeze_pulse, on);
   efl_gfx_visible_set(_main_widgets->freeze_inwin, on);
}

static Extension_Config *
_ext_cfg_find_by_path(const char *path)
{
   Extension_Config *cfg;
   Eina_List *itr;
   EINA_LIST_FOREACH(_config->extensions_cfgs, itr, cfg)
     {
        if (!strcmp(cfg->lib_path, path)) return cfg;
     }
   return NULL;
}

static void
_configs_load()
{
   Extension_Config *ext_cfg;
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
     }
   else
     {
        _config = eet_data_read(file, _config_edd, _EET_ENTRY);
        eet_close(file);
     }

   snprintf(path, sizeof(path), INSTALL_PREFIX"/lib/libclouseau_objects_introspection.so");
   if (!_ext_cfg_find_by_path(path))
     {
        ext_cfg = calloc(1, sizeof(*ext_cfg));
        ext_cfg->lib_path = eina_stringshare_add(path);
        _config->extensions_cfgs = eina_list_append(_config->extensions_cfgs, ext_cfg);
     }

   EINA_LIST_FOREACH(_config->extensions_cfgs, itr, ext_cfg)
     {
        ext_cfg->module = eina_module_new(ext_cfg->lib_path);
        if (!ext_cfg->module || !eina_module_load(ext_cfg->module))
          {
             printf("Failed loading extension at path %s.\n", ext_cfg->lib_path);
             if (ext_cfg->module) eina_module_free(ext_cfg->module);
             ext_cfg->module = NULL;
             continue;
          }
        const char *(*name_fn)(void) = eina_module_symbol_get(ext_cfg->module, "extension_name_get");
        if (!name_fn)
          {
             printf("Can not find extension_name_get function for %s\n", ext_cfg->lib_path);
             ext_cfg->name = ext_cfg->lib_path;
             continue;
          }
        ext_cfg->name = name_fn();
        Ext_Start_Cb start_fn = eina_module_symbol_get(ext_cfg->module, "extension_start");
        if (!start_fn)
          {
             printf("Can not find extension_start function for %s\n", ext_cfg->name);
             continue;
          }
        ext_cfg->start_fn = start_fn;
        Ext_Stop_Cb stop_fn = eina_module_symbol_get(ext_cfg->module, "extension_stop");
        if (!stop_fn)
          {
             printf("Can not find extension_stop function for %s\n", ext_cfg->name);
             continue;
          }
        ext_cfg->stop_fn = stop_fn;
        ext_cfg->ready = EINA_TRUE;
     }
   _config_save();
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
   if (ai->menu_item) efl_del(ai->menu_item);
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
_apps_free()
{
   Eina_List *itr, *itr2;
   App_Info *ai;
   EINA_LIST_FOREACH_SAFE(_apps, itr, itr2, ai)
     {
        _app_del(ai->cid);
     }
}

static void
_app_populate()
{
   Eina_List *itr;
   Clouseau_Extension *ext;
   int sel_app_id = _selected_app?_selected_app->cid:0;
   EINA_LIST_FOREACH(_extensions, itr, ext)
     {
        if (ext->app_id != sel_app_id)
          {
             ext->app_id = sel_app_id;
             ext->app_changed_cb(ext);
          }
     }
}

static void
_menu_selected_app(void *data,
      Evas_Object *obj, void *event_info)
{
   Eina_List *itr;
   Clouseau_Extension *ext;
   const char *label = elm_object_item_part_text_get(event_info, NULL);

   _selected_app = data;
   elm_object_text_set(obj, label);

   EINA_LIST_FOREACH(_extensions, itr, ext) _app_populate();
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
             char name[100];
             App_Info *ai = _app_add(cid, pid, buf);
             if (!ai->menu_item)
               {
                  snprintf(name, 90, "%s [%d]", buf, pid);
                  ai->menu_item = elm_menu_item_add(_main_widgets->apps_selector_menu,
                        NULL, "home", name, _menu_selected_app, ai);
                  efl_wref_add(ai->menu_item, &ai->menu_item);
               }
          }
        len = strlen(buf) + 1;
        buf += len;
        size -= (2 * sizeof(int) + len);
     }
   return EINA_DEBUG_OK;
}

static Eina_Debug_Error
_clients_info_deleted_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   if(size >= (int)sizeof(int))
     {
        int cid;
        EXTRACT(buf, &cid, sizeof(int));
        if (_selected_app && cid == _selected_app->cid) _selected_app = NULL;
        _app_del(cid);
     }
   return EINA_DEBUG_OK;
}

static void
_ecore_thread_dispatcher(void *data)
{
   Dispatcher_Info *info = data;
   eina_debug_dispatch(info->session, info->buffer);
   free(info->buffer);
   free(data);
}

Eina_Debug_Error
_disp_cb(Eina_Debug_Session *session, void *buffer)
{
   Eina_Debug_Packet_Header *hdr = (Eina_Debug_Packet_Header *)buffer;
   if (hdr->cid && (!_selected_app || _selected_app->cid != hdr->cid))
     {
        free(buffer);
        return EINA_DEBUG_OK;
     }

   Dispatcher_Info *info = calloc(1, sizeof(*info));
   info->session = session;
   info->buffer = buffer;
   ecore_main_loop_thread_safe_call_async(_ecore_thread_dispatcher, info);
   return EINA_DEBUG_OK;
}

static void
_post_register_handle(void *data EINA_UNUSED, Eina_Bool flag)
{
   if(!flag) return;
   eina_debug_session_send(_session, 0, _cl_stat_reg_op, NULL, 0);
}

static void
_profile_create_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Gui_New_Profile_Win_Widgets *wdgs = NULL;
   Profile *p = NULL;
   Eo *save_bt = event->object;
   wdgs = efl_key_data_get(save_bt, "_wdgs");
   const char *name = elm_object_text_get(wdgs->name_entry);
   const char *cmd = elm_entry_markup_to_utf8(elm_object_text_get(wdgs->command_entry));
   if (!name || !*name) return;
   if (!cmd || !*cmd) return;
   p = calloc(1, sizeof(*p));
   p->file_name = eina_stringshare_add(name); /* FIXME: Have to format name to conform to file names convention */
   p->name = eina_stringshare_add(name);
   p->command = eina_stringshare_add(cmd);
   _profile_save(p);
   efl_del(wdgs->inwin);
}

static void
_profile_modify_cb(void *data, const Efl_Event *event)
{
   Profile *p = data;
   Eo *save_bt = event->object;
   Gui_New_Profile_Win_Widgets *wdgs = efl_key_data_get(save_bt, "_wdgs");
   const char *name = elm_object_text_get(wdgs->name_entry);
   const char *cmd = elm_entry_markup_to_utf8(elm_object_text_get(wdgs->command_entry));
   if (!name || !*name) return;
   if (!cmd || !*cmd) return;
   _profile_remove(p);
   p = calloc(1, sizeof(*p));
   p->file_name = eina_stringshare_add(name); /* FIXME: Have to format name to conform to file names convention */
   p->name = eina_stringshare_add(name);
   p->command = eina_stringshare_add(cmd);
   _profile_save(p);
   efl_del(wdgs->inwin);
}

void
gui_new_profile_win_create_done(Gui_New_Profile_Win_Widgets *wdgs)
{
   efl_key_data_set(wdgs->save_button, "_wdgs", wdgs);
   efl_key_data_set(wdgs->cancel_button, "_wdgs", wdgs);
}

static void
_session_populate()
{
   Eina_List *itr;
   Clouseau_Extension *ext;
   EINA_LIST_FOREACH(_extensions, itr, ext)
     {
        if (ext->session) continue;
        switch (_conn_type)
          {
           case OFFLINE:
                {
                   if (ext->session_changed_cb) ext->session_changed_cb(ext);
                }
              break;
           case LOCAL_CONNECTION:
                {
                   ext->session = eina_debug_local_connect(EINA_TRUE);
                   eina_debug_session_dispatch_override(ext->session, _disp_cb);
                   if (ext->session_changed_cb) ext->session_changed_cb(ext);
                   break;
                }
           case REMOTE_CONNECTION:
                {
                   ext->session = eina_debug_shell_remote_connect(_selected_profile->command);
                   eina_debug_session_dispatch_override(ext->session, _disp_cb);
                   if (ext->session_changed_cb) ext->session_changed_cb(ext);
                   break;
                }
           default: return;
          }
     }
}

static void
_connection_type_change(Connection_Type conn_type)
{
   Eina_List *itr;
   Clouseau_Extension *ext;
   EINA_LIST_FOREACH(_extensions, itr, ext)
     {
        eina_debug_session_terminate(ext->session);
        ext->session = NULL;
        ext->app_id = 0;
     }
   eina_debug_session_terminate(_session);
   _apps_free();
   _cl_stat_reg_op = EINA_DEBUG_OPCODE_INVALID;
   elm_object_item_text_set(_main_widgets->apps_selector, "Select App");
   switch (conn_type)
     {
      case OFFLINE:
           {
              _selected_profile = NULL;
              elm_object_item_disabled_set(_main_widgets->apps_selector, EINA_TRUE);
              break;
           }
      case LOCAL_CONNECTION:
           {
              _selected_profile = NULL;
              elm_object_item_disabled_set(_main_widgets->apps_selector, EINA_FALSE);
              _session = eina_debug_local_connect(EINA_TRUE);
              eina_debug_session_dispatch_override(_session, _disp_cb);
              break;
           }
      case REMOTE_CONNECTION:
           {
              elm_object_item_disabled_set(_main_widgets->apps_selector, EINA_FALSE);
              _session = eina_debug_shell_remote_connect(_selected_profile->command);
              eina_debug_session_dispatch_override(_session, _disp_cb);
              break;
           }
      default: return;
     }
   if (_session) eina_debug_opcodes_register(_session, _ops, _post_register_handle, NULL);
   elm_object_item_text_set(_main_widgets->conn_selector, _conn_strs[conn_type]);
   _conn_type = conn_type;
   _session_populate();
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
_menu_profile_modify(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Profile *p = data;
   Gui_New_Profile_Win_Widgets *wdgs = gui_new_profile_win_create(_main_widgets->main_win);
   gui_new_profile_win_create_done(wdgs);
   efl_event_callback_add(wdgs->save_button, EFL_UI_EVENT_CLICKED, _profile_modify_cb, p);
   elm_object_text_set(wdgs->name_entry, p->name);
   elm_object_text_set(wdgs->command_entry, elm_entry_utf8_to_markup(p->command));
}

static void
_menu_profile_delete(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Profile *p = data;
   _profile_remove(p);
}

void
conn_menu_show(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eina_List *itr;
   Profile *p;

   EINA_LIST_FOREACH(_profiles, itr, p)
     {
        if (p->menu_item) continue;
        p->menu_item = elm_menu_item_add(_main_widgets->conn_selector_menu,
              _menu_remote_item, NULL, p->name,
              _menu_profile_selected, p);
        efl_wref_add(p->menu_item, &p->menu_item);
        elm_menu_item_add(_main_widgets->conn_selector_menu,
              p->menu_item, NULL, "Modify", _menu_profile_modify, p);
        elm_menu_item_add(_main_widgets->conn_selector_menu,
              p->menu_item, NULL, "Delete", _menu_profile_delete, p);
     }
}

static void
_profile_new_clicked(void *data EINA_UNUSED,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Gui_New_Profile_Win_Widgets *wdgs = gui_new_profile_win_create(_main_widgets->main_win);
   gui_new_profile_win_create_done(wdgs);
   efl_event_callback_add(wdgs->save_button, EFL_UI_EVENT_CLICKED, _profile_create_cb, NULL);
}

static void
_extension_delete(Clouseau_Extension *ext)
{
   Extension_Config *cfg = ext->ext_cfg;
   cfg->stop_fn(ext);
   _extensions = eina_list_remove(_extensions, ext);
   free(ext);
}

static void
_extension_instantiate(Extension_Config *cfg)
{
   Eina_List *itr, *itr2;
   Clouseau_Extension *ext;
   char path[1024];
   if (!cfg->ready) return;

   EINA_LIST_FOREACH_SAFE(_extensions, itr, itr2, ext) _extension_delete(ext);

   ext = calloc(1, sizeof(*ext));

   sprintf(path, "%s/clouseau/extensions", efreet_config_home_get());
   if (!_mkdir(path)) return;

   ext->path_to_config = eina_stringshare_add(path);
   ext->ext_cfg = cfg;
   ext->inwin_create_cb = _inwin_create;
   ext->ui_freeze_cb = _ui_freeze;
   if (!cfg->start_fn(ext, _main_widgets->main_win))
     {
        printf("Error in extension_init function of %s\n", cfg->name);
        free(ext);
        return;
     }
   _extensions = eina_list_append(_extensions, ext);

   elm_box_pack_end(_main_widgets->ext_box, ext->ui_object);

   _session_populate();
   _app_populate();
}

static void
_extension_view(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Extension_Config *cfg = data;
   _extension_instantiate(cfg);
}

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Connection_Type conn_type = OFFLINE;
   const char *offline_filename = NULL;
   Eina_List *itr;
   Extension_Config *ext_cfg;
   int i, long_index = 0, opt;
   Eina_Bool help = EINA_FALSE;

   eina_init();

   _configs_load();

   static struct option long_options[] =
     {
        /* These options set a flag. */
          {"help",      no_argument,        0, 'h'},
          {"local",     no_argument,        0, 'l'},
          {"remote",    required_argument,  0, 'r'},
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
        return 0;
     }

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   _main_widgets = gui_main_win_create(NULL);

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

   EINA_LIST_FOREACH(_config->extensions_cfgs, itr, ext_cfg)
     {
        Eo *it = elm_menu_item_add(_main_widgets->ext_selector_menu,
              NULL, NULL, ext_cfg->name, _extension_view, ext_cfg);
        if (!ext_cfg->ready) elm_object_item_disabled_set(it, EINA_TRUE);
     }
   if (eina_list_count(_config->extensions_cfgs) == 1)
      _extension_instantiate(eina_list_data_get(_config->extensions_cfgs));

   _connection_type_change(conn_type);

   elm_run();

   _connection_type_change(OFFLINE);
   eina_shutdown();
   return 0;
}
ELM_MAIN()
