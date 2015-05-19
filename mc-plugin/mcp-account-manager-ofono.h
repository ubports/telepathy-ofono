#ifndef __MCP_ACCOUNT_MANAGER_OFONO_H__
#define __MCP_ACCOUNT_MANAGER_OFONO_H__

#include <mission-control-plugins/mission-control-plugins.h>

G_BEGIN_DECLS

#define MCP_TYPE_ACCOUNT_MANAGER_OFONO \
    (mcp_account_manager_ofono_get_type ())
#define MCP_ACCOUNT_MANAGER_OFONO(o) \
    (G_TYPE_CHECK_INSTANCE_CAST ((o), MCP_TYPE_ACCOUNT_MANAGER_OFONO,   \
     McpAccountManagerOfono))

#define MCP_ACCOUNT_MANAGER_OFONO_CLASS(k)     \
    (G_TYPE_CHECK_CLASS_CAST((k), MCP_TYPE_ACCOUNT_MANAGER_OFONO, \
     McpAccountManagerOfonoClass))

#define MCP_IS_ACCOUNT_MANAGER_OFONO(o) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), MCP_TYPE_ACCOUNT_MANAGER_OFONO))

#define MCP_IS_ACCOUNT_MANAGER_OFONO_CLASS(k)  \
    (G_TYPE_CHECK_CLASS_TYPE ((k), MCP_TYPE_ACCOUNT_MANAGER_OFONO))

#define MCP_ACCOUNT_MANAGER_OFONO_GET_CLASS(o) \
    (G_TYPE_INSTANCE_GET_CLASS ((o), MCP_TYPE_ACCOUNT_MANAGER_OFONO, \
     McpAccountManagerOfonoClass))

typedef struct _McpAccountManagerOfonoPrivate McpAccountManagerOfonoPrivate;

typedef struct {
    GObject parent;

    McpAccountManagerOfonoPrivate *priv;
} _McpAccountManagerOfono;

typedef struct {
      GObjectClass parent_class;
} _McpAccountManagerOfonoClass;

typedef _McpAccountManagerOfono McpAccountManagerOfono;
typedef _McpAccountManagerOfonoClass McpAccountManagerOfonoClass;

GType mcp_account_manager_ofono_get_type (void) G_GNUC_CONST;

McpAccountManagerOfono *mcp_account_manager_ofono_new (void);

G_END_DECLS

#endif
