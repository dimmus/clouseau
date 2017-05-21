#include <Eina.h>
#include <Elementary.h>

#include "../../Clouseau.h"

#define _EET_ENTRY "config"

#define RES 500000.0
#define LEN 5.0

typedef struct _Evlog            Evlog;
typedef struct _Evlog_Thread     Evlog_Thread;
typedef struct _Evlog_Event      Evlog_Event;
typedef struct _Evlog_Cpu_Use    Evlog_Cpu_Use;
typedef struct _Evlog_Cpu_Freq   Evlog_Cpu_Freq;

struct _Evlog
{
   double           first_timestamp;
   double           last_timestamp;
   int              state_uniq;
   int              state_num;
   int              cpucores;
   int              cpumhzmax;
   int              thread_num;
   int              cpufreq_num;
   Evlog_Event     *states;
   Evlog_Thread    *threads;
   Evlog_Cpu_Freq  *cpufreqs;
   char           **state_uniq_str;
   int              cpumhzlast[64];
};

struct _Evlog_Event
{
   const char *event;
   const char *detail;
   double      timestamp;
   double      latency;
};

struct _Evlog_Cpu_Use
{
   int        usage;
   double     timestamp;
};

struct _Evlog_Cpu_Freq
{
   int        core;
   int        mhz;
   double     timestamp;
};

struct _Evlog_Thread
{
   unsigned long long  id;
   int                 event_num;
   int                 cpuused_num;
   Evlog_Event        *events;
   Evlog_Cpu_Use      *cpuused;
};

typedef struct
{
   void *src;
   char *event;
   char *detail;
   Evas_Object *obj;
   double t0, t1;
   int n, n2;
   int slot;
   Eina_Bool nuke : 1;
   Eina_Bool cpu_use : 1;
} Event;

typedef struct
{
   Evlog *evlog;
   Eo *main;
   Eo *zoom_slider;
   Eo *scroller;
   Eo *table;
   struct {
      Evas_Object *state;
      Evas_Object *cpufreq;
      Evas_Object **thread;
      Evas_Object *over;
   } grid;
   Eina_List *objs;
   struct {
      Ecore_Job *job;
      Ecore_Thread *thread;
      double t0, t1, tmin;
      volatile Eina_Bool redo;
      Eina_List *remobjs;
   } update;
} Inf;

#define ROUND_AMOUNT 1024
#define ROUND(x) ((x + (ROUND_AMOUNT - 1)) / ROUND_AMOUNT) * ROUND_AMOUNT;

static void _fill_begin(Inf *inf);

static void
_evlog_state_event_register(Evlog *evlog, Evlog_Event *ev)
{
   int n0, n, j;

   for (j = 0; j < evlog->state_uniq; j++)
     {
        if (!strcmp(evlog->state_uniq_str[j], ev->event + 1)) break;
     }
   if (j == evlog->state_uniq)
     {
        evlog->state_uniq++;
        evlog->state_uniq_str = realloc(evlog->state_uniq_str,
                                        evlog->state_uniq * sizeof(char *));
        evlog->state_uniq_str[evlog->state_uniq - 1] = strdup(ev->event + 1);
     }
   n0 = evlog->state_num;
   evlog->state_num++;
   n = evlog->state_num;
   n0 = ROUND(n0);
   n = ROUND(n);
   if (n != n0)
     {
        Evlog_Event *tmp;

        tmp = realloc(evlog->states, n * sizeof(Evlog_Event));
        if (!tmp)
          {
             eina_stringshare_del(ev->event);
             eina_stringshare_del(ev->detail);
             return;
          }
        evlog->states = tmp;
     }
   evlog->states[evlog->state_num - 1] = *ev;
}

static void
_evlog_thread_event_register(Evlog_Thread *th, Evlog_Event *ev)
{
   int n0, n;

   n0 = th->event_num;
   th->event_num++;
   n = th->event_num;
   n0 = ROUND(n0);
   n = ROUND(n);
   if (n != n0)
     {
        Evlog_Event *tmp;

        tmp = realloc(th->events, n * sizeof(Evlog_Event));
        if (!tmp)
          {
             eina_stringshare_del(ev->event);
             eina_stringshare_del(ev->detail);
             return;
          }
        th->events = tmp;
     }
   th->events[th->event_num - 1] = *ev;
}

static void
_evlog_thread_cpu_freq(Evlog *evlog, double timestamp, int core, int mhz)
{
   int n0, n;

   n0 = evlog->cpufreq_num;
   evlog->cpufreq_num++;
   n = evlog->cpufreq_num;
   n0 = ROUND(n0);
   n = ROUND(n);
   if (n != n0)
     {
        Evlog_Cpu_Freq *tmp;

        tmp = realloc(evlog->cpufreqs, n * sizeof(Evlog_Cpu_Freq));
        if (!tmp) return;
        evlog->cpufreqs = tmp;
     }
   evlog->cpufreqs[evlog->cpufreq_num - 1].core = core;
   evlog->cpufreqs[evlog->cpufreq_num - 1].mhz = mhz;
   evlog->cpufreqs[evlog->cpufreq_num - 1].timestamp = timestamp;
   if (evlog->cpucores <= core) evlog->cpucores = core + 1;
   if (mhz > evlog->cpumhzmax) evlog->cpumhzmax = mhz;
   evlog->cpumhzlast[core] = mhz;
}

static void
_evlog_thread_cpu_use(Evlog_Thread *th, double timestamp, int cpu)
{
   int n0, n;

   n0 = th->cpuused_num;
   th->cpuused_num++;
   n = th->cpuused_num;
   n0 = ROUND(n0);
   n = ROUND(n);
   if (n != n0)
     {
        Evlog_Cpu_Use *tmp;

        tmp = realloc(th->cpuused, n * sizeof(Evlog_Cpu_Use));
        if (!tmp) return;
        th->cpuused = tmp;
     }
   th->cpuused[th->cpuused_num - 1].usage = cpu;
   th->cpuused[th->cpuused_num - 1].timestamp = timestamp;
}

