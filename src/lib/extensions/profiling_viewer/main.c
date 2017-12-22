#include <Eina.h>
#include <Elementary.h>
#include <Efl_Profiler.h>

#include "../../Clouseau.h"

#define _EET_ENTRY "config"


typedef enum  {
     STREAM_PAUSED = 0,
     STREAM_PROCESSING
} CLOUSEAU_PROFILER_STATUS;

typedef struct {
   Evas_Object *profiler;
   Ecore_Timer *record_get_timer;
   CLOUSEAU_PROFILER_STATUS status; /*<< 0 - stopped, 1 - processing, 2 - paused */
   struct {
        Evas_Object *obj;
        Evas_Object *status_btn;
        Evas_Object *follow_btn;
        Evas_Object *filters_btn;
        Evas_Object *find_btn;
        Evas_Object *time_range_btn;
        Evas_Object *setting_btn;
   } toolbar;
   Eina_Bool follow;
   Eina_Bool block_processed;
} Inf;

static int _clouseau_profiling_extension_log_dom  = 0;

static int _record_on_op = EINA_DEBUG_OPCODE_INVALID;
static int _record_off_op = EINA_DEBUG_OPCODE_INVALID;
static int _record_get_op = EINA_DEBUG_OPCODE_INVALID;

static Eina_Bool _record_get_cb(Eina_Debug_Session *, int, void *, int);
static void _follow_interval_status_change_cb(void *data, Evas_Object *obj, void *event_info);

EINA_DEBUG_OPCODES_ARRAY_DEFINE(_ops,
       {"CPU/Freq/on", &_record_on_op, NULL},
       {"CPU/Freq/off", &_record_off_op, NULL},
       {"EvLog/get", &_record_get_op, &_record_get_cb},
       {NULL, NULL, NULL}
);

EAPI const char *
extension_name_get()
{
   return "Profiling viewer";
}

static Eina_Bool
_record_request_cb(void *data)
{
   Clouseau_Extension *ext = data;
   eina_debug_session_send(ext->session, ext->app_id, _record_get_op, NULL, 0);
   return EINA_TRUE;
}

static void
_stream_processing_resume_cb(void *data,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = data;
   Inf *inf = ext->data;

   eina_debug_session_send(ext->session, ext->app_id, _record_on_op, NULL, 0);
   if (!inf->record_get_timer)
     inf->record_get_timer = ecore_timer_add(0.2, _record_request_cb, ext);
}

static void
_session_changed(Clouseau_Extension *ext)
{
   int i = 0;
   Eina_Debug_Opcode *ops = _ops();
   Inf *inf = ext->data;

   EINA_LOG_DOM_DBG(_clouseau_profiling_extension_log_dom, "Session changed");

   while (ops[i].opcode_name)
     {
        if (ops[i].opcode_id) *(ops[i].opcode_id) = EINA_DEBUG_OPCODE_INVALID;
        i++;
     }

   if (ext->session)
     {
        eina_debug_session_data_set(ext->session, ext);
        eina_debug_opcodes_register(ext->session, ops, NULL, NULL);
     }

   /*disable controls on toolbar */
   elm_object_disabled_set(inf->toolbar.status_btn, EINA_TRUE);
   elm_object_disabled_set(inf->toolbar.follow_btn, EINA_TRUE);

   if (!inf->block_processed)
     {
        elm_object_disabled_set(inf->toolbar.filters_btn, EINA_TRUE);
        elm_object_disabled_set(inf->toolbar.time_range_btn, EINA_TRUE);
        elm_object_disabled_set(inf->toolbar.find_btn, EINA_TRUE);
     }

   return;
}

static void
_app_changed(Clouseau_Extension *ext)
{
   Inf *inf = ext->data;
   evas_object_smart_callback_call(inf->profiler, "stream,app,changed", &ext->app_id);
   EINA_LOG_DOM_DBG(_clouseau_profiling_extension_log_dom, "Application changed");

   /*enable controls on toolbar */
   elm_object_disabled_set(inf->toolbar.status_btn, EINA_FALSE);
   elm_object_disabled_set(inf->toolbar.follow_btn, EINA_FALSE);
   elm_object_disabled_set(inf->toolbar.filters_btn, EINA_TRUE);
   elm_object_disabled_set(inf->toolbar.time_range_btn, EINA_TRUE);
   elm_object_disabled_set(inf->toolbar.find_btn, EINA_TRUE);
   _follow_interval_status_change_cb(ext, inf->toolbar.follow_btn, NULL);

   inf->block_processed = EINA_FALSE;

   return;
}

