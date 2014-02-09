/*
 * notmuch-addrlookup.c
 * Copyright (C) 2014 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <glib.h>
#include <notmuch.h>
#include <stdlib.h>
#include <string.h>

#define NUM_QUERIES  3

static gchar* notmuch_user_abook_tag = NULL;
static gchar* notmuch_database_path = NULL;
static gchar* notmuch_user_email = NULL;

typedef struct {
  gchar *name;
  guint  occurrences[NUM_QUERIES];
} ContactInfo;


/*
 * Note: Takes ownership of the passed string
 */
static ContactInfo*
contact_info_new (gchar* name)
{
  ContactInfo* info =  g_slice_new0 (ContactInfo);
  info->name = name;
  return info;
}


static void
contact_info_free (ContactInfo *info)
{
  if (info == NULL)
    return;

  g_free (info->name);
  g_slice_free (ContactInfo, info);
}


static int
sort_by_frequency (gconstpointer data1,
                   gconstpointer data2)
{
  const ContactInfo *info1 = (ContactInfo*) data1;
  const ContactInfo *info2 = (ContactInfo*) data2;

  if (info1->occurrences[0] == info2->occurrences[0] &&
      info1->occurrences[1] == info2->occurrences[1] &&
      info1->occurrences[2] == info2->occurrences[2])
    return 0;

  if ((info1->occurrences[0] <  info2->occurrences[0]) ||
      (info1->occurrences[0] == info2->occurrences[0] &&
       info1->occurrences[1] <  info2->occurrences[1]) ||
      (info1->occurrences[0] == info2->occurrences[0] &&
       info1->occurrences[1] == info2->occurrences[1] &&
       info1->occurrences[2] <  info2->occurrences[2]))
    return -1;

  return 1;
}


static gboolean
load_notmuch_settings (void)
{
  GKeyFile *key_file = g_key_file_new ();
  gchar *config_path = NULL;
  gboolean success = FALSE;

  if (g_getenv ("NOTMUCH_CONFIG"))
    config_path = g_strdup (g_getenv ("NOTMUCH_CONFIG"));
  else
    config_path = g_strdup_printf ("%s/.notmuch-config", g_get_home_dir ());

  if (!g_key_file_load_from_file (key_file,
                                  config_path,
                                  G_KEY_FILE_NONE,
                                  NULL))
    goto bailout;

  notmuch_database_path = g_key_file_get_string (key_file,
                                                 "database",
                                                 "path",
                                                 NULL);

  notmuch_user_email = g_key_file_get_string (key_file,
                                              "user",
                                              "primary_email",
                                              NULL);

  notmuch_user_abook_tag = g_key_file_get_string (key_file,
                                                  "user",
                                                  "addrbook_tag",
                                                  NULL);

  if (!notmuch_user_abook_tag)
    notmuch_user_abook_tag = g_strdup ("addressbook");

  success = (notmuch_database_path != NULL);

bailout:
  g_key_file_free (key_file);
  g_free (config_path);
  return success;
}


static void
global_cleanup (void)
{
  g_free (notmuch_user_abook_tag);
  g_free (notmuch_database_path);
  g_free (notmuch_user_email);
}


static void
create_queries (notmuch_database_t *db,
                notmuch_query_t    *queries[NUM_QUERIES],
                const gchar        *name)
{
  /* Pass 1: looks at all the "from:" addresses with the address book tag */
  GString *qs = g_string_new ("tag:");
  g_string_append (qs, notmuch_user_abook_tag);
  if (name)
    g_string_append_printf (qs, " and from:%s*", name);
  gchar *sbuf = g_string_free (qs, FALSE);
  queries[0] = notmuch_query_create (db, sbuf);
  g_free (sbuf);

  /* Pass 2: looks at all "to:" addresses sent from the primary e-mail */
  qs = g_string_new ("");
  if (name)
    g_string_append_printf (qs, "to:%s*", name);
  if (notmuch_user_email)
    g_string_append_printf (qs, " from:%s", notmuch_user_email);
  sbuf = g_string_free (qs, FALSE);
  queries[1] = notmuch_query_create (db, sbuf);
  g_free (sbuf);

  /* Pass 3: With only a few hits, add all the "from:" addresses */
  if (notmuch_query_count_messages (queries[0]) +
      notmuch_query_count_messages (queries[1]) < 10)
    {
      qs = g_string_new ("");
      if (name)
        g_string_append_printf (qs, "from:%s*", name);
      sbuf = g_string_free (qs, FALSE);
      queries[2] = notmuch_query_create (db, sbuf);
      g_free (sbuf);
    }
}