static Eina_Bool
_evlog_thread_push(Evlog *evlog, int slot, unsigned long long thread)
{
   Evlog_Thread *tmp;

   if (slot < evlog->thread_num) return EINA_TRUE;

   evlog->thread_num = slot + 1;
   tmp = realloc(evlog->threads, evlog->thread_num * sizeof(Evlog_Thread));
   if (!tmp) return EINA_FALSE;
   evlog->threads = tmp;
   evlog->threads[slot].id = thread;
   evlog->threads[slot].event_num = 0;
   evlog->threads[slot].events = NULL;
   evlog->threads[slot].cpuused_num = 0;
   evlog->threads[slot].cpuused = NULL;
   return EINA_TRUE;
}

static int
_evlog_thread_slot_find(Evlog *evlog, unsigned long long thread)
{
   int i;

   for (i = 0; i < evlog->thread_num; i++)
     {
        if (evlog->threads[i].id == thread) return i;
     }
   return i;
}

static void
_evlog_event_register(Evlog *evlog, unsigned long long thread, Evlog_Event *ev)
{
   int i;

   if ((ev->event[0] == '<') || (ev->event[0] == '>'))
     {
        _evlog_state_event_register(evlog, ev);
     }
   else if (ev->event[0] == '*')
     {
        if ((!strncmp(ev->event, "*CPUFREQ ", 9)) && (ev->detail))
          {
             int cpu = atoi(ev->event + 9);
             int freq = atoi(ev->detail);

             _evlog_thread_cpu_freq(evlog, ev->timestamp, cpu, freq);
          }
        else if ((!strncmp(ev->event, "*CPUUSED ", 9)) && (ev->detail))
          {
             unsigned long long thcpu = atoll(ev->event + 9);
             int cpu = atoi(ev->detail);

             i = _evlog_thread_slot_find(evlog, thcpu);
             if (!_evlog_thread_push(evlog, i, thcpu)) return;
             _evlog_thread_cpu_use(&(evlog->threads[i]), ev->timestamp, cpu);
          }
        eina_stringshare_del(ev->event);
        eina_stringshare_del(ev->detail);
    }
   else
     {
        i = _evlog_thread_slot_find(evlog, thread);
        if (!_evlog_thread_push(evlog, i, thread))
          {
             eina_stringshare_del(ev->event);
             eina_stringshare_del(ev->detail);
             return;
          }
        _evlog_thread_event_register(&(evlog->threads[i]), ev);
     }
}

static void *
_evlog_event_read(Evlog *evlog, void *ptr, void *end)
{
   char *data    = ptr;
   char *dataend = end;
   const char *eventstr = NULL, *detailstr = NULL;
   Eina_Evlog_Item item;

   if ((dataend - data) < sizeof(Eina_Evlog_Item)) return NULL;

   memcpy(&item, data, sizeof(Eina_Evlog_Item));

   if (item.event_offset >= sizeof(Eina_Evlog_Item))
     eventstr = data + item.event_offset;
   if (item.detail_offset >= sizeof(Eina_Evlog_Item))
     detailstr = data + item.detail_offset;
   if (eventstr)
     {
        Evlog_Event ev;
        ev.event = eina_stringshare_add(eventstr);
        if (detailstr)
          ev.detail = eina_stringshare_add(detailstr);
        else
          ev.detail = NULL;
        ev.timestamp = (item.srctim == 0.0) ? item.tim : item.srctim;
        ev.latency = (item.srctim != 0.0) ? item.tim - item.srctim : 0.0;
        if (evlog->first_timestamp == 0.0)
          evlog->first_timestamp = ev.timestamp;
        if (ev.timestamp > evlog->last_timestamp)
          evlog->last_timestamp = ev.timestamp;
        _evlog_event_register(evlog, item.thread, &ev);
     }

   data += item.event_next;
   if (data >= dataend) return NULL;
   return data;
}

static int
_evlog_block_read(Evlog *evlog, char *buffer, int size)
{
   int i;
   unsigned int *header = (unsigned int *)buffer;
   Eina_Bool bigendian = EINA_FALSE;

   if (size < 12) return -1;
   size -= 12;

   if      (header[0] == 0x0ffee211) bigendian = EINA_FALSE;
   else if (header[0] == 0x11e2fe0f) bigendian = EINA_TRUE;
   else return -1;
   if (!bigendian)
     {
        unsigned int blocksize = header[1];
        //unsigned int overflow = header[2];
        void *buf = buffer + 12;
        char *ptr, *end;
        if ((unsigned int)size < blocksize) return -1;
        ptr = buf;
        end = ptr + blocksize;
        while ((ptr = _evlog_event_read(evlog, ptr, end)));
        for (i = 0; i < evlog->cpucores; i++)
          {
             _evlog_thread_cpu_freq(evlog, evlog->last_timestamp,
                                   i, evlog->cpumhzlast[i]);
          }
        return blocksize + 12;
     }
   else
     {
        // XXX: handle bigendian
     }
   return -1;
}

#if 0
static Evlog *
_evlog_open(const char *file)
{
   Evlog *evlog = calloc(1, sizeof(Evlog));
   if (!evlog) return NULL;
   evlog->file = fopen(file, "rb");
   if (!evlog->file)
     {
        free(evlog);
        return EINA_FALSE;
     }
   while (_evlog_block_read(evlog, NULL, 0));
   return evlog;
}

