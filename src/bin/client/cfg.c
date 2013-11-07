#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eet.h>
#include <Efreet.h>

#include "cfg.h"
#include "Clouseau.h"

Clouseau_Cfg *_clouseau_cfg = NULL;
static Eet_Data_Descriptor * _clouseau_cfg_descriptor;
#define _CONFIG_ENTRY "config"

static char *config_file = NULL;

static void
_clouseau_cfg_descriptor_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Clouseau_Cfg);
   _clouseau_cfg_descriptor = eet_data_descriptor_stream_new(&eddc);

#define CFG_ADD_BASIC(member, eet_type)\
   EET_DATA_DESCRIPTOR_ADD_BASIC\
      (_clouseau_cfg_descriptor, Clouseau_Cfg, # member, member, eet_type)

   CFG_ADD_BASIC(version, EET_T_UINT);
   CFG_ADD_BASIC(show_hidden, EET_T_UCHAR);
   CFG_ADD_BASIC(show_clippers, EET_T_UCHAR);
   CFG_ADD_BASIC(show_elm_only, EET_T_UCHAR);

#undef CFG_ADD_BASIC
}

static void
_clouseau_cfg_descriptor_shutdown(void)
{
   eet_data_descriptor_free(_clouseau_cfg_descriptor);
}

void
clouseau_cfg_shutdown(void)
{
   if (config_file)
      free(config_file);

   _clouseau_cfg_descriptor_shutdown();

   eet_shutdown();
   efreet_shutdown();
}

void
clouseau_cfg_init(const char *file)
{
   const char *ext = ".cfg";
   const char *path;
   size_t len;

   efreet_init();
   eet_init();

   path = efreet_config_home_get();
   if (!path || !file)
      return;

   if (config_file)
      free(config_file);

   len = strlen(path) + strlen(file) + strlen(ext) + 1; /* +1 for '/' */

   config_file = malloc(len + 1);
   snprintf(config_file, len + 1, "%s/%s%s", path, file, ext);

   _clouseau_cfg_descriptor_init();
}

static Clouseau_Cfg *
_clouseau_cfg_new(void)
{
   Clouseau_Cfg *ret;
   ret = calloc(1, sizeof(*ret));

   ret->version = _CLOUSEAU_CFG_VERSION;
   /* Default values */
   ret->show_elm_only = EINA_TRUE;
   ret->show_clippers = EINA_TRUE;
   ret->show_hidden = EINA_TRUE;

   return ret;
}

/* Return false on error. */
Eina_Bool
clouseau_cfg_load(void)
{
   Eet_File *ef;

   if (!config_file)
      goto end;

   ef = eet_open(config_file, EET_FILE_MODE_READ);
   if (!ef)
     {
        /* FIXME Info message? create new config? */
        goto end;
     }

   _clouseau_cfg = eet_data_read(ef, _clouseau_cfg_descriptor, _CONFIG_ENTRY);

end:
   if (!_clouseau_cfg)
     {
        _clouseau_cfg = _clouseau_cfg_new();
     }

   eet_close(ef);
   return EINA_TRUE;
}

/* Return false on error. */
Eina_Bool
clouseau_cfg_save(void)
{
   Eet_File *ef;
   Eina_Bool ret;

   if (!config_file)
      return EINA_FALSE;


   ef = eet_open(config_file, EET_FILE_MODE_WRITE);
   if (!ef)
     {
        EINA_LOG_ERR("could not open '%s' for writing.", config_file);
        return EINA_FALSE;
     }

   ret = eet_data_write
         (ef, _clouseau_cfg_descriptor, _CONFIG_ENTRY, _clouseau_cfg, EINA_TRUE);
   eet_close(ef);

   return ret;
}

