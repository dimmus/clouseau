#include "clouseau_private.h"

static int _clouseau_init_count = 0;

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
