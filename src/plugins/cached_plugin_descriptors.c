/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/cached_plugin_descriptors.h"
#include "utils/file.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#define CACHED_PLUGIN_DESCRIPTORS_VERSION 8

static char *
get_cached_plugin_descriptors_file_path (void)
{
  char * zrythm_dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_TOP);
  g_return_val_if_fail (zrythm_dir, NULL);

  return
    g_build_filename (
      zrythm_dir,
      "cached_plugin_descriptors.yaml", NULL);
}

void
cached_plugin_descriptors_serialize_to_file (
  CachedPluginDescriptors * self)
{
  self->version = CACHED_PLUGIN_DESCRIPTORS_VERSION;
  g_message (
    "Serializing cached plugin descriptors...");
  char * yaml =
    cached_plugin_descriptors_serialize (self);
  g_return_if_fail (yaml);
  GError *err = NULL;
  char * path =
    get_cached_plugin_descriptors_file_path ();
  g_return_if_fail (path && strlen (path) > 2);
  g_message (
    "Writing cached plugin descriptors to %s...",
    path);
  g_file_set_contents (
    path, yaml, -1, &err);
  if (err != NULL)
    {
      g_warning (
        "Unable to write cached plugin descriptors "
        "file: %s",
        err->message);
      g_error_free (err);
      g_free (path);
      g_free (yaml);
      g_return_if_reached ();
    }
  g_free (path);
  g_free (yaml);
}

/**
 * Reads the file and fills up the object.
 */
CachedPluginDescriptors *
cached_plugin_descriptors_new (void)
{
  GError *err = NULL;
  char * path =
    get_cached_plugin_descriptors_file_path ();
  if (!file_exists (path))
    {
      g_message (
        "Cached plugin descriptors file at %s does "
        "not exist", path);
return_new_instance:
      return
        calloc (1, sizeof (CachedPluginDescriptors));
    }
  char * yaml = NULL;
  g_file_get_contents (path, &yaml, NULL, &err);
  if (err != NULL)
    {
      g_critical (
        "Failed to create CachedPluginDescriptors "
        "from %s", path);
      g_free (err);
      g_free (yaml);
      g_free (path);
      return NULL;
    }

  /* if not same version, purge file and return
   * a new instance */
  char version_str[120];
  sprintf (
    version_str, "version: %d",
    CACHED_PLUGIN_DESCRIPTORS_VERSION);
  if (!g_str_has_prefix (yaml, version_str))
    {
      g_message (
        "Found old plugin descriptor file version. "
        "Purging file and creating a new one.");
      GFile * file =
        g_file_new_for_path (path);
      g_file_delete (file, NULL, NULL);
      g_object_unref (file);
      goto return_new_instance;
    }

  CachedPluginDescriptors * self =
    cached_plugin_descriptors_deserialize (yaml);
  if (!self)
    {
      g_critical (
        "Failed to deserialize "
        "CachedPluginDescriptors from %s", path);
      g_free (err);
      g_free (yaml);
      g_free (path);
      return NULL;
    }
  g_free (yaml);
  g_free (path);

  for (int i = 0; i < self->num_descriptors; i++)
    {
      self->descriptors[i]->category =
        plugin_descriptor_string_to_category (
          self->descriptors[i]->category_str);
    }

  return self;
}

static void
delete_file (void)
{
  char * path =
    get_cached_plugin_descriptors_file_path ();
  GFile * file =
    g_file_new_for_path (path);
  if (!g_file_delete (file, NULL, NULL))
    {
      g_warning (
        "Failed to delete cached plugin descriptors "
        "file");
    }
  g_free (path);
  g_object_unref (file);
}

/**
 * Returns if the plugin at the given path is
 * blacklisted or not.
 */
int
cached_plugin_descriptors_is_blacklisted (
  CachedPluginDescriptors * self,
  const char *           abs_path)
{
  for (int i = 0; i < self->num_blacklisted; i++)
    {
      PluginDescriptor * descr =
        self->blacklisted[i];
      GFile * file =
        g_file_new_for_path (descr->path);
      if (string_is_equal (descr->path, abs_path) &&
          descr->ghash == g_file_hash (file))
        {
          g_object_unref (file);
          return 1;
        }
      g_object_unref (file);
    }
  return 0;
}

