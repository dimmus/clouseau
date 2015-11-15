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
#include <Eolian.h>
#include "gui.h"

#include <Eolian_Debug.h>

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

#define EXTRACT(_buf, pval, sz) \
{ \
   memcpy(pval, _buf, sz); \
   _buf += sz; \
}

static uint32_t _cl_stat_reg_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _module_init_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _poll_on_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _poll_off_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _evlog_on_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _evlog_off_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _eo_list_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _elm_list_opcode = EINA_DEBUG_OPCODE_INVALID;
static uint32_t _obj_info_opcode = EINA_DEBUG_OPCODE_INVALID;

static Gui_Widgets *pub_widgets = NULL;

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
   CLOUSEAU_OBJ_PARAM
} Clouseau_Obj_Info_Type;

typedef struct
{
   Clouseau_Obj_Info_Type type;
   void *data;
} _Obj_info_node;

static Eina_List *_pending = NULL;
static Eina_Debug_Session *_session = NULL;
static int _selected_app = -1;
static Elm_Genlist_Item_Class *_objs_itc = NULL;
static Elm_Genlist_Item_Class *_obj_info_itc = NULL;
static Eina_List *_objs_list_tree = NULL;
static Eolian_Debug_Object_Information *obj_info = NULL;
static Eina_Debug_Client *_current_client = NULL;

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
_obj_info_expanded_cb(void *data EINA_UNUSED,
      Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED,
      void *event_info)
{
   Elm_Object_Item *glit = event_info;
   _Obj_info_node *node = elm_object_item_data_get(glit);
   Evas_Object *list = elm_object_item_widget_get(glit);
   Eolian_Debug_Class *kl;
   Eina_List *func_itr, *param_itr;
   Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_TREE;
   Eolian_Debug_Function *func;

   if(node->type == CLOUSEAU_OBJ_CLASS)
     {
        kl = (Eolian_Debug_Class *)(node->data);

        EINA_LIST_FOREACH(kl->functions, func_itr, func)
          {
             node = calloc(1, sizeof(*node));
             node->type = CLOUSEAU_OBJ_FUNC;
             node->data = func;

             Elm_Object_Item  *glg = elm_genlist_item_append(
                   list, _obj_info_itc,
                   (void *)node, glit,
                   type,
                   NULL, NULL);
            elm_genlist_item_expanded_set(glg, EINA_FALSE);
          }
     }
   else if(node->type == CLOUSEAU_OBJ_FUNC)
     {
        func =  (Eolian_Debug_Function *)(node->data);
        type = ELM_GENLIST_ITEM_NONE;
        Eolian_Debug_Parameter *param;
        EINA_LIST_FOREACH(func->params, param_itr, param)
          {
             node = calloc(1, sizeof(*node));
             node->type = CLOUSEAU_OBJ_PARAM;
             node->data = param;

             elm_genlist_item_append(
                   list, _obj_info_itc,
                   (void *)node, glit,
                   type,
                   NULL, NULL);
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

static void _obj_info_item_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   _Obj_info_node *node = data;
   free(node);
}

#define _MAX_LABEL 50
static char *_obj_info_item_label_get(void *data, Evas_Object *obj EINA_UNUSED,
      const char *part EINA_UNUSED)
{
   _Obj_info_node *node = data;
   char name[_MAX_LABEL]; name[0] = '\0';
   Eolian_Debug_Parameter *param;
   char *print_format;
   switch(node->type)
     {
      case CLOUSEAU_OBJ_CLASS : strncpy(name,
                                      ((Eolian_Debug_Class *)(node->data))->name, _MAX_LABEL);                                      break;
      case CLOUSEAU_OBJ_FUNC : strncpy(name,
                                     ((Eolian_Debug_Function *)(node->data))->name, _MAX_LABEL);
                               break;
      case CLOUSEAU_OBJ_PARAM :   param = node->data;
                                  if (param->type == EOLIAN_DEBUG_STRING)
                                     print_format = "%s";
                                  else
                                     print_format = "%lX";
                                  snprintf(name, _MAX_LABEL,
                                        print_format, param->value.value);
     }
   return (char *)(long)strdup(name);
}
#undef _MAX_LABEL

static Eina_Bool
_debug_obj_info_cb(Eina_Debug_Client *src EINA_UNUSED,
      void *buffer, int size)
{
   if(obj_info)
     {
        elm_genlist_clear(pub_widgets->elm_win1->elm_genlist2);
        eolian_debug_object_information_free(obj_info);
        obj_info = NULL;
     }
   obj_info = eolian_debug_object_information_decode(buffer, size);

   Eolian_Debug_Class *kl;
   Eina_List *kl_itr;
   EINA_LIST_FOREACH(obj_info->classes, kl_itr, kl)
     {
        Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_TREE;
        _Obj_info_node *node = NULL;
        node = calloc(1, sizeof(*node));
        node->type = CLOUSEAU_OBJ_CLASS;
        node->data = kl;

        Elm_Object_Item  *glg = elm_genlist_item_append(
              pub_widgets->elm_win1->elm_genlist2, _obj_info_itc,
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
   eina_debug_session_send(_current_client, _obj_info_opcode, &ptr, sizeof(uint64_t));
}

static Eina_Bool
_objs_expanded_cb(void *data EINA_UNUSED,
      Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED,
      void *event_info)
{
   Eina_List *l;
   Elm_Object_Item *glit = event_info;
   _Obj_list_node *info_node = elm_object_item_data_get(glit), *it_data;
   Evas_Object *list = elm_object_item_widget_get(glit);
   EINA_LIST_FOREACH(info_node->children, l, it_data)
     {
        Elm_Object_Item *nitem;
        Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_NONE;

        if (it_data->children)
          {
             type = ELM_GENLIST_ITEM_TREE;
          }

        nitem = elm_genlist_item_append(list, _objs_itc, it_data, glit,
                                        type, _objs_sel_cb, NULL);
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
   return (char *)(long)strdup(info_node->info->kl_name);
}

static void
_objs_nodes_free(Eina_List *parents)
{
  _Obj_list_node *info_node;

   EINA_LIST_FREE(parents, info_node)
     {
        free(info_node->info);
        _objs_nodes_free(info_node->children);
        free(info_node);
     }
}

static void
_hoversel_selected_app(void *data EINA_UNUSED,
      Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *hoversel_it = event_info;
   _selected_app = (int)(long)elm_object_item_data_get(hoversel_it);

   if(_objs_list_tree)
     {
        _objs_nodes_free(_objs_list_tree);
        _objs_list_tree = NULL;
        elm_genlist_clear(pub_widgets->elm_win1->elm_genlist1);
     }

   if (_current_client) eina_debug_client_free(_current_client);
   _current_client = eina_debug_client_new(_session, _selected_app);
   eina_debug_session_send(_current_client, _module_init_opcode, "elementary", 11);
   eina_debug_session_send(_current_client, _module_init_opcode, "eolian", 7);
   _pending_add(&_elm_list_opcode, NULL, 0);
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
             char option[100];
             snprintf(option, 90, "CID: %d - PID: %d - Name: %s", cid, pid, buf);
             elm_hoversel_item_add(pub_widgets->elm_win1->elm_hoversel1,
                   option, "home", ELM_ICON_STANDARD, _hoversel_selected_app,
                   (const void *)(long)cid);
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
        eina_hash_add(objects_hash, &(info_node->info->ptr),
              info_node);
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
        Elm_Genlist_Item_Type type = ELM_GENLIST_ITEM_NONE;

        if (info_node->children)
          {
             type = ELM_GENLIST_ITEM_TREE;
          }
        Elm_Object_Item  *glg = elm_genlist_item_append(
              pub_widgets->elm_win1->elm_genlist1, _objs_itc,
              (void *)info_node, NULL,
              type,
              _objs_sel_cb, NULL);
        if (info_node->children)
             elm_genlist_item_expanded_set(glg, EINA_FALSE);
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

static const Eina_Debug_Opcode ops[] =
{
     {"daemon/client_status_register", &_cl_stat_reg_opcode,  _clients_info_cb},
     {"daemon/client_added", NULL, _clients_info_cb},
     {"daemon/client_deleted", NULL, _clients_info_deleted_cb},
     {"Module/Init",          &_module_init_opcode,   &_module_initted},
     {"poll/on",              &_poll_on_opcode,       NULL},
     {"poll/off",             &_poll_off_opcode,      NULL},
     {"evlog/on",             &_evlog_on_opcode,      NULL},
     {"evlog/off",            &_evlog_off_opcode,     NULL},
     {"Eo/list",              &_eo_list_opcode,       &_objects_list_cb},
     {"Elementary/objects_list",       &_elm_list_opcode,      &_elm_objects_list_cb},
     {"Eolian/object/info_get", &_obj_info_opcode, &_debug_obj_info_cb},
     {NULL, NULL, NULL}
};

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   eina_init();
   eolian_init();
   eolian_directory_scan(EOLIAN_EO_DIR);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   pub_widgets =  gui_gui_get();

   //Init objects Genlist
   Evas_Object *objs_list_genlist = pub_widgets->elm_win1->elm_genlist1;
   if (!_objs_itc)
     {
        _objs_itc = elm_genlist_item_class_new();
        _objs_itc->item_style = "default";
        _objs_itc->func.text_get = _objs_item_label_get;
        _objs_itc->func.content_get = NULL;
        _objs_itc->func.state_get = NULL;
        _objs_itc->func.del = NULL;
     }
   eo_do(objs_list_genlist,
         eo_event_callback_add(ELM_GENLIST_EVENT_EXPAND_REQUEST, _objs_expand_request_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_CONTRACT_REQUEST, _objs_contract_request_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_EXPANDED, _objs_expanded_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_CONTRACTED, _objs_contracted_cb, NULL)
        );

   //Init object info Genlist
   Evas_Object *obj_info_genlist = pub_widgets->elm_win1->elm_genlist2;
   if (!_obj_info_itc)
     {
        _obj_info_itc = elm_genlist_item_class_new();
        _obj_info_itc->item_style = "default";
        _obj_info_itc->func.text_get = _obj_info_item_label_get;
        _obj_info_itc->func.content_get = NULL;
        _obj_info_itc->func.state_get = NULL;
        _obj_info_itc->func.del =  _obj_info_item_del;
     }
   eo_do(obj_info_genlist,
         eo_event_callback_add(ELM_GENLIST_EVENT_EXPAND_REQUEST, _obj_info_expand_request_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_CONTRACT_REQUEST, _obj_info_contract_request_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_EXPANDED, _obj_info_expanded_cb, NULL),
         eo_event_callback_add(ELM_GENLIST_EVENT_CONTRACTED, _obj_info_contracted_cb, NULL)
        );
   _session = eina_debug_session_new();

   if (!eina_debug_local_connect(_session))
     {
        fprintf(stderr, "ERROR: Cannot connect to debug daemon.\n");
        goto error;
     }

   eina_debug_opcodes_register(_session, ops, _post_register_handle);

   elm_run();

error:
   eolian_debug_object_information_free(obj_info);
   _objs_nodes_free(_objs_list_tree);
   eina_debug_session_free(_session);
   eina_shutdown();
   eolian_shutdown();
   return 0;
}
ELM_MAIN()
