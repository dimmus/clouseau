#ifndef EFL_BETA_API_SUPPORT
#define EFL_BETA_API_SUPPORT
#endif
#ifndef EFL_EO_API_SUPPORT
#define EFL_EO_API_SUPPORT
#endif
#ifndef ELM_INTERNAL_API_ARGESFSDFEFC
#define ELM_INTERNAL_API_ARGESFSDFEFC
#endif
#include <Elementary.h>
#include <Evas.h>
#include "gui.h"

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

#define EXTRACT(_buf, pval, sz) \
{ \
   memcpy(pval, _buf, sz); \
   _buf += sz; \
}

static uint32_t _cl_stat_reg_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _cid_from_pid_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _poll_on_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _poll_off_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _evlog_on_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _evlog_off_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _eo_list_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _elm_list_opcode = EINA_DEBUG_OPCODE_INVALID;

static Gui_Widgets *pub_widgets = NULL;

typedef struct
{
   uint32_t *opcode; /* address to the opcode */
   void *buffer;
   int size;
} _pending_request;

static Eina_List *_pending = NULL;
static Eina_Debug_Session *_session = NULL;

static uint32_t _cid = 0;

static int my_argc = 0;
static char **my_argv = NULL;
static int selected_app = -1;
static Elm_Genlist_Item_Class *_itc = NULL;
Eina_List *objs = NULL;

static char *
_item_label_get(void *data, Evas_Object *obj, const char *part)
{
   Obj_Info *info = data;
   return strdup(info->kl_name);
}

static void
_hoversel_selected_app(void *data, Evas_Object *obj, void *event_info)
{
        Elm_Object_Item *hoversel_it = event_info;
        selected_app = (int)(long)elm_object_item_data_get(hoversel_it);
        printf("selected app %d\n", selected_app);

        if(objs)
          {
             Obj_Info *data;
             EINA_LIST_FREE(objs, data)
                free(data);
             objs = NULL;
             elm_genlist_clear(pub_widgets->elm_win1->elm_genlist1);
          }

        Eina_Debug_Client *cl = eina_debug_client_new(_session, selected_app);
        eina_debug_session_send(cl, _elm_list_opcode, NULL, 0);
        eina_debug_client_free(cl);
}

static void
_consume()
{
   if (!_pending)
     {
        return;
     }
   _pending_request *req = eina_list_data_get(_pending);
   _pending = eina_list_remove(_pending, req);

   Eina_Debug_Client *cl = eina_debug_client_new(_session, _cid);
   eina_debug_session_send(cl, *(req->opcode), req->buffer, req->size);
   eina_debug_client_free(cl);

   free(req->buffer);
   free(req);
}

static void
_pending_add(uint32_t *opcode, void *buffer, int size)
{
   _pending_request *req = malloc(sizeof(*req));
   req->opcode = opcode;
   req->buffer = buffer;
   req->size = size;
   _pending = eina_list_append(_pending, req);
}

static Eina_Bool
_cid_get_cb(Eina_Debug_Client *src EINA_UNUSED, void *buffer, int size EINA_UNUSED)
{
   _cid = *(uint32_t *)buffer;
   _consume();
   return EINA_TRUE;
}

static Eina_Bool
_clients_info_cb(Eina_Debug_Client *src EINA_UNUSED, void *buffer, int size)
{
   char *buf = buffer;
   while(size)
     {
        int cid, pid, len;
        EXTRACT(buf, &cid, sizeof(uint32_t));
        EXTRACT(buf, &pid, sizeof(uint32_t));
        if(pid != getpid())
          {
             printf("CID: %d - PID: %d - Name: %s\n", cid, pid, buf);
             char option[100];
             snprintf(option, 90, "CID: %d - PID: %d - Name: %s", cid, pid, buf);
             printf("%s\n", option);
             elm_hoversel_item_add(pub_widgets->elm_win1->elm_hoversel1, option, "home", ELM_ICON_STANDARD, _hoversel_selected_app,
                   (const void *)(long)cid);
          }
        len = strlen(buf) + 1;
        buf += len;
        size -= (2 * sizeof(uint32_t) + len);
     }
   _consume();
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
        printf("CID: %d deleted\n", cid);

        const Eina_List *items = elm_hoversel_items_get(pub_widgets->elm_win1->elm_hoversel1);
        const Eina_List *l;
        Elm_Object_Item *hoversel_it;

        EINA_LIST_FOREACH(items, l, hoversel_it)
           if((int)(long)elm_object_item_data_get(hoversel_it) == cid)
             {
                elm_object_item_del(hoversel_it);
                break;
             }
     }
   _consume();
   return EINA_TRUE;
}

static Eina_Bool
_objects_list_cb(Eina_Debug_Client *src EINA_UNUSED, void *buffer, int size)
{
   Eina_List *objs = eo_debug_list_response_decode(buffer, size), *itr;
   Obj_Info *info;
   EINA_LIST_FOREACH(objs, itr, info)
     {
        printf("%p: %s\n", info->ptr, info->kl_name);
     }
   return EINA_TRUE;
}