static void
_profiling_import(Clouseau_Extension *ext,
                  void *buffer, int size,
                  int version EINA_UNUSED)
{
   EINA_LOG_DOM_DBG(_clouseau_profiling_extension_log_dom, "Data imported");
   Inf *inf = ext->data;

   Stream_Block_Data block_data = { .size = size, .data = buffer };

   evas_object_smart_callback_call(inf->profiler, "stream,block,process", &block_data);
   inf->block_processed = EINA_TRUE;
   _stream_processing_resume_cb(ext,NULL, NULL);
   if (inf->block_processed)
     {
        elm_object_disabled_set(inf->toolbar.filters_btn, EINA_FALSE);
        elm_object_disabled_set(inf->toolbar.time_range_btn, EINA_FALSE);
        elm_object_disabled_set(inf->toolbar.find_btn, EINA_FALSE);
     }

   return;
}

static Eina_Bool
_record_get_cb(Eina_Debug_Session *session, int cid EINA_UNUSED, void *buffer, int size)
{
   Clouseau_Extension *ext = eina_debug_session_data_get(session);
   _profiling_import(ext, buffer, size, -1);
   return EINA_TRUE;
}

static void
_find_dialog_show_cb(void *data,
                     Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = (Clouseau_Extension *)data;
   Inf *inf = ext->data;
   evas_object_smart_callback_call(inf->profiler, "find,show", NULL);
}

static void
_time_range_dialog_show_cb(void *data,
                           Evas_Object *obj EINA_UNUSED,
                           void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = (Clouseau_Extension *)data;
   Inf *inf = ext->data;
   evas_object_smart_callback_call(inf->profiler, "time,interval,win", NULL);
}

static void
_filters_dialog_show_cb(void *data,
                        Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = (Clouseau_Extension *)data;
   Inf *inf = ext->data;
   evas_object_smart_callback_call(inf->profiler, "filters,show", NULL);
}

static void
_unfollow_interval_cb(void *data,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = (Clouseau_Extension *)data;
   Inf *inf = ext->data;
   Evas_Object *icon = NULL;

   evas_object_smart_callback_call(inf->profiler, "unfollow,processed,data", NULL);
   inf->follow = EINA_FALSE;
   icon = elm_object_part_content_get(inf->toolbar.follow_btn, "icon");
   elm_icon_standard_set(icon, "go-last");
}

static void
_follow_interval_status_change_cb(void *data,
                                  Evas_Object *obj,
                                  void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = (Clouseau_Extension *)data;
   Inf *inf = ext->data;
   char *icon_name = NULL;
   Evas_Object *icon = NULL;

   if (inf->follow)
     {
        evas_object_smart_callback_call(inf->profiler, "unfollow,processed,data", NULL);
        inf->follow = EINA_FALSE;
        icon_name = "go-last";
     }
   else
     {
         evas_object_smart_callback_call(inf->profiler, "follow,processed,data", NULL);
         inf->follow = EINA_TRUE;
         icon_name = "go-bottom";
     }
   icon = elm_object_part_content_get(obj, "icon");
   elm_icon_standard_set(icon, icon_name);
}

static void
_profiling_status_change_cb(void *data,
                    Evas_Object *obj,
                    void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = (Clouseau_Extension *)data;
   Inf *inf = ext->data;
   char *icon_name = NULL;
   Evas_Object *icon = NULL;

   switch (inf->status)
     {
      case STREAM_PROCESSING:
        {
           eina_debug_session_send(ext->session, ext->app_id, _record_off_op, NULL, 0);
           ecore_timer_del(inf->record_get_timer);
           inf->status = STREAM_PAUSED;
           icon_name = "media-playback-start";
           break;
        }
      case STREAM_PAUSED:
        {
           eina_debug_session_send(ext->session, ext->app_id, _record_on_op, NULL, 0);
           inf->record_get_timer = NULL;
           _stream_processing_resume_cb(ext,NULL, NULL);
           inf->status = STREAM_PROCESSING;
           icon_name = "media-playback-pause";
           break;
        }
     }
   icon = elm_object_part_content_get(obj, "icon");
   elm_icon_standard_set(icon, icon_name);
}