static void
_evlog_free(Evlog *evlog)
{
   // XXX free other stuff
   free(evlog);
}
#endif

static Eina_Bool
_can_see(double t0, double t1, double tmin, double t0in, double t1in)
{
   if (t1in >= 0.0)
     {
        if ((t0in <= t1) &&
            (t1in >= t0) &&
            ((t1in - t0in) >= tmin))
          return EINA_TRUE;
     }
   else
     {
        if ((t0in <= t1) &&
            (t0in >= t0))
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

static void
_create_log_states(Inf *inf)
{
   Evas_Object *o;
   int h = 1;
   double res = elm_slider_value_get(inf->zoom_slider);

   o = elm_grid_add(inf->main);
   inf->grid.state = o;
   elm_grid_size_set(o, LEN * RES, h);

   res = res * res;
   evas_object_size_hint_min_set(o, LEN * res, h * 20);
}

static void
_fill_log_states(Inf *inf)
{
   Evas_Object *o, *oo;
   Evlog *evlog = inf->evlog;
   int i, j;
   int h = 0;
   Eina_List *events = NULL, *l;
   Event *ev;
   Evlog_Event *e;
   double t;

   double len = evlog->last_timestamp - evlog->first_timestamp;
   o = inf->grid.state;
   for (i = 0; i < evlog->state_num; i++)
     {
        e = &(evlog->states[i]);
        t = e->timestamp - evlog->first_timestamp;
        if (e->event[0] == '>')
          {
             ev = calloc(1, sizeof(Event));
             if (ev)
               {
                  ev->src = e;
                  ev->event = strdup(e->event + 1);
                  if (e->detail) ev->detail = strdup(e->detail);
                  ev->t0 = t;
                  events = eina_list_append(events, ev);
                  for (j = 0; j < evlog->state_uniq; j++)
                    {
                       if (!strcmp(evlog->state_uniq_str[j], ev->event))
                         {
                            ev->n = j;
                            break;
                         }
                    }
                  if ((ev->n + 1) > h) h = ev->n + 1;
               }
          }
        else if (e->event[0] == '<')
          {
             EINA_LIST_FOREACH(events, l, ev)
               {
                  if (!strcmp(ev->event, e->event + 1))
                    {
                       ev->t1 = t;
                       free(ev->event);
                       free(ev->detail);
                       free(ev);
                       events = eina_list_remove_list(events, l);
                       break;
                    }
               }
          }
     }
   t = evlog->last_timestamp - evlog->first_timestamp;
   EINA_LIST_FREE(events, ev)
     {
        ev->t1 = t;
        free(ev->event);
        free(ev->detail);
        free(ev);
     }

   if (h < 1) h = 1;
   elm_grid_size_set(o, len * RES, h);
   double res = elm_slider_value_get(inf->zoom_slider);
   res = res * res;
   evas_object_size_hint_min_set(o, len * res, h * 20);

   oo = evas_object_rectangle_add(evas_object_evas_get(inf->main));
   evas_object_color_set(oo, 24, 24, 24, 255);
   elm_grid_pack(o, oo, 0, 0, len * RES, h);
   evas_object_show(oo);
}

static void
_add_log_state(Inf *inf, Event *ev)
{
   Eina_List *l;
   Event *ev2;

   EINA_LIST_FOREACH(inf->objs, l, ev2)
     {
        if (ev2->src == ev->src)
          {
             free(ev->event);
             free(ev->detail);
             free(ev);
             return;
          }
     }
   inf->objs = eina_list_append(inf->objs, ev);
}

static void
_add_log_event(Inf *inf, Event *ev)
{
   Eina_List *l;
   Event *ev2;

   EINA_LIST_FOREACH(inf->objs, l, ev2)
     {
        if (ev2->src == ev->src)
          {
             free(ev->event);
             free(ev->detail);
             free(ev);
             return;
          }
     }
   inf->objs = eina_list_append(inf->objs, ev);
}

static void
_create_cpufreq_states(Inf *inf)
{
   int cpucores = 1;

   inf->grid.cpufreq = elm_grid_add(inf->main);
   elm_grid_size_set(inf->grid.cpufreq, LEN * RES, cpucores);
}

static void
_fill_cpufreq_states(Inf *inf)
{
   Evas_Object *o, *oo;
   Evlog *evlog = inf->evlog;
   double len = evlog->last_timestamp - evlog->first_timestamp;

   o = inf->grid.cpufreq;
   elm_grid_size_set(o, len * RES, evlog->cpucores);
   evas_object_size_hint_min_set(o, 1, evlog->cpucores * 10);

   oo = evas_object_rectangle_add(evas_object_evas_get(inf->main));
   evas_object_color_set(oo, 16, 16, 16, 255);
   elm_grid_pack(o, oo, 0, 0, len * RES, evlog->cpucores * 4);
   evas_object_show(oo);
}

static void
_fill_timed_log_states(Inf *inf, Evlog *evlog, double t0, double t1, double tmin)
{
   int i, j;
   Eina_List *events = NULL, *l;
   Event *ev;
   char buf[256];
   Evlog_Event *e;
   double t;
   double *ct;

   for (i = 0; i < evlog->state_num; i++)
     {
        e = &(evlog->states[i]);
        t = e->timestamp - evlog->first_timestamp;
        if (e->event[0] == '>')
          {
             ev = calloc(1, sizeof(Event));
             if (ev)
               {
                  ev->src = e;
                  ev->event = strdup(e->event + 1);
                  if (e->detail) ev->detail = strdup(e->detail);
                  ev->t0 = t;
                  events = eina_list_append(events, ev);
                  for (j = 0; j < evlog->state_uniq; j++)
                    {
                       if (!strcmp(evlog->state_uniq_str[j], ev->event))
                         {
                            ev->n = j;
                            break;
                         }
                    }
                  ev->slot = 0;
               }
          }
        else if (e->event[0] == '<')
          {
             EINA_LIST_FOREACH(events, l, ev)
               {
                  if (!strcmp(ev->event, e->event + 1))
                    {
                       ev->t1 = t;
                       if (_can_see(t0, t1, tmin, ev->t0, ev->t1))
                         _add_log_state(inf, ev);
                       else
                         {
                            free(ev->event);
                            free(ev->detail);
                            free(ev);
                         }
                       events = eina_list_remove_list(events, l);
                       break;
                    }
               }
          }
     }
   ct = calloc(evlog->cpucores, sizeof(double));
   for (i = 0; i < evlog->cpufreq_num; i++)
     {
        Evlog_Cpu_Freq *f = &(evlog->cpufreqs[i]);
        double tt = f->timestamp - evlog->first_timestamp;

        if (_can_see(t0, t1, tmin, ct[f->core], tt))
          {
             ev = calloc(1, sizeof(Event));
             if (ev)
               {
                  ev->src = f;
                  ev->event = strdup("*CPUFREQ");
                  snprintf(buf, sizeof(buf), "%iMHz", f->mhz);
                  ev->detail = strdup(buf);
                  ev->n = f->core;
                  ev->n2 = f->mhz;
                  ev->t0 = ct[f->core];
                  ev->t1 = tt;
                  ev->slot = -2;
                  _add_log_event(inf, ev);
               }
          }
        ct[f->core] = tt;
     }
   free(ct);

   t = evlog->last_timestamp - evlog->first_timestamp;
   EINA_LIST_FREE(events, ev)
     {
        ev->t1 = t;
        if (_can_see(t0, t1, tmin, ev->t0, ev->t1))
          _add_log_state(inf, ev);
        else
          {
             free(ev->event);
             free(ev->detail);
             free(ev);
          }
     }
}

static Evas_Object *
_create_log_thread(Inf *inf, Evlog *evlog, Evlog_Thread *th, int slot)
{
   Evas_Object *o, *oo;
   double len = evlog->last_timestamp - evlog->first_timestamp;
   int i, c;
   int h = 0;
   Eina_List *stack = NULL, *l;
   Event *ev;
   Evlog_Event *e;
   double t;

   o = elm_grid_add(inf->main);
   for (i = 0; i < th->event_num; i++)
     {
        e = &(th->events[i]);
        t = e->timestamp - evlog->first_timestamp;
        if     (e->event[0] == '+')
          {
             ev = calloc(1, sizeof(Event));
             if (ev)
               {
                  ev->src = e;
                  stack = eina_list_append(stack, ev);
                  ev->event = strdup(e->event + 1);
                  if (e->detail) ev->detail = strdup(e->detail);
                  ev->t0 = t;
                  ev->n = eina_list_count(stack) - 1;
                  if ((ev->n + 1) > h) h = ev->n + 1;
               }
          }
        else if (e->event[0] == '-')
          {
             l = eina_list_last(stack);
             if (l)
               {
                  ev = l->data;
                  ev->t1 = t;
                  free(ev->event);
                  free(ev);
                  stack = eina_list_remove_list(stack, l);
               }
          }
        else if (e->event[0] == '!')
          {
          }
        else if (e->event[0] == '*')
          {
             // XXX: handle:
             // 'CPUFREQ [X]' (CPU core)  | [N] (Mhz)
             // 'CPUUSED [X]' (Thread ID) | [N] (percent CPU used)
          }
     }
   t = evlog->last_timestamp - evlog->first_timestamp;
   EINA_LIST_FREE(stack, ev)
     {
        ev->t1 = t;
        free(ev->event);
        free(ev->detail);
        free(ev);
     }
   if (h < 1) h = 1;
   elm_grid_size_set(o, len * RES, (h * 2) + 1);
   evas_object_size_hint_min_set(o, 1, (h * 20) + 10);

   oo = evas_object_rectangle_add(evas_object_evas_get(inf->main));
   c = 32 + ((slot % 2) * 16);
   evas_object_color_set(oo, c, c, c, 255);
   elm_grid_pack(o, oo, 0, 0, len * RES, (h * 2) + 1);
   evas_object_show(oo);
   return o;
}

static void
_fill_log_thread(Inf *inf, Evlog *evlog, Evlog_Thread *th, int slot, double t0, double t1, double tmin)
{
   Eina_List *stack = NULL, *l;
   Event *ev;
   int i;
   int h = 0;
   Evlog_Event *e;
   double t;
   char buf[256];

   for (i = 0; i < th->event_num; i++)
     {
        e = &(th->events[i]);
        t = e->timestamp - evlog->first_timestamp;
        if     (e->event[0] == '+')
          {
             ev = calloc(1, sizeof(Event));
             if (ev)
               {
                  ev->src = e;
                  stack = eina_list_append(stack, ev);
                  ev->event = strdup(e->event + 1);
                  if (e->detail) ev->detail = strdup(e->detail);
                  ev->t0 = t;
                  ev->n = eina_list_count(stack) - 1;
                  ev->slot = slot;
                  if ((ev->n + 1) > h) h = ev->n + 1;
               }
          }
        else if (e->event[0] == '-')
          {
             l = eina_list_last(stack);
             if (l)
               {
                  ev = l->data;
                  ev->t1 = t;
                  if (_can_see(t0, t1, tmin, ev->t0, ev->t1))
                    _add_log_event(inf, ev);
                  else
                    {
                       free(ev->event);
                       free(ev->detail);
                       free(ev);
                    }
                  stack = eina_list_remove_list(stack, l);
               }
          }
        else if (e->event[0] == '!')
          {
             ev = calloc(1, sizeof(Event));
             if (ev)
               {
                  ev->src = e;
                  ev->event = strdup(e->event + 1);
                  if (e->detail) ev->detail = strdup(e->detail);
                  ev->t0 = t;
                  ev->t1 = -1.0;
                  ev->n = 0;
                  ev->slot = -1;
                  if (_can_see(t0, t1, tmin, ev->t0, ev->t1))
                    _add_log_event(inf, ev);
                  else
                    {
                       free(ev->event);
                       free(ev->detail);
                       free(ev);
                    }
               }
          }
//        else if (e->event[0] == '*')
//          {
             // XXX: handle:
             // 'CPUFREQ [X]' (CPU core)  | [N] (Mhz)
             // 'CPUUSED [X]' (Thread ID) | [N] (percent CPU used)
//          }
     }
   t = evlog->last_timestamp - evlog->first_timestamp;
   EINA_LIST_FREE(stack, ev)
     {
        ev->t1 = t;
        if (_can_see(t0, t1, tmin, ev->t0, ev->t1))
          _add_log_event(inf, ev);
        else
          {
             free(ev->event);
             free(ev->detail);
             free(ev);
          }
     }
   t = 0.0;
   for (i = 0; i < th->cpuused_num; i++)
     {
        double tt = th->cpuused[i].timestamp - evlog->first_timestamp;

        if (_can_see(t0, t1, tmin, t, tt))
          {
             ev = calloc(1, sizeof(Event));
             if (ev)
               {
                  ev->src = &(th->cpuused[i]);
                  ev->event = strdup("*CPUUSED");
                  snprintf(buf, sizeof(buf), "%i%%", th->cpuused[i].usage);
                  ev->detail = strdup(buf);
                  ev->t0 = t;
                  ev->t1 = tt;
                  ev->n = th->cpuused[i].usage;
                  ev->slot = slot;
                  ev->cpu_use = 1;
                  _add_log_event(inf, ev);
               }
          }
        t = tt;
     }
}

static void
_fill_event_clean(Inf *inf, double t0, double t1, double tmin)
{
   Eina_List *l;
   Event *ev;

   EINA_LIST_FOREACH(inf->objs, l, ev)
     {
        if (_can_see(t0, t1, tmin, ev->t0, ev->t1))
          {
             if (ev->nuke)
               {
                  inf->update.remobjs = eina_list_remove(inf->update.remobjs, ev);
                  ev->nuke = 0;
               }
          }
        else
          {
             if (!ev->nuke)
               {
                  inf->update.remobjs = eina_list_append(inf->update.remobjs, ev);
                  ev->nuke = 1;
               }
          }
     }
}

static void
_create_log_table(Inf *inf)
{
   Evas_Object *o;
   int y = 0;

   o = elm_label_add(inf->main);
   elm_object_text_set(o, "<b>CPU FREQ</b>");
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   elm_table_pack(inf->table, o, 0, y, 1, 1);
   evas_object_show(o);

   _create_cpufreq_states(inf);
   o = inf->grid.cpufreq;
   evas_object_size_hint_weight_set(o, 0.0, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   elm_table_pack(inf->table, o, 2, y++, 1, 1);
   evas_object_show(o);

   o = elm_label_add(inf->main);
   elm_object_text_set(o, "<b>STATES</b>");
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   elm_table_pack(inf->table, o, 0, y, 1, 1);
   evas_object_show(o);

   _create_log_states(inf);
   o = inf->grid.state;
   evas_object_size_hint_weight_set(o, 0.0, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   elm_table_pack(inf->table, o, 2, y++, 1, 1);
   evas_object_show(o);
}

static void
_fill_log_table(Inf *inf)
{
   Evlog *evlog = inf->evlog;
   Eo *o;
   char buf[256];
   int i, y = 2;

   _fill_cpufreq_states(inf);
   _fill_log_states(inf);
   inf->grid.thread = calloc(1, evlog->thread_num * sizeof(Evas_Object *));

   for (i = 0; i < evlog->thread_num; i++)
     {
        snprintf(buf, sizeof(buf), "<b>%i</b>", i + 1);

        o = elm_label_add(inf->main);
        elm_object_text_set(o, buf);
        evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
        elm_table_pack(inf->table, o, 0, y, 1, 1);
        evas_object_show(o);

        o = _create_log_thread(inf, evlog, &(evlog->threads[i]), i);
        inf->grid.thread[i] = o;
        evas_object_size_hint_weight_set(o, 0.0, 0.0);
        evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
        elm_table_pack(inf->table, o, 2, y++, 1, 1);
        evas_object_show(o);
     }

   o = elm_separator_add(inf->main);
   evas_object_size_hint_weight_set(o, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, 0.5, EVAS_HINT_FILL);
   elm_table_pack(inf->table, o, 1, 0, 1, y);
   evas_object_show(o);

   o = elm_grid_add(inf->main);
   inf->grid.over = o;
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_grid_size_set(o, (evlog->last_timestamp - evlog->first_timestamp) * RES, 1);
   elm_table_pack(inf->table, o, 2, 0, 1, y);
   evas_object_show(o);
}

static void
_cb_fill_blocking(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Inf *inf = data;
   int i;

   _fill_event_clean(inf,
                     inf->update.t0 - inf->evlog->first_timestamp,
                     inf->update.t1 - inf->evlog->first_timestamp,
                     inf->update.tmin);

   _fill_timed_log_states(inf, inf->evlog,
                    inf->update.t0 - inf->evlog->first_timestamp,
                    inf->update.t1 - inf->evlog->first_timestamp,
                    inf->update.tmin);

   for (i = 0; i < inf->evlog->thread_num; i++)
     {
        _fill_log_thread(inf, inf->evlog, &(inf->evlog->threads[i]), i + 1,
                         inf->update.t0 - inf->evlog->first_timestamp,
                         inf->update.t1 - inf->evlog->first_timestamp,
                         inf->update.tmin);
     }
}

static Evas_Object *
_add_log_state_object(Evas_Object *main, Evas_Object *grid, Event *ev)
{
   Evas_Object *o, *oe;
   unsigned char col[4] = {255, 255, 255, 255};
   int i;
   char *s;
   char buf[512];

   o = elm_layout_add(main);
   oe = elm_layout_edje_get(o);
   elm_layout_file_set(o, "./evlog.edj", "state");
   i = 0;
   for (s = ev->event; *s; s++)
     {
        col[i % 3] ^= *s;
        i++;
     }
   if (ev->detail)
     {
        for (s = ev->detail; *s; s++)
          {
             col[i % 3] ^= ((*s << 3) | (i << 1));
             i++;
          }
     }
   edje_object_color_class_set(oe, "state",
                               col[0] / 2, col[1] / 2, col[2] / 2, col[3],
                               255, 255, 255, 255,
                               255, 255, 255, 255);
   if (ev->detail)
     {
        snprintf(buf, sizeof(buf), "%s (%s)", ev->event, ev->detail);
        edje_object_part_text_set(oe, "text", buf);
     }
   else
     edje_object_part_text_set(oe, "text", ev->event);
   elm_grid_pack(grid, o, ev->t0 * RES, ev->n, (ev->t1  - ev->t0) * RES, 1);
   if (ev->detail)
     snprintf(buf, sizeof(buf), "%s (%s) - %1.5fms [%1.5fms]", ev->event, ev->detail, ev->t0 * 1000.0, (ev->t1 - ev->t0) * 1000.0);
   else
     snprintf(buf, sizeof(buf), "%s - %1.5fms [%1.5fms]", ev->event, ev->t0 * 1000.0, (ev->t1 - ev->t0) * 1000.0);
   elm_object_tooltip_text_set(o, buf);
   evas_object_show(o);
   return o;
}

static Evas_Object *
_add_log_event_object(Evas_Object *main, Evas_Object *grid, Event *ev)
{
   Evas_Object *o, *oe;
   int col[4] = {255, 255, 255, 255}, i;
   char *s;
   char buf[512];

   o = elm_layout_add(main);
   oe = elm_layout_edje_get(o);
   elm_layout_file_set(o, "./evlog.edj", "range");
   i = 0;
   for (s = ev->event; *s; s++)
     {
        col[i % 3] ^= *s;
        i++;
     }
   edje_object_color_class_set(oe, "range",
                               col[0] / 2, col[1] / 2, col[2] / 2, col[3],
                               255, 255, 255, 255,
                               255, 255, 255, 255);
   if (ev->detail)
     {
        snprintf(buf, sizeof(buf), "%s (%s)", ev->event, ev->detail);
        edje_object_part_text_set(oe, "text", buf);
     }
   else
     edje_object_part_text_set(oe, "text", ev->event);
   elm_grid_pack(grid, o, ev->t0 * RES, 1 + (ev->n * 2), (ev->t1  - ev->t0) * RES, 2);
   if (ev->detail)
     snprintf(buf, sizeof(buf), "%s (%s) - %1.5fms [%1.5fms]", ev->event, ev->detail, ev->t0 * 1000.0, (ev->t1 - ev->t0) * 1000.0);
   else
     snprintf(buf, sizeof(buf), "%s - %1.5fms [%1.5fms]", ev->event, ev->t0 * 1000.0, (ev->t1 - ev->t0) * 1000.0);
   elm_object_tooltip_text_set(o, buf);
   evas_object_show(o);
   return o;
}

static Evas_Object *
_add_log_cpuused_object(Evas_Object *main, Evas_Object *grid, Event *ev)
{
   Evas_Object *o, *oe;
   int col[4] = {0, 0, 0, 255};
   char buf[512];

   o = elm_layout_add(main);
   oe = elm_layout_edje_get(o);
   elm_layout_file_set(o, "./evlog.edj", "cpuused");
   if (ev->n <= 33)
     {
        col[0] = (ev->n * 255) / 33;
     }
   else if (ev->n <= 67)
     {
        col[0] = 255;
        col[1] = ((ev->n - 33) * 255) / 24;
     }
   else
     {
        col[0] = 255;
        col[1] = 255;
        col[2] = ((ev->n - 67) * 255) / 33;
     }
   edje_object_color_class_set(oe, "range",
                               col[0], col[1], col[2], col[3],
                               255, 255, 255, 255,
                               255, 255, 255, 255);
   elm_grid_pack(grid, o, ev->t0 * RES, 0, (ev->t1  - ev->t0) * RES, 1);
   if (ev->detail)
     snprintf(buf, sizeof(buf), "%i%% - %1.5fms [%1.5fms]", ev->n, ev->t0 * 1000.0, (ev->t1 - ev->t0) * 1000.0);
   elm_object_tooltip_text_set(o, buf);
   evas_object_show(o);
   return o;
}

static Evas_Object *
_add_log_frame_object(Evas_Object *main, Evas_Object *grid, Event *ev)
{
   Evas_Object *o;
   char buf[512];

   o = elm_layout_add(main);
   elm_layout_file_set(o, "./evlog.edj", "frame");
   elm_grid_pack(grid, o, ev->t0 * RES, ev->n, 0, 1);
   snprintf(buf, sizeof(buf), "%s - %1.5fms", ev->event, ev->t0 * 1000.0);
   elm_object_tooltip_text_set(o, buf);
   evas_object_show(o);
   return o;
}

static Evas_Object *
_add_log_event_event_object(Evas_Object *main, Evas_Object *grid, Event *ev)
{
   Evas_Object *o, *oe;
   int col[4] = {255, 255, 255, 255}, i, max;
   char *s;
   char buf[512];

   o = elm_layout_add(main);
   oe = elm_layout_edje_get(o);
   elm_layout_file_set(o, "./evlog.edj", "event");
   i = 0;
   max = 0;
   for (s = ev->event; *s; s++)
     {
        col[i % 3] ^= *s;
        if (col[i % 3] > max) max = col[i % 3];
        i++;
     }
   if (max > 0)
     {
        for (i = 0; i < 3; i++)
          {
             col[i] = (col[i] * 255) / max;
          }
     }
   edje_object_color_class_set(oe, "event",
                               (3 * col[0]) / 4, (3 * col[1]) / 4, (3 * col[2]) / 4, (3 * col[3]) / 4,
                               255, 255, 255, 255,
                               255, 255, 255, 255);
   if (ev->detail)
     {
        snprintf(buf, sizeof(buf), "%s (%s)", ev->event, ev->detail);
        edje_object_part_text_set(oe, "text", buf);
     }
   else
     edje_object_part_text_set(oe, "text", ev->event);
   elm_grid_pack(grid, o, ev->t0 * RES, ev->n, 0, 1);
   if (ev->detail)
     snprintf(buf, sizeof(buf), "%s (%s) - %1.5fms", ev->event, ev->detail, ev->t0 * 1000.0);
   else
     snprintf(buf, sizeof(buf), "%s - %1.5fms", ev->event, ev->t0 * 1000.0);
   elm_object_tooltip_text_set(o, buf);
   evas_object_show(o);
   return o;
}

static Evas_Object *
_add_log_cpufreq_object(Evas_Object *parent, Evas_Object *grid, Event *ev, int mhzmax)
{
   Evas_Object *o, *oe;
   int col[4] = {0, 0, 0, 255}, n;
   char buf[512];

   o = elm_layout_add(parent);
   oe = elm_layout_edje_get(o);
   elm_layout_file_set(o, "./evlog.edj", "cpufreq");

   n = (ev->n2 * 100) / mhzmax;
   if (n <= 33)
     {
        col[0] = (n * 255) / 33;
     }
   else if (n <= 67)
     {
        col[0] = 255;
        col[1] = ((n - 33) * 255) / 24;
     }
   else
     {
        col[0] = 255;
        col[1] = 255;
        col[2] = ((n - 67) * 255) / 33;
     }
   edje_object_color_class_set(oe, "range",
                               col[0], col[1], col[2], col[3],
                               255, 255, 255, 255,
                               255, 255, 255, 255);
   elm_grid_pack(grid, o, ev->t0 * RES, ev->n, (ev->t1  - ev->t0) * RES, 1);
   if (ev->detail)
     snprintf(buf, sizeof(buf), "%iMHz - %1.5fms [%1.5fms]", ev->n2, ev->t0 * 1000.0, (ev->t1 - ev->t0) * 1000.0);
   elm_object_tooltip_text_set(o, buf);
   evas_object_show(o);
   return o;
}

static void
_cb_fill_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Inf *inf = data;
   Event *ev;
   Evas_Object *o;
   Eina_List *l;

   inf->update.thread = NULL;
   EINA_LIST_FOREACH(inf->objs, l, ev)
     {
        if ((!ev->obj) && (!ev->nuke))
          {
             if (ev->slot == 0) // state
               {
                  o = _add_log_state_object(inf->main,
                                            inf->grid.state,
                                            ev);
                  ev->obj = o;
               }
             else if (ev->slot > 0) // thread
               {
                  if (!strcmp(ev->event, "*CPUUSED"))
                    o = _add_log_cpuused_object(inf->main,
                                                inf->grid.thread[ev->slot - 1],
                                                ev);
                  else
                    o = _add_log_event_object(inf->main,
                                              inf->grid.thread[ev->slot - 1],
                                              ev);
                  ev->obj = o;
               }
             else if (ev->slot == -1) // frames
               {
                  if (!strcmp(ev->event, "FRAME"))
                    {
                       o = _add_log_frame_object(inf->main,
                                                 inf->grid.over,
                                                 ev);
                       ev->obj = o;
                    }
                  else
                    {
                       o = _add_log_event_event_object(inf->main,
                                                       inf->grid.over,
                                                       ev);
                       ev->obj = o;
                    }
               }
             else if (ev->slot == -2) // cpufreq
               {
                  if (!strcmp(ev->event, "*CPUFREQ"))
                    {
                       o = _add_log_cpufreq_object(inf->main,
                                                   inf->grid.cpufreq,
                                                   ev,
                                                   inf->evlog->cpumhzmax);
                       ev->obj = o;
                    }
               }
          }
     }
   EINA_LIST_FREE(inf->update.remobjs, ev)
     {
        inf->objs = eina_list_remove(inf->objs, ev);
        if (ev->obj) evas_object_del(ev->obj);
        free(ev->event);
        free(ev->detail);
        free(ev);
     }
   if (inf->update.redo)
     {
        inf->update.redo = EINA_FALSE;
        _fill_begin(inf);
     }
}

static void
_cb_fill_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Inf *inf = data;
   inf->update.thread = NULL;
}

static void
_fill_begin(Inf *inf)
{
   Evas_Coord x, w, wx, ww;
   double t, t0, t1;
   Evas_Coord gw;

   if (!inf->evlog) return;
   if (inf->update.thread)
     {
        inf->update.redo = EINA_TRUE;
        return;
     }
   elm_grid_size_get(inf->grid.state, &gw, NULL);
   evas_object_geometry_get(inf->grid.state, &x, NULL, &w, NULL);
   evas_output_viewport_get(evas_object_evas_get(inf->grid.state),
                            &wx, NULL, &ww, NULL);
   if (ww < 1) ww = 1;
   if (w < 1) w = 1;
   if (w < ww) w = ww;
   if (gw < 1) gw = 1;

   t0 = inf->evlog->first_timestamp;
   t1 = inf->evlog->last_timestamp;

   inf->update.tmin = (3.0 * (double)gw) / ((double)w * RES);

   wx -= 100;
   ww += 200;

   t = ((t1 - t0) * (double)ww) / ((double)w);
   t0 = ((wx - x) * (t1 - t0)) / ((double)w);
   if (t0 < 0.0) t0 = 0.0;
   inf->update.t0 = t0 + inf->evlog->first_timestamp;
   inf->update.t1 = inf->update.t0 + t;
   inf->update.thread = ecore_thread_run(_cb_fill_blocking,
                                         _cb_fill_end,
                                         _cb_fill_cancel,
                                         inf);
}

static void
_cb_fill_job(void *data)
{
   Inf *inf = data;
   inf->update.job = NULL;
   _fill_begin(inf);
}

static void
_fill_update(Inf *inf)
{
   if (inf->update.job) ecore_job_del(inf->update.job);
   inf->update.job = ecore_job_add(_cb_fill_job, inf);
}

static void
_cb_zoom(void *data, Evas_Object *obj, void *info EINA_UNUSED)
{
   Inf *inf = data;
   Evas_Coord w = 0, h = 1;
   evas_object_size_hint_min_get(inf->grid.state, &w, &h);
   if (w < 1) w = 1;
   double len = inf->evlog->last_timestamp - inf->evlog->first_timestamp;
   double res = elm_slider_value_get(obj);
   Evas_Coord sx, sy, sw, sh;
   elm_scroller_region_get(inf->scroller, &sx, &sy, &sw, &sh);
   res = res * res;
   evas_object_size_hint_min_set(inf->grid.state, len * res, h);
   double snx = ((double)(sx + (sw / 2)) * (len * res)) / (double)w;
   double snw = ((double)(sw) * (len * res)) / (double)w;
   elm_scroller_region_show(inf->scroller, snx - (snw / 2), sy, snw, sh);
}

static void
_cb_move(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   _fill_update(data);
}

static void
_cb_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   _fill_update(data);
}

static void
_cb_sc_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   _fill_update(data);
}

