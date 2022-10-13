/*
 * notmuch-addrlookup.c
 * Copyright (C) 2014-2016 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <glib.h>
#include <notmuch.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#define NUM_QUERIES  3

static gchar* notmuch_user_abook_tag = NULL;
static gchar* notmuch_database_path = NULL;
static gchar* notmuch_user_email = NULL;
static gchar** search_terms = NULL;
static gboolean mutt_output = FALSE;
static gchar* notmuch_config_path_lookupvar = NULL;
static gchar* output_format_name = "default";

static const GOptionEntry option_entries[] = {
  { "format", 'f', 0, G_OPTION_ARG_STRING, &output_format_name,
    "Set format output (values: default, mutt, aerc)", NULL },
  { "mutt", 'm', 0, G_OPTION_ARG_NONE, &mutt_output,
    "Format output for Mutt (deprecated, use --format instead)", NULL },
  { "config", 'c', 0, G_OPTION_ARG_STRING, &notmuch_config_path_lookupvar,
    "Path to config file .notmuch-config", NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &search_terms,
    "Search terms", NULL },
  { NULL }
};



typedef struct {
  gchar *mail;
  gchar *name;
  guint  occurrences[NUM_QUERIES];
} ContactInfo;


/*
 * Note: Takes ownership of the passed strings.
 */
static ContactInfo*
contact_info_new (gchar *mail,
                  gchar* name)
{
  ContactInfo* info = g_slice_new0 (ContactInfo);
  info->mail = mail;
  info->name = name;
  return info;
}


static void
contact_info_free (ContactInfo *info)
{
  if (info == NULL)
    return;

  g_free (info->name);
  g_free (info->mail);
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
    return 1;

  return -1;
}


