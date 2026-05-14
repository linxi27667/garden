#ifndef GARDEN_MCP_TOOL_H
#define GARDEN_MCP_TOOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool Garden_Mcp_Init(void);
void Garden_Mcp_Start(void);
void Register_Garden_Mcp_Tools(void);

#ifdef __cplusplus
}
#endif

#endif