static Evas_Object *
_evlog_view_add(Inf *inf)
{
   Evas_Object *sc, *tb;

   sc = elm_scroller_add(inf->main);
   inf->scroller = sc;
   evas_object_event_callback_add(sc, EVAS_CALLBACK_RESIZE, _cb_sc_resize, inf);

   tb = elm_table_add(inf->main);
   inf->table = tb;
   evas_object_event_callback_add(tb, EVAS_CALLBACK_MOVE, _cb_move, inf);
   evas_object_event_callback_add(tb, EVAS_CALLBACK_RESIZE, _cb_resize, inf);

   _create_log_table(inf);

   elm_object_content_set(sc, tb);
   evas_object_show(tb);

   evas_object_smart_callback_add(inf->zoom_slider, "changed", _cb_zoom, inf);
   return sc;
}

static void
_evlog_import(Clouseau_Extension *ext, char *buffer, int size)
{
   Evlog *evlog = calloc(1, sizeof(Evlog));
   Inf *inf = ext->data;
   char *p = buffer;
   int consumed = -1;
   while (size && (consumed = _evlog_block_read(evlog, p, size)) != -1)
     {
        p += consumed;
        size -= consumed;
     }
   free(buffer);
   if (size)
     {
        free(evlog);
        return;
     }
   inf->evlog = evlog;
   _fill_log_table(inf);
}

