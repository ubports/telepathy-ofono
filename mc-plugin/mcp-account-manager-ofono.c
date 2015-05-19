/*
 * Copyright (C) 2015 Canonical, Ltd.
 * Contact: Tiago Salem Herrmann <tiago.herrmann@canonical.com>
 *
 * Based on Empathy ubuntu-online-accounts, mcp-account-manager-ring:
 * Contact: John Brooks <john.brooks@jollamobile.com>
 * Copyright (C) 2012 Jolla Ltd.
 * Copyright (C) 2012 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdio.h>
#include "mcp-account-manager-ofono.h"

#define PLUGIN_NAME "ofono-account"
#define PLUGIN_PRIORITY (MCP_ACCOUNT_STORAGE_PLUGIN_PRIO_DEFAULT - 10)
#define PLUGIN_DESCRIPTION "Provide ril modem accounts for telepathy-ofono"
#define PLUGIN_PROVIDER "im.telepathy.Account.Storage.Ofono"

static void account_storage_iface_init(McpAccountStorageIface *iface);

G_DEFINE_TYPE_WITH_CODE (McpAccountManagerOfono, mcp_account_manager_ofono,
    G_TYPE_OBJECT, G_IMPLEMENT_INTERFACE (MCP_TYPE_ACCOUNT_STORAGE,
    account_storage_iface_init));

struct ofono_account {
    gchar *account_name;
    int index;
    GHashTable *params;
};
typedef struct ofono_account OfonoAccount;

struct _McpAccountManagerOfonoPrivate
{
    GList *accounts;
};

void free_ofono_struct (gpointer data)
{
    OfonoAccount *account = (OfonoAccount*)data;
    g_hash_table_unref(account->params);
    g_free(account->account_name);
}

static void mcp_account_manager_ofono_dispose(GObject *object)
{
    McpAccountManagerOfono *self = (McpAccountManagerOfono*) object;

    g_list_free_full(self->priv->accounts, free_ofono_struct);

    G_OBJECT_CLASS (mcp_account_manager_ofono_parent_class)->dispose(object);
}

static void mcp_account_manager_ofono_init(McpAccountManagerOfono *self)
{
    g_debug("MC ril ofono accounts plugin initialized");
    gchar          *output = NULL;
    gchar          *output2 = NULL;
    const gchar    *force_num_modems = g_getenv("FORCE_RIL_NUM_MODEMS");
    GError         *error = NULL;
    int            num_modems = 0;
    int index;

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, MCP_TYPE_ACCOUNT_MANAGER_OFONO,
            McpAccountManagerOfonoPrivate);

    if (force_num_modems) {
        num_modems = atoi(force_num_modems);
        g_debug("forced number of modems: %d", num_modems);
    } else {
        if (!g_file_test ("/usr/bin/getprop", G_FILE_TEST_IS_EXECUTABLE)) {
            return;
        }
        if (!g_spawn_command_line_sync("/usr/bin/getprop rild.libpath ''",
                                        &output, NULL, NULL,
                                        &error)) {
            g_debug("%s", error->message);
            g_error_free (error);
            return;
        }

        if (strlen(g_strstrip(output)) != 0) {
            g_spawn_command_line_sync("/usr/bin/getprop ril.num_slots 1",
                                       &output2,
                                       NULL,
                                       NULL,
                                       &error);
            num_modems = atoi(output2);
        }
    }

    GSettings *settings = g_settings_new("com.ubuntu.phone");
    for (index = 0; index < num_modems; index++) {
        OfonoAccount *account = (OfonoAccount*)malloc(sizeof(OfonoAccount));
        char account_name[30] = {0};
        char ril_modem[10] = {0};
        account->index = index;
        sprintf(account_name, "ofono/ofono/account%d", index);
        sprintf(ril_modem, "/ril_%d", index);
        account->params = g_hash_table_new(g_str_hash, g_str_equal);
        account->account_name = g_strdup(account_name);
        g_hash_table_insert(account->params, g_strdup("manager"), g_strdup("ofono"));
        g_hash_table_insert(account->params, g_strdup("protocol"), g_strdup("ofono"));
        g_hash_table_insert(account->params, g_strdup("Enabled"), g_strdup("true"));
        g_hash_table_insert(account->params, g_strdup("ConnectAutomatically"), g_strdup("true"));
        g_hash_table_insert(account->params, g_strdup("always_dispatch"), g_strdup("true"));
        g_hash_table_insert(account->params, g_strdup("param-modem-objpath"), g_strdup(ril_modem));
        if (settings) {
            GVariant *sim_names = g_settings_get_value(settings, "sim-names");
            if (sim_names) {
                GVariantIter iter;
                GVariant *value;
                gchar *key;

                g_variant_iter_init (&iter, sim_names);
                while (g_variant_iter_next (&iter, "{ss}", &key, &value)) {
                    if (!strcmp(key, ril_modem)) {
                        g_hash_table_insert(account->params, g_strdup("DisplayName"), g_strdup((char *)value));
                    }
                    g_free (key);
                    g_free (value);
                }
            }
            if (sim_names) {
                g_variant_unref(sim_names);
            }
        }

        self->priv->accounts = g_list_append(self->priv->accounts, account);
    }
    if (settings) {
        g_object_unref (settings);
    }
    if (output) {
        g_free(output);
    }
    if (output2) {
      g_free (output2);
    }
}

static void mcp_account_manager_ofono_class_init(McpAccountManagerOfonoClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = mcp_account_manager_ofono_dispose;

    g_type_class_add_private(gobject_class, sizeof(McpAccountManagerOfonoPrivate));
}

static GList *account_manager_ofono_list(const McpAccountStorage *storage, const McpAccountManager *am)
{
    McpAccountManagerOfono *self = (McpAccountManagerOfono*) storage;
    GList *accounts = NULL;
    OfonoAccount *account = NULL;
    GList *l;
    for (l = self->priv->accounts; l != NULL; l = l->next) {
        account = (OfonoAccount*)l->data;
        accounts = g_list_prepend(accounts, g_strdup(account->account_name));
    }
    return accounts;
}

static gboolean account_manager_ofono_get(const McpAccountStorage *storage, const McpAccountManager *am,
        const gchar *account_name, const gchar *key)
{
    McpAccountManagerOfono *self = (McpAccountManagerOfono*) storage;
    GList *l;
    OfonoAccount *account = NULL;
    int found = 0;
    for (l = self->priv->accounts; l != NULL; l = l->next) {
        account = (OfonoAccount*)l->data;
        if (!strcmp(account_name, account->account_name)) {
            found = 1; 
            break;
        }
    }
    if (!found) {
        return FALSE;
    }

    if (key == NULL) {
        GHashTableIter iter;
        gpointer itkey, value;
        g_hash_table_iter_init(&iter, account->params);
        while (g_hash_table_iter_next(&iter, &itkey, &value)) {
            g_debug("%s: %s, %s %s", G_STRFUNC, account_name, (char*)itkey, (char*)value);
            mcp_account_manager_set_value(am, account_name, itkey, value);
        }
    } else {
        gchar *value = g_hash_table_lookup(account->params, key);
        g_debug("%s: %s, %s %s", G_STRFUNC, account_name, (char*)key, (char*)value);
        mcp_account_manager_set_value(am, account_name, key, value);
    }
    return FALSE;
}

static gboolean account_manager_ofono_set(const McpAccountStorage *storage, const McpAccountManager *am,
        const gchar *account_name, const gchar *key, const gchar *val)
{
    return FALSE;
}

static gchar *account_manager_ofono_create(const McpAccountStorage *storage, const McpAccountManager *am,
        const gchar *cm_name, const gchar *protocol_name, GHashTable *params, GError **error)
{
    g_set_error(error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT, "Ofono ril account manager cannot create accounts");
    return NULL;
}

static gboolean account_manager_ofono_delete(const McpAccountStorage *storage, const McpAccountManager *am,
        const gchar *account_name, const gchar *key)
{
    g_debug("%s: %s, %s", G_STRFUNC, account_name, key);
    return FALSE;
}

static gboolean account_manager_ofono_commit(const McpAccountStorage *storage, const McpAccountManager *am)
{
    g_debug("%s", G_STRFUNC);
    return FALSE;
}

static void account_manager_ofono_get_identifier(const McpAccountStorage *storage, const gchar *account_name,
        GValue *identifier)
{
    McpAccountManagerOfono *self = (McpAccountManagerOfono*) storage;

    GList *l;
    OfonoAccount *account = NULL;
    int found = 0;
    for (l = self->priv->accounts; l != NULL; l = l->next) {
        account = (OfonoAccount*)l->data;
        if (!strcmp(account_name, account->account_name)) {
            found = 1; 
            break;
        }
    }
    if (!found) {
        return;
    }

    g_debug("%s: %s", G_STRFUNC, account_name);
    g_value_init(identifier, G_TYPE_UINT);
    g_value_set_uint(identifier, account->index);
}

static guint account_manager_ofono_get_restrictions(const McpAccountStorage *storage, const gchar *account_name)
{
    McpAccountManagerOfono *self = (McpAccountManagerOfono*) storage;

    GList *l;
    OfonoAccount *account = NULL;
    int found = 0;
    for (l = self->priv->accounts; l != NULL; l = l->next) {
        account = (OfonoAccount*)l->data;
        if (!strcmp(account_name, account->account_name)) {
            found = 1; 
            break;
        }
    }

    if (!found) {
        return G_MAXUINT;
    }

    return TP_STORAGE_RESTRICTION_FLAG_CANNOT_SET_PARAMETERS |
        TP_STORAGE_RESTRICTION_FLAG_CANNOT_SET_ENABLED |
        TP_STORAGE_RESTRICTION_FLAG_CANNOT_SET_PRESENCE |
        TP_STORAGE_RESTRICTION_FLAG_CANNOT_SET_SERVICE;
}

static void account_storage_iface_init(McpAccountStorageIface *iface)
{
    iface->name = PLUGIN_NAME;
    iface->desc = PLUGIN_DESCRIPTION;
    iface->priority = PLUGIN_PRIORITY;
    iface->provider = PLUGIN_PROVIDER;

#define IMPLEMENT(x) iface->x = account_manager_ofono_##x
    IMPLEMENT (get);
    IMPLEMENT (list);
    IMPLEMENT (set);
    IMPLEMENT (create);
    IMPLEMENT (delete);
    IMPLEMENT (commit);
    IMPLEMENT (get_identifier);
    IMPLEMENT (get_restrictions);
#undef IMPLEMENT
}

McpAccountManagerOfono *mcp_account_manager_ofono_new(void)
{
    return g_object_new(MCP_TYPE_ACCOUNT_MANAGER_OFONO, NULL);
}

