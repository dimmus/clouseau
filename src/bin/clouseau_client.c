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
#include <Efl_Ui.h>
#include <Elementary.h>
#include <Evas.h>
#include <Ecore_File.h>
#include <Eo.h>
#include "gui.h"
#include "Clouseau.h"

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

#define EXTRACT(_buf, pval, sz) \
{ \
   memcpy(pval, _buf, sz); \
   _buf += sz; \
}

#define _EET_ENTRY "config"

#define _SNAPSHOT_EET_ENTRY "Clouseau_Snapshot"

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
   Eo *menu_item;
   Ext_Start_Cb start_fn;
   Ext_Stop_Cb stop_fn;
   Eina_Bool ready : 1;
};

typedef struct
{
   Eina_List *import_extensions_cfgs;
   Eina_List *extensions_cfgs; /* Imported + auto-loaded*/
   const char *last_extension_name;
} Config;

typedef struct
{
   const char *name;
   void *data;
   unsigned int data_count;
   int version;
} Extension_Snapshot;

typedef struct
{
   const char *app_name;
   int app_pid;
   Eina_List *ext_snapshots; /* List of Extension_Snapshot */
} Snapshot;

static Connection_Type _conn_type = OFFLINE;
static Eina_Debug_Session *_session = NULL;
static Eina_List *_apps = NULL;
static App_Info *_selected_app = NULL;

static Eet_Data_Descriptor *_config_edd = NULL, *_snapshot_edd = NULL;
static Config *_config = NULL;
static Eina_List *_extensions = NULL;

static int _selected_port = -1;
static int _arg_pid = -1;

static Eina_Bool _clients_info_added_cb(Eina_Debug_Session *, int, void *, int);
static Eina_Bool _clients_info_deleted_cb(Eina_Debug_Session *, int, void *, int);

static void _extension_view(void *, Evas_Object *, void *);
static void _fs_extension_import_show(void *, Evas_Object *, void *);

static void _connection_type_change(Connection_Type);

EINA_DEBUG_OPCODES_ARRAY_DEFINE(_ops,
     {"Daemon/Client/register_observer", &_cl_stat_reg_op, NULL},
     {"Daemon/Client/added", NULL, _clients_info_added_cb},
     {"Daemon/Client/deleted", NULL, _clients_info_deleted_cb},
     {NULL, NULL, NULL}
);

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

   EET_DATA_DESCRIPTOR_ADD_LIST(_config_edd, Config, "import_extensions_cfgs", import_extensions_cfgs, ext_edd);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_config_edd, Config, "last_extension_name",
         last_extension_name, EET_T_STRING);
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
_snapshot_eet_load()
{
   Eet_Data_Descriptor *ext_edd;
   if (_snapshot_edd) return;
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Extension_Snapshot);
   ext_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(ext_edd, Extension_Snapshot, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(ext_edd, Extension_Snapshot, "data_count", data_count, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(ext_edd, Extension_Snapshot, "version", version, EET_T_INT);

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Snapshot);
   _snapshot_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_snapshot_edd, Snapshot, "app_name", app_name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_snapshot_edd, Snapshot, "app_pid", app_pid, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_LIST(_snapshot_edd, Snapshot, "ext_snapshots", ext_snapshots, ext_edd);
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
   efl_gfx_entity_visible_set(_main_widgets->freeze_pulse, on);
   efl_gfx_entity_visible_set(_main_widgets->freeze_inwin, on);
}

static Extension_Config *
_ext_cfg_find_by_path(const char *path)
{
   Extension_Config *cfg;
   Eina_List *itr;
   EINA_LIST_FOREACH(_config->extensions_cfgs, itr, cfg)
     {
        if (cfg->lib_path && !strcmp(cfg->lib_path, path)) return cfg;
     }
   return NULL;
}

static Extension_Config *
_ext_cfg_find_by_name(const char *name)
{
   Extension_Config *cfg;
   Eina_List *itr;
   EINA_LIST_FOREACH(_config->extensions_cfgs, itr, cfg)
     {
        if (cfg->name && !strcmp(cfg->name, name)) return cfg;
     }
   return NULL;
}

