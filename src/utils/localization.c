/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Localization utils.
 *
 */

#include "config.h"

#include <locale.h>

#include "settings/settings.h"
#include "utils/localization.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#ifdef _WOE32
#define CODESET "1252"
#else
#define CODESET "UTF-8"
#endif

#ifdef MAC_RELEASE
#include "CoreFoundation/CoreFoundation.h"
#include <unistd.h>
#include <libgen.h>
#endif

static char *
get_match (
 char **      installed_locales,
 int          num_installed_locales,
 const char * prefix,
 const char * suffix)
{
  for (int i = 0; i < num_installed_locales; i++)
    {
      if (
        g_str_match_string (
          prefix, installed_locales[i], 0) &&
        g_str_match_string (
          suffix, installed_locales[i], 0))
        {
          return installed_locales[i];
        }
    }
  return NULL;
}

/**
 * Returns the first locale found matching the given
 * language, or NULL if a locale for the given
 * language does not exist.
 *
 * Must be free'd with g_free().
 */
char *
localization_locale_exists (
  LocalizationLanguage lang)
{
#ifdef _WOE32
  const char * _code =
    localization_get_string_code (lang);
  return g_strdup (_code);
#endif

  /* get available locales on the system */
  FILE *fp;
  char path[1035];
  char * installed_locales[8000];
  int num_installed_locales = 0;

  /* Open the command for reading. */
  fp = popen ("locale -a", "r");
  if (fp == NULL)
    {
      g_error ("localization_init: popen failed");
      exit (1);
    }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    installed_locales[num_installed_locales++] =
      g_strdup_printf (
        "%s", g_strchomp (path));
  }

  /* close */
  pclose (fp);

#define IS_MATCH(caps,code) \
  case LL_##caps: \
    match = \
      get_match ( \
        installed_locales, num_installed_locales, \
        code, CODESET); \
    break;

  char * match = NULL;
  switch (lang)
    {
    IS_MATCH(ENGLISH_UK,"en_GB");
    IS_MATCH(GERMAN,"de_");
    IS_MATCH(FRENCH,"fr_");
    IS_MATCH(ITALIAN,"it_");
    IS_MATCH(NORWEGIAN,"nb_");
    IS_MATCH(SPANISH,"es_");
    IS_MATCH(JAPANESE,"ja_");
    IS_MATCH(PORTUGUESE,"pt_");
    IS_MATCH(RUSSIAN,"ru_");
    IS_MATCH(CHINESE,"zh_");
    default:
      g_warn_if_reached ();
      break;
    }

#undef IS_MATCH

  match = g_strdup (match);
  for (int i = 0; i < num_installed_locales; i++)
    {
      g_free (installed_locales[i]);
    }

  return match;
}

/**
 * Sets the locale to the currently selected one and
 * inits gettext.
 *
 * Returns if a locale for the selected language
 * exists on the system or not.
 */
int
localization_init ()
{
  /* get selected locale */
  GSettings * prefs =
    g_settings_new ("org.zrythm.Zrythm.preferences");
  LocalizationLanguage lang =
    g_settings_get_enum (
      prefs, "language");
  g_object_unref (G_OBJECT (prefs));

  if (lang == LL_ENGLISH)
    {
      setlocale (LC_ALL, "C");
      return 1;
    }

  char * code = localization_locale_exists (lang);
  char * match = setlocale (LC_ALL, code);

  if (match)
    {
      g_message (
        "setting locale to %s (found %s)",
        code, match);
#if defined(_WOE32) || defined(__APPLE__)
      char buf[120];
      sprintf (buf, "LANG=%s", code);
      putenv (buf);
#endif
    }
  else
    {
      g_warning (
        "No locale for \"%s\" is "
        "installed, using default", code);
      setlocale (LC_ALL, "C");
    }

  /* bind text domain */
#if defined(WINDOWS_RELEASE)
  bindtextdomain (GETTEXT_PACKAGE, "share/locale");
#elif defined(MAC_RELEASE)
  CFBundleRef bundle =
    CFBundleGetMainBundle ();
  CFURLRef bundleURL =
    CFBundleCopyBundleURL (bundle);
  char path[PATH_MAX];
  Boolean success =
    CFURLGetFileSystemRepresentation (
      bundleURL, TRUE, (UInt8 *)path, PATH_MAX);
  g_return_val_if_fail (success, 0);
  CFRelease (bundleURL);
  g_message ("bundle path: %s", path);
  char localedir[PATH_MAX];
  sprintf (localedir, "%s/../share/locale", path);
  bindtextdomain (
    GETTEXT_PACKAGE, localedir);
#else
  bindtextdomain (
    GETTEXT_PACKAGE, CONFIGURE_DATADIR "/locale");
#endif

  /* set domain codeset */
  bind_textdomain_codeset (GETTEXT_PACKAGE, CODESET);

  /* set domain */
  textdomain (GETTEXT_PACKAGE);

  g_free (code);

  return match != NULL;
}
