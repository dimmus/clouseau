#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <fcntl.h>
#include <sys/file.h>
#include <Ecore_Con_Eet.h>

#include "clouseau_private.h"

#define LOCK_FILE "/tmp/clouseaud.pid"

static Eina_List *gui = NULL; /* List of app_info_st for gui clients */
static Eina_List *app = NULL; /* List of app_info_st for app clients */
static Ecore_Con_Eet *eet_svr = NULL;

struct _tree_info_st
{
   void *app;    /* app ptr to identify where the data came from */
   void *data;   /* Tree data */
};
typedef struct _tree_info_st tree_info_st;

int _clouseaud_log_dom = -1;

#ifdef CRITICAL
#undef CRITICAL
#endif
#define CRITICAL(...) EINA_LOG_DOM_CRIT(_clouseaud_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_clouseaud_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_clouseaud_log_dom, __VA_ARGS__)

#ifdef INF
#undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_clouseaud_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_clouseaud_log_dom, __VA_ARGS__)


static void
_daemon_cleanup(void)
{  /*  Free strings */
   app_info_st *p;

   DBG("Clients connected to this server when exiting: %d",
         eina_list_count(app) + eina_list_count(gui));

   EINA_LIST_FREE(gui, p)
     {
        if(p->file)
          free(p->file);

        if (p->ptr)
          free((void *) (uintptr_t) p->ptr);

        free(p->name);
        free(p);
     }

   EINA_LIST_FREE(app, p)
     {
        if(p->file)
          free(p->file);

        if (p->ptr)
          free((void *) (uintptr_t) p->ptr);

        free(p->name);
        free(p);
     }

   gui = app = NULL;
   eet_svr = NULL;

   clouseau_data_shutdown();
   ecore_con_shutdown();
   ecore_shutdown();
   eina_shutdown();
}

/* START - Ecore communication callbacks */
static int
_client_ptr_cmp(const void *d1, const void *d2)
{
   return (((app_info_st *) d1)->ptr - ((unsigned long long) (uintptr_t) d2));
}

static Eina_List *
_add_client(Eina_List *clients, connect_st *t, void *client, const char *type)
{
   DBG("msg from <%p>", client);

   if(!eina_list_search_unsorted(clients, _client_ptr_cmp, client))
     {
        app_info_st *st = calloc(1, sizeof(app_info_st));
        st->name = strdup(t->name);
        st->pid = t->pid;
        st->ptr = (unsigned long long) (uintptr_t) client;
        DBG("Added %s client <%p>", type, client);
        return eina_list_append(clients, st);
     }

   return clients;
}

static Eina_List *
_remove_client(Eina_List *clients, void *client, const char *type)
{
   app_info_st *p = eina_list_search_unsorted(clients, _client_ptr_cmp, client);
   DBG("Msg from <%p>", client);

   if (p)
     {
        free(p->name);
        free(p);
        DBG("Removed %s client <%p>", type, client);
        return eina_list_remove(clients, p);
     }

   return clients;
}