/**
 * Returns the PluginDescriptor's corresponding to
 * the .so/.dll file at the given path, if it
 * exists and the MD5 hash matches.
 *
 * @note The returned array must be free'd but not
 *   the descriptors.
 *
 * @return NULL-terminated array.
 */
PluginDescriptor **
cached_plugin_descriptors_get (
  CachedPluginDescriptors * self,
  const char *              abs_path)
{
  PluginDescriptor ** descriptors =
    calloc (1, sizeof (PluginDescriptor *));
  int num_descriptors = 0;

  for (int i = 0; i < self->num_descriptors; i++)
    {
      PluginDescriptor * descr =
        self->descriptors[i];
      GFile * file =
        g_file_new_for_path (descr->path);
      if (string_is_equal (descr->path, abs_path) &&
          descr->ghash == g_file_hash (file))
        {
          num_descriptors++;
          descriptors =
            realloc (
              descriptors,
              (size_t) (num_descriptors + 1) *
                sizeof (PluginDescriptor *));
          descriptors[num_descriptors - 1] = descr;
        }
      g_object_unref (file);
    }

  if (num_descriptors == 0)
    {
      free (descriptors);
      return NULL;
    }

  /* NULL-terminate */
  descriptors[num_descriptors] = NULL;

  return descriptors;
}

/**
 * Appends a descriptor to the cache.
 *
 * @param serialize 1 to serialize the updated cache
 *   now.
 */
void
cached_plugin_descriptors_blacklist (
  CachedPluginDescriptors * self,
  const char *           abs_path,
  int                    _serialize)
{
  g_return_if_fail (abs_path && self);

  PluginDescriptor * new_descr =
    calloc (1, sizeof (PluginDescriptor));
  new_descr->path = g_strdup (abs_path);
  GFile * file = g_file_new_for_path (abs_path);
  new_descr->ghash = g_file_hash (file);
  g_object_unref (file);
  self->blacklisted[self->num_blacklisted++] =
    new_descr;
  if (_serialize)
    {
      cached_plugin_descriptors_serialize_to_file (
        self);
    }
}

/**
 * Appends a descriptor to the cache.
 *
 * @param serialize 1 to serialize the updated cache
 *   now.
 */
void
cached_plugin_descriptors_add (
  CachedPluginDescriptors * self,
  PluginDescriptor *     descr,
  int                    _serialize)
{
  PluginDescriptor * new_descr =
    plugin_descriptor_clone (descr);
  GFile * file = g_file_new_for_path (descr->path);
  new_descr->ghash = g_file_hash (file);
  g_object_unref (file);
  self->descriptors[self->num_descriptors++] =
    new_descr;

  if (_serialize)
    {
      cached_plugin_descriptors_serialize_to_file (
        self);
    }
}

/**
 * Clears the descriptors and removes the cache file.
 */
void
cached_plugin_descriptors_clear (
  CachedPluginDescriptors * self)
{
  for (int i = 0; i < self->num_descriptors; i++)
    {
      plugin_descriptor_free (self->descriptors[i]);
    }
  self->num_descriptors = 0;

  delete_file ();
}

void
cached_plugin_descriptors_free (
  CachedPluginDescriptors * self)
{
  for (int i = 0; i < self->num_descriptors; i++)
    {
      object_free_w_func_and_null (
        plugin_descriptor_free,
        self->descriptors[i]);
    }
  for (int i = 0; i < self->num_blacklisted; i++)
    {
      object_free_w_func_and_null (
        plugin_descriptor_free,
        self->blacklisted[i]);
    }
}

SERIALIZE_SRC (
  CachedPluginDescriptors, cached_plugin_descriptors);
DESERIALIZE_SRC (
  CachedPluginDescriptors, cached_plugin_descriptors);
