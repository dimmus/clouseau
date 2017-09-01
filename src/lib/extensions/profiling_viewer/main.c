#include <Eina.h>
#include <Elementary.h>
#include <Efl_Profiler.h>

#include "../../Clouseau.h"

#define _EET_ENTRY "config"


typedef enum  {
     STREAM_STOPPED = 0,
     STREAM_PROCESSING,
     STREAM_PAUSED
} CLOUSEAU_PROFILER_STATUS;

typedef struct {
   Evas_Object *profiler;
   Ecore_Timer *record_get_timer;
   struct {
        CLOUSEAU_PROFILER_STATUS status; /*<< 0 - stopped, 1 - processing, 2 - paused */
   } stream;
} Inf;

static int _clouseau_profiling_extension_log_dom  = 0;

static int _record_on_op = EINA_DEBUG_OPCODE_INVALID;
static int _record_off_op = EINA_DEBUG_OPCODE_INVALID;
static int _record_get_op = EINA_DEBUG_OPCODE_INVALID;

static Eina_Bool _record_get_cb(Eina_Debug_Session *, int, void *, int);

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
_stream_processing_pause_cb(void *data,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Clouseau_Extension *ext = data;
   Inf *inf = ext->data;

   eina_debug_session_send(ext->session, ext->app_id, _record_off_op, NULL, 0);
   ecore_timer_del(inf->record_get_timer);
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
   Inf *inf = ext->data;
   int i = 0;
   Eina_Debug_Opcode *ops = _ops();

   switch (inf->stream.status)
     {
      case STREAM_PROCESSING:
         evas_object_smart_callback_call(inf->profiler, "stream,processing,pause", NULL);
         break;
      case STREAM_PAUSED:
      case STREAM_STOPPED:
      default:
         evas_object_smart_callback_call(inf->profiler, "stream,processing,resume", NULL);

     }
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

   return;
}

static void
_app_changed(Clouseau_Extension *ext)
{
   Inf *inf = ext->data;
   evas_object_smart_callback_call(inf->profiler, "stream,processing,stop", &ext->app_id);
   EINA_LOG_DOM_DBG(_clouseau_profiling_extension_log_dom, "Application changed");
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
   _stream_processing_resume_cb(ext,NULL, NULL);
   return;
}

static Eina_Bool
_record_get_cb(Eina_Debug_Session *session, int cid EINA_UNUSED, void *buffer, int size)
{
   Clouseau_Extension *ext = eina_debug_session_data_get(session);
   _profiling_import(ext, buffer, size, -1);
   return EINA_TRUE;
}
static Eo *
_ui_get(Clouseau_Extension *ext, Eo *parent)
{
   Inf *inf = ext->data;
   inf->profiler  = efl_profiling_viewer_init(parent);
   evas_object_size_hint_weight_set(inf->profiler, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(inf->profiler, EVAS_HINT_FILL, EVAS_HINT_FILL);

   evas_object_smart_callback_add(inf->profiler, "stream,processing,pause",
                                  _stream_processing_pause_cb, ext);
   evas_object_smart_callback_add(inf->profiler, "stream,processing,resume",
                                  _stream_processing_resume_cb, ext);

   return inf->profiler;
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
   efl_del(ext->ui_object);

   free(inf);

   EINA_LOG_DOM_DBG(_clouseau_profiling_extension_log_dom, "Extension stopped");
   eina_log_domain_unregister(_clouseau_profiling_extension_log_dom);
   eina_shutdown();

   return EINA_TRUE;
}
