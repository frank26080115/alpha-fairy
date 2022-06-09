#ifndef _PTPIPDEFS_H_
#define _PTPIPDEFS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PTPIP_NAME_LENGTH_MAX 256

typedef struct
{
    uint32_t length;
    uint32_t pkt_type;
}
ptpip_pkthdr_t;

typedef struct
{
    ptpip_pkthdr_t header;
    uint8_t        guid[16];
    uint16_t       name[PTPIP_NAME_LENGTH_MAX];
}
ptpip_pkt_cmdreq_t;

typedef struct
{
    ptpip_pkthdr_t header;
    uint32_t       conn_id;
    uint8_t        guid[16];
    uint16_t       name[PTPIP_NAME_LENGTH_MAX];
}
ptpip_pkt_cmdack_t;

typedef struct
{
    ptpip_pkthdr_t header;
    uint32_t       conn_id;
}
ptpip_pkt_eventreq_t;

#ifdef __cplusplus
}
#endif

#endif