static Eina_Bool
_elm_objects_list_cb(Eina_Debug_Client *src EINA_UNUSED, void *buffer, int size)
{
   printf("size=%d\n", size);
   Eina_List *itr;
   objs = eo_debug_list_response_decode(buffer, size);
   Obj_Info *info;
   EINA_LIST_FOREACH(objs, itr, info)
     {
        printf("%p: %s\n", info->ptr, info->kl_name);

        elm_genlist_item_append(pub_widgets->elm_win1->elm_genlist1, _itc,
              (void *)info, NULL,
              ELM_GENLIST_ITEM_NONE,
              NULL, NULL);
     }

   return EINA_TRUE;
}

static void
_ecore_thread_dispatcher(void *data)
{
   eina_debug_dispatch(_session, data);
}

Eina_Bool
_disp_cb(Eina_Debug_Session *session, void *buffer)
{
   ecore_main_loop_thread_safe_call_async(_ecore_thread_dispatcher, buffer);
   return EINA_TRUE;
}

static void
_args_handle(Eina_Bool flag)
{
   int i;
   if(!flag) return;
   eina_debug_session_dispatch_override(_session, _disp_cb);
   Eina_Debug_Client *cl = eina_debug_client_new(_session, 0);
   eina_debug_session_send(cl, _cl_stat_reg_opcode, NULL, 0);

   for (i = 1; i < my_argc;)
     {
        if (i < my_argc - 1)
          {
             const char *op_str = my_argv[i++];
             uint32_t pid = atoi(my_argv[i++]);
             char *buf = NULL;
             eina_debug_session_send(cl, _cid_from_pid_opcode, &pid, sizeof(uint32_t));
             printf("got %s %d\n", op_str, pid);
             if ((!strcmp(op_str, "pon")) && (i < (my_argc - 2)))
               {
                  uint32_t freq = atoi(my_argv[i++]);
                  buf = malloc(sizeof(uint32_t));
                  memcpy(buf, &freq, sizeof(uint32_t));
                  _pending_add(&_poll_on_opcode, buf, sizeof(uint32_t));
               }
             else if (!strcmp(op_str, "poff"))
                _pending_add(&_poll_off_opcode, NULL, 0);
             else if (!strcmp(op_str, "evlogon"))
                _pending_add(&_evlog_on_opcode, NULL, 0);
             else if (!strcmp(op_str, "evlogoff"))
                _pending_add(&_evlog_off_opcode, NULL, 0);
             else if (!strcmp(op_str, "eo_list"))
               {
                  if (i <= my_argc - 1) buf = strdup(my_argv[i++]);
                  _pending_add(&_eo_list_opcode, buf, buf ? strlen(buf) + 1 : 0);
               }
             else if (!strcmp(op_str, "elm_list"))
               {
                  if (i <= my_argc - 1) buf = strdup(my_argv[i++]);
                  _pending_add(&_elm_list_opcode, buf, buf ? strlen(buf) + 1 : 0);
               }
          }
     }
   eina_debug_client_free(cl);
}

static const Eina_Debug_Opcode ops[] =
{
     {"daemon/client_status_register", &_cl_stat_reg_opcode,  _clients_info_cb},
     {"daemon/client_added", NULL, _clients_info_cb},
     {"daemon/client_deleted", NULL, _clients_info_deleted_cb},
     {"daemon/cid_from_pid",  &_cid_from_pid_opcode,  &_cid_get_cb},
     {"poll/on",              &_poll_on_opcode,       NULL},
     {"poll/off",             &_poll_off_opcode,      NULL},
     {"evlog/on",             &_evlog_on_opcode,      NULL},
     {"evlog/off",            &_evlog_off_opcode,     NULL},
     {"Eo/list",              &_eo_list_opcode,       &_objects_list_cb},
     {"Elementary/objects_list",       &_elm_list_opcode,      &_elm_objects_list_cb},
     {NULL, NULL, NULL}
};

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   pub_widgets =  gui_gui_get();

   if (!_itc)
     {
        _itc = elm_genlist_item_class_new();
        _itc->item_style = "default";
        _itc->func.text_get = _item_label_get;
        _itc->func.content_get = NULL;
        _itc->func.state_get = NULL;
        _itc->func.del = NULL;
     }
   evas_object_size_hint_weight_set(pub_widgets->elm_win1->elm_genlist1, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   evas_object_show(pub_widgets->elm_win1->elm_genlist1);
   evas_object_show(pub_widgets->elm_win1->elm_win1);

   eina_init();

   _session = eina_debug_session_new();

   if (!eina_debug_local_connect(_session))
     {
        fprintf(stderr, "ERROR: Cannot connect to debug daemon.\n");
        goto error;
     }
   my_argc = argc;
   my_argv = argv;

   eina_debug_opcodes_register(_session, ops, _args_handle);

   elm_run();

error:
   eina_debug_session_free(_session);
   eina_shutdown();

   return 0;
}
ELM_MAIN()