static gboolean
load_notmuch_settings (void)
{
  GKeyFile *key_file = g_key_file_new ();
  gchar *config_path = NULL;
  gboolean success = FALSE;

  if(notmuch_config_path_lookupvar)
    config_path = notmuch_config_path_lookupvar;
  else if (g_getenv ("NOTMUCH_CONFIG"))
    config_path = g_strdup (g_getenv ("NOTMUCH_CONFIG"));
  else if (g_getenv ("NOTMUCH_PROFILE")){
    if (g_getenv("XDG_CONFIG_HOME")){
      //TODO check if file exists
    }
  }
  else
    config_path = g_strdup_printf ("%s/.notmuch-config", g_get_home_dir ());

  g_print("%s\n", config_path);

  if (!g_key_file_load_from_file (key_file,
                                  config_path,
                                  G_KEY_FILE_NONE,
                                  NULL))
    goto bailout;


  if (g_getenv ("NOTMUCH_DATABASE"))
    notmuch_database_path = g_strdup (g_getenv ("NOTMUCH_DATABASE"));
  else if (g_key_file_get_string (key_file, "database", "path", NULL))
    notmuch_database_path = g_key_file_get_string (key_file, "database", "path", NULL);

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
  g_strfreev (search_terms);
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

#if LIBNOTMUCH_CHECK_VERSION(5, 0, 0)
  unsigned int count = 0;
  unsigned int tmp;
  if (notmuch_query_count_messages (queries[0], &tmp) == NOTMUCH_STATUS_SUCCESS)
      count += tmp;
  if (notmuch_query_count_messages (queries[1], &tmp) == NOTMUCH_STATUS_SUCCESS)
      count += tmp;
#elif LIBNOTMUCH_CHECK_VERSION(4, 3, 0)
  unsigned int count = 0;
  unsigned int tmp;
  if (notmuch_query_count_messages_st (queries[0], &tmp) == NOTMUCH_STATUS_SUCCESS)
      count += tmp;
  if (notmuch_query_count_messages_st (queries[1], &tmp) == NOTMUCH_STATUS_SUCCESS)
      count += tmp;
#else
  unsigned int count = notmuch_query_count_messages (queries[0])
                     + notmuch_query_count_messages (queries[1]);
#endif

  /* Pass 3: With only a few hits, add all the "from:" addresses */
  if (count < 10)
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
print_contact_info (gpointer data, gpointer userdata)
{
  ContactInfo *info = (ContactInfo*) data;
  g_print ("%s <%s>\n", info->name, info->mail);
}


static void
print_contact_info_mutt (gpointer data, gpointer userdata)
{
  ContactInfo *info = (ContactInfo*) data;
  g_print ("%s\t%s\n", info->mail, info->name);
}

static void
print_contact_info_aerc (gpointer data, gpointer userdata)
{
  static GRegex *replace_re = NULL;
  if (!replace_re)
      replace_re = g_regex_new ("\\\"?([^\\\"]+)\\\"?", G_REGEX_OPTIMIZE, 0, NULL);

  ContactInfo *info = (ContactInfo*) data;
  g_print ("%s\t%s\n", info->mail,
          g_regex_replace(
              replace_re,
              info->name,
              -1,
              0,
              "\\1",
              G_REGEX_MATCH_ANCHORED,
              NULL)
          );
}

static void print_summary_mutt (gpointer addrs)
{
  g_print ("Searching database ... %u matching entries:\n", g_list_length (addrs));
}

struct PrintFormat {
    const char *name;
    void (*print_item)(gpointer, gpointer);
    void (*print_header)(gpointer addrs);
};


static const struct PrintFormat s_formats[] = {
    { .name = "default", .print_item = print_contact_info },
    { .name = "aerc", .print_item = print_contact_info_aerc },
    { .name = "mutt", .print_item = print_contact_info_mutt, .print_header = print_summary_mutt },
};

static void
run_queries (notmuch_database_t *db,
             notmuch_query_t    *queries[NUM_QUERIES],
             const gchar        *query_name,
             const struct PrintFormat *output_format)
{
  static const gchar *headers_pass0[] = { "from", NULL };
  static const gchar *headers_pass1[] = { "to", "cc", "bcc", NULL };

  GHashTable *frequencies = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   (GDestroyNotify) contact_info_free);

  GRegex *match_re = g_regex_new ("\\s*((?P<name>(\\\"(\\\\.|[^\\\\\"])*\\\"|[^,<])*)"
                                  "<?(?P<mail>\\b\\w+([-+.]\\w+)*\\@\\w+[-\\.\\w]*\\.([-\\.\\w]+)*\\w\\b)>?)",
                                  G_REGEX_OPTIMIZE, 0, NULL);

  for (guint i = 0; i < NUM_QUERIES; i++)
    {
      if (!queries[i])
        continue;

      const gchar **headers = (i == 1) ? headers_pass1 : headers_pass0;
      notmuch_messages_t *messages = NULL;

#if LIBNOTMUCH_CHECK_VERSION(5, 0, 0)
      if (notmuch_query_search_messages (queries[i], &messages) != NOTMUCH_STATUS_SUCCESS)
          continue;
#elif LIBNOTMUCH_CHECK_VERSION(4, 3, 0)
      if (notmuch_query_search_messages_st (queries[i], &messages) != NOTMUCH_STATUS_SUCCESS)
          continue;
#else
      if (!(messages = notmuch_query_search_messages (queries[i])))
          continue;
#endif

      for (; notmuch_messages_valid (messages); notmuch_messages_move_to_next (messages)) {
          notmuch_message_t *msg = notmuch_messages_get (messages);

          for (guint j = 0; headers[j] != NULL; j++)
            {
              const gchar* froms = notmuch_message_get_header (msg, headers[j]);
              if (!froms || *froms == '\0')
                continue;

              GMatchInfo *matches;
              g_regex_match (match_re, froms, 0, &matches);
              while (g_match_info_matches (matches))
                {
                  gchar *from = g_match_info_fetch (matches, 1);
                  gboolean has_space = strchr (from, ' ') != NULL;
                  gchar *name = has_space ? g_match_info_fetch_named (matches, "name") : g_strdup (from);
                  gchar *addr = has_space ? g_match_info_fetch_named (matches, "mail") : g_strdup (from);
                  g_strstrip(name); /* Name should be 'Bob', not 'Bob ' */
                  gchar *patt = g_strdup_printf ("\\b%s", query_name);

                  if (g_regex_match_simple (patt, from, G_REGEX_CASELESS, 0))
                    {
                      gchar* addr_fold = g_utf8_casefold (addr, -1);
                      gchar* addr_key = g_utf8_collate_key (addr_fold, -1);
                      g_free (addr_fold);

                      ContactInfo *info = (ContactInfo*) g_hash_table_lookup (frequencies, addr_key);
                      if (info == NULL)
                        {
                          info = contact_info_new (addr, name);
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

  if (output_format->print_header)
      (*output_format->print_header) (addrs);

  g_list_foreach (addrs, output_format->print_item, NULL);


  g_list_free (addrs);
  g_hash_table_destroy (frequencies);
}


int
main (int argc, char **argv)
{
  setlocale(LC_CTYPE, "");
  atexit (global_cleanup);

  GOptionContext *option_context = g_option_context_new ("search-string");
#if GLIB_CHECK_VERSION(2, 44, 0)
  g_option_context_set_strict_posix (option_context, TRUE);
#endif
  g_option_context_add_main_entries (option_context,
                                     option_entries,
                                     "notmuch-addrlookup-c");

  GError *error = NULL;
  if (!g_option_context_parse (option_context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s.\n", g_get_prgname (), error->message);
      g_option_context_free (option_context);
      g_error_free (error);
      return EXIT_FAILURE;
    }
  g_option_context_free (option_context);

  if (mutt_output)
    {
      g_warning ("Option -m/--mutt is deprecated, use --format=mutt instead.");
      output_format_name = "mutt";
    }

  const struct PrintFormat *output_format = NULL;
  for (unsigned i = 0; i < G_N_ELEMENTS(s_formats); i++) {
      if (strcmp(s_formats[i].name, output_format_name) == 0) {
          output_format = &s_formats[i];
          break;
      }
  }

  if (!output_format)
    {
      g_printerr ("%s: %s is an invalid format name.\n", g_get_prgname (),output_format_name);
      return EXIT_FAILURE;
    }

  if (!search_terms || g_strv_length (search_terms) == 0)
    {
      g_printerr ("%s: No search terms provided.\n", g_get_prgname ());
      return EXIT_FAILURE;
    }

  if (!load_notmuch_settings ())
    {
      g_printerr ("Could not load Notmuch settings (~/.notmuch-config missing?)\n");
      return EXIT_FAILURE;
    }

  notmuch_database_t *db;
  g_autofree char *error_msg = NULL;
#if LIBNOTMUCH_CHECK_VERSION(5, 4, 0)
  notmuch_status_t status = notmuch_database_open_with_config (notmuch_database_path,
                                                               NOTMUCH_DATABASE_MODE_READ_ONLY,
                                                               NULL,
                                                               NULL,
                                                               &db,
                                                               &error_msg);
#else
  notmuch_status_t status = notmuch_database_open (notmuch_database_path,
                                                   NOTMUCH_DATABASE_MODE_READ_ONLY,
                                                   &db);
#endif

  if (status != NOTMUCH_STATUS_SUCCESS)
    {
      g_printerr ("Could not open Notmuch database: %s (%s)\n",
                  notmuch_status_to_string (status),
                  error_msg ? error_msg : "no error message");
      return EXIT_FAILURE;
    }

  notmuch_query_t *queries[NUM_QUERIES] = { NULL, NULL, NULL };
  gchar *name = g_strjoinv (" ", search_terms);

  create_queries (db, queries, name);
  run_queries (db, queries, name, output_format);

  g_free (name);
  notmuch_database_close (db);
  return EXIT_SUCCESS;
}
