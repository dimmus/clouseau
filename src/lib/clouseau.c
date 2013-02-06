#include "clouseau_private.h"

static int _clouseau_init_count = 0;
static Eina_Stringshare *_my_appname = NULL;
static Eina_List *(*_load_list)(void) = NULL;
static void *(*_canvas_bmp_get)(Ecore_Evas *ee, Evas_Coord *w_out, Evas_Coord *h_out) = NULL;

/* Highlight functions. */
static Eina_Bool
_clouseau_highlight_fade(void *_rect)
{
   Evas_Object *rect = _rect;
   int r, g, b, a;
   double na;

   evas_object_color_get(rect, &r, &g, &b, &a);
   if (a < 20)
     {
        evas_object_del(rect);
        /* The del callback of the object will destroy the animator */
        return EINA_TRUE;
     }

   na = a - 5;
   r = na / a * r;
   g = na / a * g;
   b = na / a * b;
   evas_object_color_set(rect, r, g, b, na);

   return EINA_TRUE;
}

static Evas_Object *
_clouseau_verify_e_children(Evas_Object *obj, Evas_Object *ptr)
{
   /* Verify that obj still exists (can be found on evas canvas) */
   Evas_Object *child;
   Evas_Object *p = NULL;
   Eina_List *children;

   if (ptr == obj)
     return ptr;

   children = evas_object_smart_members_get(obj);
   EINA_LIST_FREE(children, child)
     {
        p = _clouseau_verify_e_children(child, ptr);
        if (p) break;
     }
   eina_list_free(children);

   return p;
}

static Evas_Object *
_clouseau_verify_e_obj(Evas_Object *obj)
{
   /* Verify that obj still exists (can be found on evas canvas) */
   Evas_Object *o;
   Eina_List *ees;
   Ecore_Evas *ee;
   Evas_Object *rt = NULL;

   ees = ecore_evas_ecore_evas_list_get();
   EINA_LIST_FREE(ees, ee)
     {
        Evas *e;
        Eina_List *objs;

        e = ecore_evas_get(ee);
        objs = evas_objects_in_rectangle_get(e, SHRT_MIN, SHRT_MIN,
                                             USHRT_MAX, USHRT_MAX,
                                             EINA_TRUE, EINA_TRUE);

        EINA_LIST_FREE(objs, o)
          {
             rt = _clouseau_verify_e_children(o, obj);
             if (rt) break;
          }
        eina_list_free(objs);

        if (rt) break;
     }
   eina_list_free(ees);

   return rt;
}

static void
_clouseau_highlight_del(void *data,
                        EINA_UNUSED Evas *e,
                        EINA_UNUSED Evas_Object *obj,
                        EINA_UNUSED void *event_info)
{  /* Delete timer for this rect */
   ecore_animator_del(data);
}

EAPI void
clouseau_data_object_highlight(Evas_Object *obj, Clouseau_Evas_Props *props, bmp_info_st *view)
{
   Ecore_Animator *t;
   Evas_Object *r;
   int x, y, wd, ht;
   Evas *e = NULL;

   if (props)
     {
        /* When working offline grab info from struct */
        Evas_Coord x_bmp, y_bmp;

        evas_object_geometry_get(view->o, &x_bmp, &y_bmp, NULL, NULL);
        x =  (view->zoom_val * props->x) + x_bmp;
        y =  (view->zoom_val * props->y) + y_bmp;
        wd = (view->zoom_val * props->w);
        ht = (view->zoom_val * props->h);

        e = evas_object_evas_get(view->win);
     }
   else
     {
        /* Check validity of object when working online */
        if (!_clouseau_verify_e_obj(obj))
          {
             printf("<%s> Evas Object not found <%p> (probably deleted)\n",
                   __func__, obj);
             return;
          }

        evas_object_geometry_get(obj, &x, &y, &wd, &ht);

        /* Take evas from object if working online */
        e = evas_object_evas_get(obj);
        if (!e) return;
     }

   /* Continue and do the Highlight */
   r = evas_object_rectangle_add(e);
   evas_object_move(r, x - PADDING, y - PADDING);
   evas_object_resize(r, wd + (2 * PADDING), ht + (2 * PADDING));
   evas_object_color_set(r,
                         HIGHLIGHT_R, HIGHLIGHT_G, HIGHLIGHT_B, HIGHLIGHT_A);
   evas_object_show(r);

   /* Add Timer for fade and a callback to delete timer on obj del */
   t = ecore_animator_add(_clouseau_highlight_fade, r);
   evas_object_event_callback_add(r, EVAS_CALLBACK_DEL,
                                  _clouseau_highlight_del, t);
/* Print backtrace info, saved for future ref
   tmp = evas_object_data_get(obj, ".clouseau.bt");
   fprintf(stderr, "Creation backtrace :\n%s*******\n", tmp); */
}

EAPI Clouseau_Object *
clouseau_object_information_get(Clouseau_Tree_Item *treeit)
{
   Evas_Object *obj = (void*) (uintptr_t) treeit->ptr;
   Eo_Dbg_Info *eo_dbg_info = EO_DBG_INFO_LIST_APPEND(NULL, "");

   if (!treeit->is_obj)
     return NULL;

   eo_do(obj, eo_dbg_info_get(eo_dbg_info));
   treeit->eo_info = clouseau_eo_to_legacy_convert(eo_dbg_info);

   eo_dbg_info_free(eo_dbg_info); /* Free original list */

   return NULL;
}