static void
_extension_configs_validate()
{
   Extension_Config *ext_cfg;
   Eina_List *itr;
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
   EINA_LIST_FOREACH(_config->extensions_cfgs, itr, ext_cfg)
     {
        if (ext_cfg->menu_item || !ext_cfg->name) continue;
        ext_cfg->menu_item = elm_menu_item_add(_main_widgets->ext_selector_menu,
              NULL, NULL, ext_cfg->name, _extension_view, ext_cfg);
        elm_object_item_tooltip_text_set(ext_cfg->menu_item, ext_cfg->lib_path);
        efl_wref_add(ext_cfg->menu_item, &ext_cfg->menu_item);
        if (!ext_cfg->ready) elm_object_item_disabled_set(ext_cfg->menu_item, EINA_TRUE);
     }
}

static void
_ext_cfg_load(const char *name, const char *path, void *data EINA_UNUSED)
{
   if (strncmp(name, "libclouseau_", 12)) return;
   if (!eina_str_has_suffix(name, ".so")) return;

   if (!_ext_cfg_find_by_path(path))
     {
        Extension_Config *ext_cfg;
        char fname[1024];
        sprintf(fname, "%s/%s", path, name);
        ext_cfg = calloc(1, sizeof(*ext_cfg));
        ext_cfg->lib_path = eina_stringshare_add(fname);
        _config->extensions_cfgs = eina_list_append(_config->extensions_cfgs, ext_cfg);
     }
}

static void
_configs_load()
{
   char path[1024];

   sprintf(path, "%s/clouseau", efreet_config_home_get());
   if (!_mkdir(path)) return;

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

   _config->extensions_cfgs = eina_list_clone(_config->import_extensions_cfgs);
   eina_file_dir_list(INSTALL_PREFIX"/lib", EINA_TRUE, _ext_cfg_load, NULL);

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
        if (!ext->app_changed_cb) continue;
        if (ext->app_id != sel_app_id)
          {
             ext->app_id = sel_app_id;
             ext->app_changed_cb(ext);
          }
     }
   elm_object_item_disabled_set(_main_widgets->save_load_bt, !sel_app_id);
}

static void
_menu_selected_app(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Eina_List *itr;
   Clouseau_Extension *ext;
   const char *label = elm_object_item_part_text_get(event_info, NULL);

   _selected_app = data;
   elm_object_item_text_set(_main_widgets->apps_selector, label);

   EINA_LIST_FOREACH(_extensions, itr, ext) _app_populate();
}

static Eina_Bool
_clients_info_added_cb(Eina_Debug_Session *session EINA_UNUSED, int src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   while (size > 0)
     {
        int cid, pid, len;
        EXTRACT(buf, &cid, sizeof(int));
        EXTRACT(buf, &pid, sizeof(int));
        cid = SWAP_32(cid);
        pid = SWAP_32(pid);
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
             if (_arg_pid == pid)
               {
                  _menu_selected_app(ai, _main_widgets->apps_selector_menu, ai->menu_item);
               }
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
        cid = SWAP_32(cid);
        if (_selected_app && cid == _selected_app->cid) _selected_app = NULL;
        _app_del(cid);
     }
   return EINA_TRUE;
}

static void
_ecore_thread_dispatcher(void *data)
{
   Dispatcher_Info *info = data;
   eina_debug_dispatch(info->session, info->buffer);
   free(info->buffer);
   free(data);
}

Eina_Bool
_disp_cb(Eina_Debug_Session *session, void *buffer)
{
   Eina_Debug_Packet_Header *hdr = (Eina_Debug_Packet_Header *)buffer;
   if (hdr->cid && (!_selected_app || _selected_app->cid != hdr->cid))
     {
        free(buffer);
        return EINA_TRUE;
     }

   Dispatcher_Info *info = calloc(1, sizeof(*info));
   info->session = session;
   info->buffer = buffer;
   ecore_main_loop_thread_safe_call_async(_ecore_thread_dispatcher, info);
   return EINA_TRUE;
}