static void
print_contact_info_name (gpointer data, gpointer userdata)
{
    g_print ("%s\n", ((ContactInfo*) data)->name);
}


static void
run_queries (notmuch_database_t *db,
             notmuch_query_t    *queries[NUM_QUERIES],
             const gchar        *name)
{
  static const gchar *headers_pass0[] = { "from", NULL };
  static const gchar *headers_pass1[] = { "to", "cc", "bcc", NULL };

  GHashTable *frequencies = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   (GDestroyNotify) contact_info_free);

  GRegex *match_re = g_regex_new ("\\s*((\\\"(\\\\.|[^\\\\\"])*\\\"|[^,])*"
                                  "<?(?P<mail>\\b\\w+([-+.]\\w+)*\\@\\w+[-\\.\\w]*\\.([-\\.\\w]+)*\\w\\b)>?)",
                                  G_REGEX_OPTIMIZE, 0, NULL);

  for (guint i = 0; i < NUM_QUERIES; i++)
    {
      if (!queries[i])
        continue;

      const gchar **headers = (i == 1) ? headers_pass1 : headers_pass0;

      for (notmuch_messages_t *messages = notmuch_query_search_messages (queries[i]);
           notmuch_messages_valid (messages);
           notmuch_messages_move_to_next (messages))
        {
          notmuch_message_t *msg = notmuch_messages_get (messages);

          for (guint i = 0; headers[i] != NULL; i++)
            {
              const gchar* froms = notmuch_message_get_header (msg, headers[i]);

              GMatchInfo *matches;
              g_regex_match (match_re, froms, 0, &matches);
              while (g_match_info_matches (matches))
                {
                  gchar* from = g_match_info_fetch (matches, 1);
                  gchar* addr = g_match_info_fetch_named (matches, "mail");
                  gchar* patt = g_strdup_printf ("\\b%s", name);

                  if (g_regex_match_simple (patt, from, G_REGEX_CASELESS, 0))
                    {
                      gchar* addr_fold = g_utf8_casefold (addr, -1);
                      gchar* addr_key = g_utf8_collate_key (addr_fold, -1);
                      g_free (addr_fold);

                      ContactInfo *info = (ContactInfo*) g_hash_table_lookup (frequencies, addr_key);
                      if (info == NULL)
                        {
                          info = contact_info_new (from);
                          g_hash_table_insert (frequencies, addr_key, info);

                          /*
                           * The ContactInfo takes the "from" string, and
                           * the hash table takes ownership of the key. Set
                           * them NULL to avoid the g_free() calls below to
                           * free the memory.
                           */
                          addr_key = from = NULL;
                        }
                      info->occurrences[i]++;

                      g_free (addr_key);
                    }

                  g_free (addr);
                  g_free (patt);
                  g_free (from);

                  g_match_info_next (matches, NULL);
                }
            }
          notmuch_message_destroy (msg);
        }
      notmuch_query_destroy (queries[i]);
    }

  g_regex_unref (match_re);

  GList *addrs = g_hash_table_get_values (frequencies);
  addrs = g_list_sort (addrs, sort_by_frequency);
  g_list_foreach (addrs, print_contact_info_name, NULL);
  g_list_free (addrs);

  g_hash_table_destroy (frequencies);
}


int
main (int argc, char **argv)
{
  if (argc != 1 && argc != 2)
    {
      g_printerr ("Usage: %s [address prefix]\n", argv[0]);
      return EXIT_FAILURE;
    }

  atexit (global_cleanup);

  if (!load_notmuch_settings ())
    {
      g_printerr ("Could not load Notmuch settings (~/.notmuch-config missing?)\n");
      return EXIT_FAILURE;
    }

  notmuch_database_t *db;
  notmuch_status_t status = notmuch_database_open (notmuch_database_path,
                                                   NOTMUCH_DATABASE_MODE_READ_ONLY,
                                                   &db);

  if (status != NOTMUCH_STATUS_SUCCESS)
    {
      g_printerr ("Could not open Notmuch database: %s\n",
                  notmuch_status_to_string (status));
      return EXIT_FAILURE;
    }

  notmuch_query_t *queries[NUM_QUERIES] = { NULL, NULL, NULL };
  const gchar *name = (argc == 2) ? argv[1] : NULL;

  create_queries (db, queries, name);
  run_queries (db, queries, name);

  notmuch_database_close (db);
  return EXIT_SUCCESS;
}