Eina_Bool
_add(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED Ecore_Con_Server *conn)
{
/*   ecore_con_server_data_size_max_set(conn, -1); */
   connect_st t = { getpid(), _my_appname };
   ecore_con_eet_send(reply, CLOUSEAU_APP_CLIENT_CONNECT_STR, &t);

   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
_del(EINA_UNUSED void *data, EINA_UNUSED Ecore_Con_Reply *reply,
      Ecore_Con_Server *conn)
{
   if (!conn)
     {
        printf("Failed to establish connection to the server.\n");
     }

   printf("Lost server with ip <%s>\n", ecore_con_server_ip_get(conn));

   ecore_con_server_del(conn);

   return ECORE_CALLBACK_RENEW;
}

void
_data_req_cb(EINA_UNUSED void *data, Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* data req includes ptr to GUI, to tell which client asking */
   data_req_st *req = value;
   tree_data_st t;
   t.gui = req->gui;  /* GUI client requesting data from daemon */
   t.app = req->app;  /* APP client sending data to daemon */
   t.tree = _load_list();

   if (t.tree)
     {  /* Reply with tree data to data request */
        ecore_con_eet_send(reply, CLOUSEAU_TREE_DATA_STR, &t);
        clouseau_data_tree_free(t.tree);
     }
}

void
_highlight_cb(EINA_UNUSED void *data, EINA_UNUSED Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* Highlight msg contains PTR of object to highlight */
   highlight_st *ht = value;
   Evas_Object *obj = (Evas_Object *) (uintptr_t) ht->object;
   clouseau_data_object_highlight(obj, NULL, NULL);
}

void
_bmp_req_cb(EINA_UNUSED void *data, EINA_UNUSED Ecore_Con_Reply *reply,
      EINA_UNUSED const char *protocol_name, void *value)
{  /* Bitmap req msg contains PTR of Ecore Evas */
   bmp_req_st *req = value;
   Evas_Coord w, h;
   unsigned int size = 0;
   void *bmp = _canvas_bmp_get((Ecore_Evas *) (uintptr_t)
         req->object, &w, &h);

   bmp_info_st t = { req->gui,
        req->app, req->object , req->ctr, w, h,
        NULL,NULL, NULL, 1.0,
        NULL, NULL, NULL, NULL, NULL, NULL };

   void *p = clouseau_data_packet_compose(CLOUSEAU_BMP_DATA_STR,
         &t, &size, bmp, (w * h * sizeof(int)));


   if (p)
     {
        ecore_con_eet_raw_send(reply, CLOUSEAU_BMP_DATA_STR, "BMP", p, size);
        free(p);
     }

   if (bmp)
     free(bmp);
}

/* FIXME: This one is ugly and should be done elsewhere.. */
EAPI void
clouseau_app_data_req_cb_set(Eina_List *(*cb)(void))
{
   _load_list = cb;
}

/* FIXME: This one is ugly and should be done elsewhere.. */
EAPI void
clouseau_app_canvas_bmp_cb_set(void *(*cb)(Ecore_Evas *ee, Evas_Coord *w_out, Evas_Coord *h_out))
{
   _canvas_bmp_get = cb;
}

EAPI Eina_Bool
clouseau_app_connect(const char *appname)
{
   Ecore_Con_Server *server;
   const char *address = LOCALHOST;
   Ecore_Con_Eet *eet_svr = NULL;

   eina_stringshare_replace(&_my_appname, appname);

   server = ecore_con_server_connect(ECORE_CON_REMOTE_TCP,
         LOCALHOST, PORT, NULL);

   if (!server)
     {
        printf("could not connect to the server: %s, port %d.\n",
              address, PORT);
        return EINA_FALSE;
     }

   eet_svr = ecore_con_eet_client_new(server);
   if (!eet_svr)
     {
        printf("could not create con_eet client.\n");
        return EINA_FALSE;
     }

   clouseau_register_descs(eet_svr);

   /* Register callbacks for ecore_con_eet */
   ecore_con_eet_server_connect_callback_add(eet_svr, _add, NULL);
   ecore_con_eet_server_disconnect_callback_add(eet_svr, _del, NULL);
   ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_DATA_REQ_STR,
         _data_req_cb, NULL);
   ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_HIGHLIGHT_STR,
         _highlight_cb, NULL);
   ecore_con_eet_data_callback_add(eet_svr, CLOUSEAU_BMP_REQ_STR,
         _bmp_req_cb, NULL);

   return EINA_TRUE;
}

EAPI Eina_Bool
clouseau_daemon_connect(void)
{
   return EINA_TRUE;
}

EAPI Eina_Bool
clouseau_client_connect(void)
{
   return EINA_TRUE;
}

EAPI Eina_Bool
clouseau_disconnect(void)
{
   return EINA_TRUE;
}

EAPI int
clouseau_init(void)
{
   if (++_clouseau_init_count == 1)
     {
        eina_init();
        ecore_init();
        ecore_con_init();
        clouseau_data_init();
     }

   return _clouseau_init_count;
}

EAPI int
clouseau_shutdown(void)
{
   if (--_clouseau_init_count == 0)
     {
        eina_stringshare_del(_my_appname);
        _my_appname = NULL;

        clouseau_data_shutdown();
        ecore_con_shutdown();
        ecore_shutdown();
        eina_shutdown();
     }
   else if (_clouseau_init_count < 0)
     {
        _clouseau_init_count = 0;
        printf("Tried to shutdown although not initiated.\n");
     }

   return _clouseau_init_count;
}