static Eo *
_ui_get(Clouseau_Extension *ext, Eo *parent)
{
   Inf *inf = ext->data;
   Evas_Object *toolbar = NULL;
   Evas_Object *box = NULL;
   Evas_Object *button = NULL, *icon = NULL;
   Elm_Object_Item *item = NULL;

   box = elm_box_add(parent);
   evas_object_size_hint_weight_set(box, 1, 1);
   evas_object_size_hint_align_set(box, -1, -1);
   evas_object_show(box);

   toolbar = elm_toolbar_add(parent);
   inf->toolbar.obj = toolbar;
   elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_select_mode_set(toolbar, ELM_OBJECT_SELECT_MODE_NONE);
   elm_toolbar_homogeneous_set(toolbar, EINA_TRUE);
   elm_object_style_set(toolbar, "transparent");
   elm_toolbar_menu_parent_set(toolbar, parent);
   evas_object_size_hint_weight_set(toolbar, 0, 0);
   evas_object_size_hint_align_set(toolbar, -1, 0);
   evas_object_show(toolbar);

   item = elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL);
   button = elm_button_add(toolbar);
   elm_object_item_part_content_set(item, "object", button);
   evas_object_smart_callback_add(button, "clicked", _profiling_status_change_cb, ext);
   icon = elm_icon_add(button);
   elm_image_resizable_set(icon, EINA_FALSE, EINA_FALSE);
   elm_icon_standard_set(icon, "media-playback-start");
   elm_object_part_content_set(button, "icon", icon);
   inf->toolbar.status_btn = button;
   evas_object_show(button);
   elm_object_disabled_set(button, EINA_TRUE);

   item = elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL);
   button = elm_button_add(toolbar);
   elm_object_item_part_content_set(item, "object", button);
   evas_object_smart_callback_add(button, "clicked", _follow_interval_status_change_cb, ext);
   icon = elm_icon_add(button);
   elm_icon_standard_set(icon, "go-last");
   elm_object_part_content_set(button, "icon", icon);
   evas_object_show(button);
   inf->toolbar.follow_btn = button;
   elm_object_disabled_set(button, EINA_TRUE);

   item = elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL);
   button = elm_button_add(toolbar);
   elm_object_item_part_content_set(item, "object", button);
   evas_object_smart_callback_add(button, "clicked", _filters_dialog_show_cb, ext);
   icon = elm_icon_add(button);
   elm_icon_standard_set(icon, "view-list-details");
   elm_object_part_content_set(button, "icon", icon);
   evas_object_show(button);
   inf->toolbar.filters_btn = button;
   elm_object_disabled_set(button, EINA_TRUE);

   item = elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL);
   button = elm_button_add(toolbar);
   elm_object_item_part_content_set(item, "object", button);
   evas_object_smart_callback_add(button, "clicked", _find_dialog_show_cb, ext);
   icon = elm_icon_add(button);
   elm_icon_standard_set(icon, "system-search");
   elm_object_part_content_set(button, "icon", icon);
   evas_object_show(button);
   inf->toolbar.find_btn = button;

   item = elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL);
   button = elm_button_add(toolbar);
   elm_object_item_part_content_set(item, "object", button);
   evas_object_smart_callback_add(button, "clicked", _time_range_dialog_show_cb, ext);
   icon = elm_icon_add(button);
   elm_icon_standard_set(icon, "clock");
   elm_object_part_content_set(button, "icon", icon);
   evas_object_show(button);
   inf->toolbar.time_range_btn = button;
   elm_object_disabled_set(button, EINA_TRUE);

   item = elm_toolbar_item_append(toolbar, NULL, NULL, NULL, NULL);
   button = elm_button_add(toolbar);
   elm_object_item_part_content_set(item, "object", button);
   icon = elm_icon_add(button);
   elm_icon_standard_set(icon, "preferences-other");
   elm_object_part_content_set(button, "icon", icon);
   evas_object_show(button);
   inf->toolbar.setting_btn= button;
   elm_object_disabled_set(button, EINA_TRUE);


   inf->profiler = efl_profiling_viewer_init(parent);
   evas_object_size_hint_weight_set(inf->profiler, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(inf->profiler, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(inf->profiler, "unfollow", _unfollow_interval_cb, ext);

   elm_box_pack_end(box, toolbar);
   elm_box_pack_end(box, inf->profiler);

   return box;
}

EAPI Eina_Bool
extension_start(Clouseau_Extension *ext, Eo *parent)
{
   Inf *inf;

   eina_init();
   const char *log_dom = "clouseau_profiling_extension";
   _clouseau_profiling_extension_log_dom = eina_log_domain_register(log_dom, EINA_COLOR_ORANGE);
   if (_clouseau_profiling_extension_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: %s", log_dom);
        return EINA_FALSE;
     }

   EINA_LOG_DOM_DBG(_clouseau_profiling_extension_log_dom, "Extension started");

   inf = calloc(1, sizeof(Inf));
   ext->data = inf;
   ext->session_changed_cb = _session_changed;
   ext->app_changed_cb = _app_changed;
   ext->import_data_cb = _profiling_import;

   ext->ui_object = _ui_get(ext, parent);
   return !!ext->ui_object;
}

EAPI Eina_Bool
extension_stop(Clouseau_Extension *ext)
{
   Inf *inf = ext->data;

   evas_object_smart_callback_call(inf->profiler, "log,close", NULL);

   efl_profiling_viewer_shutdown(inf->profiler);
   free(inf);
   efl_del(ext->ui_object);

   EINA_LOG_DOM_DBG(_clouseau_profiling_extension_log_dom, "Extension stopped");
   eina_log_domain_unregister(_clouseau_profiling_extension_log_dom);
   eina_shutdown();

   return EINA_TRUE;
}