Eina_Bool
_add(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED Ecore_Con_Client *conn)
{
/* TODO:   ecore_ipc_client_data_size_max_set(ev->client, -1); */
   DBG("msg from <%p>", reply);

   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
_del(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED Ecore_Con_Client *conn)
{
   DBG("msg from <%p>", reply);

   /* Now we need to find if its an APP or GUI client */
   app_info_st *i = eina_list_search_unsorted(gui, _client_ptr_cmp, reply);
   if (i)  /* Only need to remove GUI client from list */
     gui = _remove_client(gui, reply, "GUI");

   i = eina_list_search_unsorted(app, _client_ptr_cmp, reply);
   if (i)
     {  /* Notify all GUI clients to remove this APP */
        app_closed_st t = { (unsigned long long) (uintptr_t) reply };
        Eina_List *l;
        EINA_LIST_FOREACH(gui, l, i)
          {
             DBG("<%p> Sending APP_CLOSED to <%p>", reply,
                   (void *) (uintptr_t) i->ptr);
             ecore_con_eet_send((void *) (uintptr_t) i->ptr,
                   CLOUSEAU_APP_CLOSED_STR, &t);
          }

        app = _remove_client(app, reply, "APP");
     }

   if (!(gui || app))
     {  /* Trigger cleanup and exit when all clients disconneced */
        /* ecore_con_eet_server_free(eet_svr); why this causes Segfault? */
        ecore_con_server_del(data);
        ecore_main_loop_quit();
     }

   return ECORE_CALLBACK_RENEW;
}

void
_gui_client_connect_cb(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* Register GUI, then notify about all APP */
   app_info_st *st;
   Eina_List *l;
   connect_st *t = value;
   DBG("msg from <%p>", reply);

   gui = _add_client(gui, t, reply, "GUI");

   /* Add all registered apps to newly open GUI */
   EINA_LIST_FOREACH(app, l, st)
     {
        DBG("<%p> Sending APP_ADD to <%p>",
              (void *) (uintptr_t) st->ptr, reply);

        ecore_con_eet_send(reply, CLOUSEAU_APP_ADD_STR, st);
     }
}

void
_app_client_connect_cb(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* Register APP then notify GUI about it */
   app_info_st *st;
   Eina_List *l;
   connect_st *t = value;
   app_info_st m = { t->pid, (char *) t->name, NULL,
        (unsigned long long) (uintptr_t) reply, NULL, 0 };

   DBG("msg from <%p>", reply);

   app = _add_client(app, t, reply, "APP");

   /* Notify all GUI clients to add APP */
   EINA_LIST_FOREACH(gui, l, st)
     {
        DBG("<%p> Sending APP_ADD to <%p>",
              reply, (void *) (uintptr_t) st->ptr);
        ecore_con_eet_send((void *) (uintptr_t) st->ptr,
              CLOUSEAU_APP_ADD_STR, &m);
     }
}

void
_data_req_cb(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* msg coming from GUI, FWD this to APP specified in REQ */
   data_req_st *req = value;
   DBG("msg from <%p>", reply);

   if (req->app)
     {  /* Requesting specific app data */
        if(eina_list_search_unsorted(app,
                 _client_ptr_cmp,
                 (void *) (uintptr_t) req->app))
          {  /* Do the req only of APP connected to daemon */
             data_req_st t = {
                  (unsigned long long) (uintptr_t) reply,
                  (unsigned long long) (uintptr_t) req->app };

             DBG("<%p> Sending DATA_REQ to <%p>",
                   reply, (void *) (uintptr_t) req->app);
             ecore_con_eet_send((void *) (uintptr_t) req->app,
                   CLOUSEAU_DATA_REQ_STR, &t);
          }
     }
   else
     {  /* requesting ALL apps data */
        Eina_List *l;
        app_info_st *st;
        data_req_st t = {
             (unsigned long long) (uintptr_t) reply,
             (unsigned long long) (uintptr_t) NULL };

        EINA_LIST_FOREACH(app, l, st)
          {
             t.app = (unsigned long long) (uintptr_t) st->ptr;
             DBG("<%p> Sending DATA_REQ to <%p>",
                   reply, (void *) (uintptr_t) st->ptr);
             ecore_con_eet_send((void *) (uintptr_t) st->ptr, CLOUSEAU_DATA_REQ_STR, &t);
          }
     }
}

void
_tree_data_cb(EINA_UNUSED void *data, EINA_UNUSED Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* Tree Data comes from APP, GUI client specified in msg */
   tree_data_st *td = value;
   DBG("msg from <%p>", reply);

   if (td->gui)
     {  /* Sending tree data to specific GUI client */
        if(eina_list_search_unsorted(gui,
                 _client_ptr_cmp,
                 (void *) (uintptr_t) td->gui))
          {  /* Do the req only of GUI connected to daemon */
             DBG("<%p> Sending TREE_DATA to <%p>",
                   reply, (void *) (uintptr_t) td->gui);
             ecore_con_eet_send((void *) (uintptr_t) td->gui,
                   CLOUSEAU_TREE_DATA_STR, value);
          }
     }
   else
     {  /* Sending tree data to all GUI clients */
        Eina_List *l;
        app_info_st *info;
        EINA_LIST_FOREACH(gui, l, info)
          {
             DBG("<%p> Sending TREE_DATA to <%p>",
                   reply, (void *) (uintptr_t) info->ptr);
             ecore_con_eet_send((void *) (uintptr_t) info->ptr,
                   CLOUSEAU_TREE_DATA_STR, value);
          }
     }

   clouseau_data_tree_free(td->tree);
}

void
_highlight_cb(EINA_UNUSED void *data, EINA_UNUSED Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* FWD this message to APP */
   highlight_st *ht = value;
   DBG("msg from <%p>", reply);

   if(eina_list_search_unsorted(app,
            _client_ptr_cmp, (void *) (uintptr_t) ht->app))
     {  /* Do the REQ only of APP connected to daemon */
        DBG("<%p> Sending HIGHLIGHT to <%p>",
              reply, (void *) (uintptr_t) ht->app);
        ecore_con_eet_send((void *) (uintptr_t) ht->app,
              CLOUSEAU_HIGHLIGHT_STR, value);
     }
}

void
_bmp_req_cb(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* BMP data request coming from GUI to APP client */
   bmp_req_st *req = value;
   DBG("msg from <%p>", reply);

   if(eina_list_search_unsorted(app,
            _client_ptr_cmp, (void *) (uintptr_t) req->app))
     {  /* Do the req only if APP connected to daemon */
        bmp_req_st t = {
             (unsigned long long) (uintptr_t) reply,
             req->app, req->object, req->ctr };

        DBG("<%p> Sending BMP_REQ to <%p>",
              reply, (void *) (uintptr_t) req->app);
        ecore_con_eet_send((void *) (uintptr_t) req->app,
              CLOUSEAU_BMP_REQ_STR, &t);
     }
}

void
_bmp_data_cb(EINA_UNUSED void *data, EINA_UNUSED Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, EINA_UNUSED const char *section,
      void *value, EINA_UNUSED size_t length)
{  /* BMP Data comes from APP, GUI client specified in msg */
   bmp_info_st *st = clouseau_data_packet_info_get(protocol_name,
         value, length);

   DBG("msg from <%p>", reply);

   if (st->gui)
     {  /* Sending BMP data to specific GUI client */
        if(eina_list_search_unsorted(gui,
                 _client_ptr_cmp,
                 (void *) (uintptr_t) st->gui))
          {  /* Do the req only of GUI connected to daemon */
             DBG("<%p> Sending BMP_DATA to <%p>",
                   reply, (void *) (uintptr_t) st->gui);
             ecore_con_eet_raw_send((void *) (uintptr_t) st->gui,
                   CLOUSEAU_BMP_DATA_STR, "BMP", value, length);
          }
     }
   else
     {  /* Sending BMP data to all GUI clients */
        Eina_List *l;
        app_info_st *info;
        EINA_LIST_FOREACH(gui, l, info)
          {
             DBG("<%p> Sending BMP_DATA to <%p>",
                   reply, (void *) (uintptr_t) info->ptr);
             ecore_con_eet_raw_send((void *) (uintptr_t) info->ptr,
                   CLOUSEAU_BMP_DATA_STR, "BMP", value, length);
          }
     }

   if (st->bmp)
     free(st->bmp);

   free(st);
}
/* END   - Ecore communication callbacks */

int main(void)
{
   /* Check single instance. */

     {
        int pid_file = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);
        int rc = flock(pid_file, LOCK_EX | LOCK_NB);
        if ((pid_file == -1) || ((rc) && (EWOULDBLOCK == errno)))
          {
             fprintf(stderr, "Clouseaud already running.\n");
             exit(0);
          }
     }
   /* End of check single instance. */

   eina_init();
   ecore_init();
   ecore_con_init();
   clouseau_data_init();
   Ecore_Con_Server *server = NULL;
   const char *log_dom = "clouseaud";


   _clouseaud_log_dom = eina_log_domain_register(log_dom, EINA_COLOR_LIGHTBLUE);
   if (_clouseaud_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: %s", log_dom);
        return EINA_FALSE;
     }

   if (!(server = ecore_con_server_add(ECORE_CON_REMOTE_TCP,
               LISTEN_IP, PORT, NULL)))
     exit(1);

   eet_svr = ecore_con_eet_server_new(server);
   if (!eet_svr)
     exit(2);

   clouseau_register_descs(eet_svr);

   /* Register callbacks for ecore_con_eet */
   ecore_con_eet_client_connect_callback_add(eet_svr, _add, NULL);
   ecore_con_eet_client_disconnect_callback_add(eet_svr, _del, server);
   ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_GUI_CLIENT_CONNECT_STR,
         _gui_client_connect_cb, NULL);
   ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_APP_CLIENT_CONNECT_STR,
         _app_client_connect_cb, NULL);
   ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_DATA_REQ_STR,
         _data_req_cb, NULL);
   ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_TREE_DATA_STR,
         _tree_data_cb, NULL);
   ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_HIGHLIGHT_STR,
         _highlight_cb, NULL);
   ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_BMP_REQ_STR,
         _bmp_req_cb, NULL);
   ecore_con_eet_raw_data_callback_add(eet_svr, CLOUSEAU_BMP_DATA_STR,
         _bmp_data_cb, NULL);

   ecore_main_loop_begin();
   _daemon_cleanup();

   return 0;
}