static Eo *
_ui_get(Clouseau_Extension *ext, Eo *parent)
{
   Eo *box;
   Eo *zoom_slider;
   Eo *view;
   Inf *inf = ext->data;

   box = elm_box_add(parent);
   inf->main = box;
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_horizontal_set(box, EINA_FALSE);
   evas_object_show(box);

   zoom_slider = elm_slider_add(box);
   inf->zoom_slider = zoom_slider;
   evas_object_data_set(zoom_slider, "inf", inf);
   elm_slider_min_max_set(zoom_slider, 1.0, 1000.0);
   elm_slider_step_set(zoom_slider, 0.1);
   elm_slider_value_set(zoom_slider, 50.0);
   elm_object_text_set(zoom_slider, "Zoom");
   elm_slider_unit_format_set(zoom_slider, "%1.1f");
   evas_object_size_hint_weight_set(zoom_slider, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(zoom_slider, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(box, zoom_slider);
   evas_object_show(zoom_slider);

   view = _evlog_view_add(ext->data);
   evas_object_size_hint_weight_set(view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(view, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, view);
   evas_object_show(view);

   return box;
}

EAPI const char *
extension_name_get()
{
   return "Event log";
}

EAPI Eina_Bool
extension_start(Clouseau_Extension *ext, Eo *parent)
{
   Inf *inf;

   eina_init();

   inf = calloc(1, sizeof(Inf));
   ext->data = inf;
//   ext->session_changed_cb = _session_changed;
//   ext->app_changed_cb = _app_changed;
   ext->import_data_cb = _evlog_import;

   ext->ui_object = _ui_get(ext, parent);
   return !!ext->ui_object;
}

EAPI Eina_Bool
extension_stop(Clouseau_Extension *ext)
{
   Inf *inf = ext->data;

   efl_del(ext->ui_object);

   free(inf);

   eina_shutdown();

   return EINA_TRUE;
}