static void
_post_register_handle(void *data EINA_UNUSED, Eina_Bool flag)
{
   if(!flag) return;
   eina_debug_session_send(_session, 0, _cl_stat_reg_op, NULL, 0);
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
                   break;
                }
           case LOCAL_CONNECTION:
                {
                   ext->session = eina_debug_local_connect(EINA_TRUE);
                   eina_debug_session_dispatch_override(ext->session, _disp_cb);
                   if (ext->session_changed_cb) ext->session_changed_cb(ext);
                   break;
                }
           case REMOTE_CONNECTION:
                {
                   ext->session = eina_debug_remote_connect(_selected_port);
                   eina_debug_session_dispatch_override(ext->session, _disp_cb);
                   if (ext->session_changed_cb) ext->session_changed_cb(ext);
                   break;
                }
           default: return;
          }
     }
}

static void
_dialog_cancel(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *win = data;
   efl_del(win);
}

static void
_dialog_box_create(const char *title, const char *content, const char *but1_str, const char *but2_str,
      Efl_Event_Cb cb1, Efl_Event_Cb cb2)
{
   Eo *win, *bx, *lb;

   win = efl_add(EFL_UI_WIN_CLASS, _main_widgets->main_win,
         efl_ui_win_type_set(efl_added, ELM_WIN_DIALOG_BASIC),
         efl_ui_win_autodel_set(efl_added, EINA_TRUE),
         efl_text_set(efl_added, title),
         efl_gfx_entity_visible_set(efl_added, EINA_TRUE));

   bx = efl_add(EFL_UI_BOX_CLASS, win,
         efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL),
         efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND),
         efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
         efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
   efl_content_set(win, bx);

   lb = efl_add(EFL_UI_TEXTBOX_CLASS, bx,
         efl_text_multiline_set(efl_added, EINA_TRUE),
         efl_text_interactive_editable_set(efl_added, EINA_FALSE),
         efl_text_horizontal_align_set(efl_added, 0.5),
         efl_text_set(efl_added, content),
         efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND),
         efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
         efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
   efl_pack(bx, lb);

   if (but1_str || but2_str)
     {
        Eo *bx2;
        bx2 = efl_add(EFL_UI_BOX_CLASS, win,
              efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
              efl_gfx_hint_weight_set(efl_added, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND),
              efl_gfx_hint_align_set(efl_added, EVAS_HINT_FILL, EVAS_HINT_FILL),
              efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
        efl_pack(bx, bx2);

        if (but1_str)
          {
             Eo *bt1 = efl_add(EFL_UI_BUTTON_CLASS, bx2,
                   efl_text_set(efl_added, but1_str),
                   efl_gfx_hint_weight_set(efl_added, 0, 0),
                   efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
             if (cb1) efl_event_callback_add(bt1, EFL_INPUT_EVENT_CLICKED, cb1, win);
             efl_pack(bx2, bt1);
          }
        if (but2_str)
          {
             Eo *bt2 = efl_add(EFL_UI_BUTTON_CLASS, bx2,
                   efl_text_set(efl_added, but2_str),
                   efl_gfx_hint_weight_set(efl_added, 0, 0),
                   efl_gfx_entity_visible_set(efl_added, EINA_TRUE));
             if (cb2) efl_event_callback_add(bt2, EFL_INPUT_EVENT_CLICKED, cb2, win);
             efl_pack(bx2, bt2);
          }
     }
}

static Eina_Bool
_local_reconnect(void *data EINA_UNUSED)
{
   _connection_type_change(LOCAL_CONNECTION);
   return ECORE_CALLBACK_CANCEL;
}

static void
_daemon_launch(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *win = data;
   ecore_exe_pipe_run("efl_debugd", ECORE_EXE_NONE, NULL);
   ecore_timer_add(1.0, _local_reconnect, NULL);
   efl_del(win);
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
   _session = NULL;
   _apps_free();
   _cl_stat_reg_op = EINA_DEBUG_OPCODE_INVALID;
   elm_object_item_text_set(_main_widgets->apps_selector, "Select App");
   switch (conn_type)
     {
      case OFFLINE:
           {
              _selected_port = -1;
              elm_object_item_disabled_set(_main_widgets->apps_selector, EINA_TRUE);
              break;
           }
      case LOCAL_CONNECTION:
           {
              _selected_port = -1;
              _session = eina_debug_local_connect(EINA_TRUE);
              if (!_session)
                {
                   _dialog_box_create("Can't connect",
                         "Clouseau cannot connect to the local daemon.\nDo you want Clouseau to launch it?",
                         "Launch", "Don't launch and stay offline",
                         _daemon_launch, _dialog_cancel);
                   _connection_type_change(OFFLINE);
                   return;
                }
              else
                {
                   elm_object_item_disabled_set(_main_widgets->apps_selector, EINA_FALSE);
                   eina_debug_session_dispatch_override(_session, _disp_cb);
                }
              break;
           }
      case REMOTE_CONNECTION:
           {
              elm_object_item_disabled_set(_main_widgets->apps_selector, EINA_FALSE);
              _session = eina_debug_remote_connect(_selected_port);
              if (!_session)
                {
                   _dialog_box_create("Can't connect",
                         "Clouseau cannot connect to the remote daemon.\nCheck it is running on the remote device and\nthat the port forwarding (e.g SSH) has been set.",
                         "Ok", NULL,
                         _dialog_cancel, NULL);
                   _connection_type_change(OFFLINE);
                   return;
                }
              else
                {
                   eina_debug_session_dispatch_override(_session, _disp_cb);
                }
              break;
           }
      default: return;
     }
   if (_session) eina_debug_opcodes_register(_session, _ops(), _post_register_handle, NULL);
   elm_object_item_text_set(_main_widgets->conn_selector, _conn_strs[conn_type]);
   _conn_type = conn_type;
   _session_populate();
   if (_session)
     {
        elm_object_item_text_set(_main_widgets->save_load_bt, "Save");
        elm_toolbar_item_icon_set(_main_widgets->save_load_bt, "document-import");
        elm_object_item_disabled_set(_main_widgets->save_load_bt, EINA_TRUE);
     }
   else
     {
        elm_object_item_text_set(_main_widgets->save_load_bt, "Load");
        elm_toolbar_item_icon_set(_main_widgets->save_load_bt, "document-export");
        elm_object_item_disabled_set(_main_widgets->save_load_bt, EINA_FALSE);
     }
}

void
remote_port_entry_changed(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Eo *inwin = data;
   const char *ptr = elm_entry_entry_get(obj);
   _selected_port = atoi(ptr);
   _connection_type_change(REMOTE_CONNECTION);
   efl_del(inwin);
}

static void
_menu_selected_conn(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Connection_Type ctype = (uintptr_t)data;
   if (ctype == REMOTE_CONNECTION)
      gui_remote_port_win_create(_main_widgets->main_win);
   else
      _connection_type_change(ctype);
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
_all_extensions_delete()
{
   Eina_List *itr, *itr2;
   Clouseau_Extension *ext;
   EINA_LIST_FOREACH_SAFE(_extensions, itr, itr2, ext) _extension_delete(ext);
}

static Clouseau_Extension *
_extension_instantiate(Extension_Config *cfg)
{
   Clouseau_Extension *ext;
   char path[1024];
   if (!cfg->ready) return NULL;

   ext = calloc(1, sizeof(*ext));

   sprintf(path, "%s/clouseau/extensions", efreet_config_home_get());
   if (!_mkdir(path))
     {
        free(ext);
        return NULL;
     }

   ext->path_to_config = eina_stringshare_add(path);
   ext->ext_cfg = cfg;
   ext->inwin_create_cb = _inwin_create;
   ext->ui_freeze_cb = _ui_freeze;
   if (!cfg->start_fn(ext, _main_widgets->main_win))
     {
        printf("Error in extension_init function of %s\n", cfg->name);
        free(ext);
        return NULL;
     }
   _extensions = eina_list_append(_extensions, ext);

   elm_box_pack_end(_main_widgets->ext_box, ext->ui_object);

   _session_populate();
   _app_populate();

   _config->last_extension_name = cfg->name;
   _config_save();
   return ext;
}

static void
_extension_view(void *data,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Extension_Config *cfg = data;
   _all_extensions_delete();
   _extension_instantiate(cfg);
}

static void
_fs_extension_import(void *data, Evas_Object *fs EINA_UNUSED, void *ev)
{
   const char *filename = ev;
   Eo *inwin = data;

   _config_eet_load();
   if (!_ext_cfg_find_by_path(filename))
     {
        Extension_Config *ext_cfg = calloc(1, sizeof(*ext_cfg));
        ext_cfg->lib_path = eina_stringshare_add(filename);
        _config->import_extensions_cfgs = eina_list_append(_config->import_extensions_cfgs, ext_cfg);
        _config->extensions_cfgs = eina_list_append(_config->extensions_cfgs, ext_cfg);
        _config_save();
        _extension_configs_validate();
     }

   efl_del(inwin);
}

static void
_fs_extension_import_show(void *data EINA_UNUSED,
      Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *inwin = _inwin_create();
   Eo *fs = elm_fileselector_add(inwin);

   elm_fileselector_is_save_set(fs, EINA_FALSE);
   elm_fileselector_path_set(fs, getenv("HOME"));
   evas_object_size_hint_weight_set
      (fs, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(fs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(fs, "done", _fs_extension_import, inwin);
   evas_object_show(fs);

   elm_win_inwin_content_set(inwin, fs);
   elm_win_inwin_activate(inwin);
}

static int
_file_get(const char *filename, char **buffer_out)
{
   char *file_data = NULL;
   int file_size;
   FILE *fp = fopen(filename, "r");
   if (!fp)
     {
        printf("Can not open file: \"%s\".\n", filename);
        return -1;
     }

   fseek(fp, 0, SEEK_END);
   file_size = ftell(fp);
   if (file_size <= 0)
     {
        fclose(fp);
        if (file_size < 0) printf("Can not ftell file: \"%s\".\n", filename);
        return -1;
     }
   rewind(fp);
   file_data = (char *) calloc(1, file_size);
   if (!file_data)
     {
        fclose(fp);
        printf("Calloc failed\n");
        return -1;
     }
   int res = fread(file_data, 1, file_size, fp);
   if (!res)
     {
        free(file_data);
        file_data = NULL;
        if (!feof(fp)) printf("fread failed\n");
     }
   fclose(fp);
   if (file_data)
     {
        if (buffer_out) *buffer_out = file_data;
        else free(file_data);
     }
   return file_size;
}

static void
_extension_file_import(void *data, Evas_Object *obj,
      void *event_info EINA_UNUSED)
{
   _all_extensions_delete();
   Extension_Config *cfg = data;
   Clouseau_Extension *ext = _extension_instantiate(cfg);
   char *buffer = NULL;
   int size = 0;
   const char *filename = efl_key_data_get(obj, "_filename");

   while (obj && strcmp(efl_class_name_get(obj), "Elm.Inwin"))
      obj = efl_parent_get(obj);
   if (obj) efl_del(obj);

   size = _file_get(filename, &buffer);
   if (size <= 0) return;

   _ui_freeze(ext, EINA_TRUE);
   if (ext->import_data_cb) ext->import_data_cb(ext, buffer, size, -1);
   _ui_freeze(ext, EINA_FALSE);
   free(buffer);
}

static void
_extensions_cfgs_inwin_create(const char *filename)
{
   Eina_List *itr;
   Extension_Config *ext_cfg;
   Eo *inwin = _inwin_create();
   elm_object_style_set(inwin, "minimal");

   Eo *box = elm_box_add(inwin);
   evas_object_size_hint_weight_set(box, 1, 1);
   evas_object_size_hint_align_set(box, -1, -1);
   efl_gfx_entity_visible_set(box, EINA_TRUE);

   Eo *label = elm_label_add(box);
   elm_object_text_set(label, "Choose an extension to open the file with:");
   evas_object_size_hint_align_set(label, 0, -1);
   evas_object_size_hint_weight_set(label, 1, 1);
   efl_gfx_entity_visible_set(label, EINA_TRUE);
   elm_box_pack_end(box, label);

   Eo *list = elm_list_add(inwin);
   elm_list_mode_set(list, ELM_LIST_EXPAND);
   evas_object_size_hint_weight_set(list, 1, 1);
   evas_object_size_hint_align_set(list, -1, -1);
   efl_key_data_set(list, "_filename", filename);
   EINA_LIST_FOREACH(_config->extensions_cfgs, itr, ext_cfg)
     {
        if (ext_cfg->ready)
          {
             elm_list_item_append(list, ext_cfg->name, NULL, NULL,
                   _extension_file_import, ext_cfg);
          }
     }
   evas_object_show(list);
   elm_box_pack_end(box, list);
   elm_win_inwin_content_set(inwin, box);
   elm_win_inwin_activate(inwin);
}

static void
_export_to_file(void *_data EINA_UNUSED, Evas_Object *fs EINA_UNUSED, void *ev)
{
   const char *filename = ev;
   _snapshot_eet_load();
   FILE *fp = fopen(filename, "w");
   if (fp)
     {
        Snapshot s;
        Extension_Snapshot *e_s;
        Clouseau_Extension *e;
        Eina_List *itr;
        char *eet_buf;
        int eet_size = 0;

        s.app_name = _selected_app->name;
        s.app_pid = _selected_app->pid;
        s.ext_snapshots = NULL;
        EINA_LIST_FOREACH(_extensions, itr, e)
          {
             if (e->export_data_cb)
               {
                  int data_count = 0;
                  int version = 1;
                  void *data = e->export_data_cb(e, &data_count, &version);
                  if (!data) continue;
                  e_s = alloca(sizeof(*e_s));
                  e_s->name = e->ext_cfg->name;
                  e_s->data = data;
                  e_s->data_count = data_count;
                  e_s->version = version;
                  s.ext_snapshots = eina_list_append(s.ext_snapshots, e_s);
               }
          }

        eet_buf = eet_data_descriptor_encode(_snapshot_edd, &s, &eet_size);
        fwrite(&eet_size, sizeof(int), 1, fp);
        fwrite(eet_buf, 1, eet_size, fp);

        EINA_LIST_FREE(s.ext_snapshots, e_s)
          {
             fwrite(e_s->data, 1, e_s->data_count, fp);
             free(e_s->data);
          }
        fclose(fp);
     }
}

static void
_file_import(void *_data EINA_UNUSED, Evas_Object *fs EINA_UNUSED, void *ev)
{
   const char *filename = ev;
   FILE *fp = fopen(filename, "r");
   void *eet_buf = NULL;
   Snapshot *s;
   unsigned int eet_size;
   if (!fp) return;
   _snapshot_eet_load();
   if (fread(&eet_size, sizeof(int), 1, fp) != 1) goto end;
   eet_buf = malloc(eet_size);
   if (fread(eet_buf, 1, eet_size, fp) != eet_size) goto end;
   s = eet_data_descriptor_decode(_snapshot_edd, eet_buf, eet_size);
   if (s)
     {
        Extension_Snapshot *e_s;
        char name[100];
        _all_extensions_delete();
        EINA_LIST_FREE(s->ext_snapshots, e_s)
          {
             void *data = malloc(e_s->data_count);
             if (fread(data, 1, e_s->data_count, fp) == e_s->data_count)
               {
                  Extension_Config *e_cfg = _ext_cfg_find_by_name(e_s->name);
                  if (e_cfg)
                    {
                       Clouseau_Extension *e = _extension_instantiate(e_cfg);
                       if (e->import_data_cb)
                          e->import_data_cb(e, data, e_s->data_count, e_s->version);
                    }
               }
             free(data);
             free(e_s);
          }
        snprintf(name, sizeof(name) - 1, "%s [%d]", s->app_name, s->app_pid);
        elm_object_item_text_set(_main_widgets->apps_selector, name);
        free(s);
     }
   else
      _extensions_cfgs_inwin_create(eina_stringshare_add(filename));
end:
   if (eet_buf) free(eet_buf);
   if (fp) fclose(fp);
}

static void
_inwin_del(void *data, Evas_Object *obj EINA_UNUSED, void *ev EINA_UNUSED)
{
   Eo *inwin = data;
   efl_del(inwin);
}

static void
_fs_activate(Eina_Bool is_save)
{
   Eo *inwin = _inwin_create();
   Eo *fs = elm_fileselector_add(inwin);

   elm_fileselector_is_save_set(fs, is_save);
   elm_fileselector_path_set(fs, getenv("HOME"));
   evas_object_size_hint_weight_set
      (fs, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(fs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(fs, "done", _inwin_del, inwin);
   evas_object_smart_callback_add(fs, "done",
         is_save?_export_to_file:_file_import, NULL);
   evas_object_show(fs);

   elm_win_inwin_content_set(inwin, fs);
   elm_win_inwin_activate(inwin);
}

void
save_load_perform(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   if (_session) _fs_activate(EINA_TRUE);
   else _fs_activate(EINA_FALSE);
}

static void
_main_window_del(void *data EINA_UNUSED,
                 Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   _all_extensions_delete();
}

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Connection_Type conn_type = OFFLINE;
   Extension_Config *ext_cfg;
   Eina_Stringshare *offline_filename = NULL;
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
          {"pid",       required_argument,  0, 'p'},
          {"file",      required_argument,  0, 'f'},
          {0, 0, 0, 0}
     };
   while ((opt = getopt_long(argc, argv,"hlr:p:f:", long_options, &long_index )) != -1)
     {
        if (opt != 'p' && (conn_type != OFFLINE || offline_filename))
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
                      _selected_port = atoi(optarg);
                      break;
                   }
           case 'f':
                   {
                      conn_type = OFFLINE;
                      offline_filename = eina_stringshare_add(optarg);
                      break;
                   }
           case 'p':
                   {
                      _arg_pid = atoi(optarg);
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
        printf("       --remote/-r Create a remote connection by using the given port\n");
        printf("       --pid/-p PID of the program to connect to\n");
        printf("       --file/-f Run in offline mode and load the given file\n");
        return 0;
     }

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   _main_widgets = gui_main_win_create(NULL);
   evas_object_smart_callback_add(_main_widgets->main_win, "delete,request",
                                  _main_window_del, NULL);

   for (i = 0; i < LAST_CONNECTION; i++)
     {
        elm_menu_item_add(_main_widgets->conn_selector_menu,
              NULL, NULL, _conn_strs[i],
              _menu_selected_conn, (void *)(uintptr_t)i);
     }

   elm_menu_item_add(_main_widgets->ext_selector_menu,
         NULL, NULL, "Import ...", _fs_extension_import_show, NULL);
   _extension_configs_validate();

   if (!_config->last_extension_name)
     {
        ext_cfg = _ext_cfg_find_by_path(INSTALL_PREFIX"/lib/libclouseau_objects_introspection.so");
        if (ext_cfg) _config->last_extension_name = ext_cfg->name;
        _config_save();
     }
   else
     {
        ext_cfg = _ext_cfg_find_by_name(_config->last_extension_name);
     }
   if (ext_cfg) _extension_instantiate(ext_cfg);

   _connection_type_change(conn_type);

   if (conn_type == OFFLINE && offline_filename)
      _file_import(NULL, NULL, (void *)offline_filename);

   elm_run();

   eina_shutdown();
   return 0;
}
ELM_MAIN()
