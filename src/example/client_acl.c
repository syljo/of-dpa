/*********************************************************************
*
* (C) Copyright Broadcom Corporation 2003-2014
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
**********************************************************************
*
* @filename     client_acl.c
*
* @purpose      Example code for ACL Flow Table. Uses RPC calls.
*
* @component    Unit Test
*
* @comments
*
* @create
*
* @end
*
**********************************************************************/
#include "ofdpa_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <net/ethernet.h>

#define VERSION              1.0

#define DEFAULT_COUNT         1
#define DEFAULT_PRIORITY      0
#define DEFAULT_IFNUM         1
#define DEFAULT_IFNUMMASK     (OFDPA_INPORT_EXACT_MASK)
#define DEFAULT_SRCMAC        {{ 0x00, 0x09, 0x07, 0x05, 0x03, 0x01 }}
#define DEFAULT_SRCMACMASK    {{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }}
#define DEFAULT_DESTMAC       {{ 0x00, 0x01, 0x03, 0x05, 0x07, 0x09 }}
#define DEFAULT_DESTMACMASK   {{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }}
#define DEFAULT_VLANID        1
#define DEFAULT_VLANIDMASK    OFDPA_VID_EXACT_MASK
#define DEFAULT_TUNNELID      0
#define DEFAULT_ETHERTYPE     0x0800u
#define DEFAULT_VLANPRIO      0
#define DEFAULT_VLANPRIOMASK  0
#define DEFAULT_SOURCEIP4     0x01010101u
#define DEFAULT_SOURCEIP4MASK (OFDPA_IPV4_ADDR_EXACT_MASK)
#define DEFAULT_DESTIP4       0x02020202u
#define DEFAULT_DESTIP4MASK   (OFDPA_IPV4_ADDR_EXACT_MASK)
#define DEFAULT_PROTOCOL      17
#define DEFAULT_PROTOCOLMASK  0xFF
#define DEFAULT_DSCP          0
#define DEFAULT_DSCPMASK      0
#define DEFAULT_SRCPORT       100
#define DEFAULT_SRCPORTMASK   0xFFFFFFFFu
#define DEFAULT_DSTPORT       200
#define DEFAULT_DSTPORTMASK   0xFFFFFFFFu
#define DEFAULT_WRITEACTIONS  0
#define DEFAULT_SETGROUP      0
#define DEFAULT_SETQUEUE      0
#define DEFAULT_SETVLANPRI    0
#define DEFAULT_SETDSCP       0

#define INVALID_ETHERTYPE    0xFFFFu
#define INVALID_PROTOCOL     0xFFFFu

#define VLANID_MAX        4096
#define VLANPRIO_MAX      7
#define DSCP_MAX          63
#define PORTNUM_MAX       65535

#define ARG_DELETE        "delete"
#define ARG_LIST          "list"

#define KEY_COUNT          1
#define KEY_INTF           2
#define KEY_VLANID         3
#define KEY_VLANPRI        4
#define KEY_ETHERTYPE      5
#define KEY_SRCMAC         6
#define KEY_DESTMAC        7
#define KEY_DESTIP4        8
#define KEY_PROTOCOL       9
#define KEY_SOURCEIP4     10
#define KEY_SOURCEIP6     11
#define KEY_DESTIP6       12
#define KEY_DSCP          13
#define KEY_SRCPORT       14
#define KEY_DSTPORT       15
#define KEY_PRIORITY      16
#define KEY_SETGROUP      17
#define KEY_SETQUEUE      18
#define KEY_SETVLANPRI    19
#define KEY_SETDSCP       20
#define KEY_DISCARD       21
#define KEY_COPY          22
#define KEY_ICMPTYPE      23
#define KEY_ICMPCODE      24
#define KEY_SRCIP4PREFIX  25
#define KEY_DESTIP4PREFIX 26
#define KEY_SRCIP6PREFIX  27
#define KEY_DESTIP6PREFIX 28
#define KEY_SRCMACMASK    30
#define KEY_DESTMACMASK   31
#define KEY_TUNNELID      32
#define KEY_OUTPUT_TUNNEL_PORT 33

typedef struct
{
  int              count;
  int              delete;
  int              list;
  uint32_t         ipv4Found;
  uint32_t         ipv6Found;
  ofdpaFlowEntry_t flow;
} arguments_t;

ofdpaMacAddr_t noMacMask  = {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};

const struct in6_addr in6addr_null = { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } };

/* The options we understand. */
static struct argp_option options[] =
{
  { "count",       KEY_COUNT,        "COUNT",        0, "Number of ACLs to add, delete or list.",        0 },
  { "priority",    KEY_PRIORITY,     "PRIORITY",     0, "The priority of the rule.",                       },
  { "intf",        KEY_INTF,         "IFNUM",        0, "The ingress interface number (or \"all\").",    1 },
  { 0,             0,                0,              0, "L2 Qualifiers:",                                  },
  { "vlan",        KEY_VLANID,       "VLANID",       0, "The VLAN to which the ACL should be applied.",    },
  { "tunnelid",    KEY_TUNNELID,     "TUNNELID",     0, "The tenant identifier.",                          },
  { "vlanpri",     KEY_VLANPRI,      "VLANPRI",      0, "The VLAN priority tag.",                          },
  { "ether",       KEY_ETHERTYPE,    "ETHERTYPE",    0, "The ethertype.",                                  },
  { "srcmac",      KEY_SRCMAC,       "SRCMAC",       0, "The source MAC address.",                         },
  { "dstmac",      KEY_DESTMAC,      "DESTMAC",      0, "The destination MAC address.",                    },
  { "srcmacmask",  KEY_SRCMACMASK,   "MASK",         0, "The source MAC address Mask (Default: ff:ff:ff:ff:ff:ff).",     },
  { "dstmacmask",  KEY_DESTMACMASK,  "MASK",         0, "The destination MAC address Mask (Default: ff:ff:ff:ff:ff:ff).",},
  { 0,             0,                0,              0, "L3 Qualifiers:",                                  },
  { "dstip4",      KEY_DESTIP4,      "DESTIP4",      0, "The destination IPv4 address.",                   },
  { "dstip4pfx",   KEY_DESTIP4PREFIX,"PREFIXLEN",    0, "The destination IPv4 prefix length (Default = 32).",},
  { "proto",       KEY_PROTOCOL,     "PROTOCOL",     0, "The IP protocol.",                                },
  { "srcip4",      KEY_SOURCEIP4,    "SOURCEIP4",    0, "The source IPv4 address.",                        },
  { "srcip4pfx",   KEY_SRCIP4PREFIX, "PREFIXLEN",    0, "The source IPv4 prefix length (Default = 32).",   },
  { "srcip6",      KEY_SOURCEIP6,    "SOURCEIP6",    0, "The source IPv6 address.",                        },
  { "srcip6pfx",   KEY_SRCIP6PREFIX, "PREFIXLEN",    0, "The source IPv6 prefix length (Default = 0).",    },
  { "dstip6",      KEY_DESTIP6,      "DESTIP6",      0, "The destination IPv6 address.",                   },
  { "dstip6pfx",   KEY_DESTIP6PREFIX,"PREFIXLEN",    0,  "The destination IPv6 prefix length (Default = 0).", },
  { "dscp",        KEY_DSCP,         "DSCP",         0, "The DSCP.",                                       },
  { 0,             0,                0,              0, "L4 Qualifiers:",                                  },
  { "srcport",     KEY_SRCPORT,      "SRCPORT",      0, "The source L4 port.",                             },
  { "dstport",     KEY_DSTPORT,      "DSTPORT",      0, "The destination L4 port.",                        },
  { "icmptype",    KEY_ICMPTYPE,     "ICMPTYPE",     0, "The ICMP packet type.",                           },
  { "icmpcode",    KEY_ICMPCODE,     "ICMPCODE",     0, "The ICMP packet code.",                           },
  { 0,             0,                0,              0, "Write Actions:",                                  },
  { "setgroup",    KEY_SETGROUP,     "GROUP",        0, "Set the output group for packets in this flow.",  },
  { "setqueue",    KEY_SETQUEUE,     "QUEUE",        0, "Set the output queue for packets in this flow.",  },
  { "setvlanp",    KEY_SETVLANPRI,   "PRIORITY",     0, "Set the VLAN priority for packets in this flow.", },
  { "setdscp",     KEY_SETDSCP,      "DSCP",         0, "Set the DSCP for packets in this flow.",          },
  { "outtunnelport", KEY_OUTPUT_TUNNEL_PORT, "OUTTUNNELPORT",      0, "Output tunnel port for Tenant type flows.",       },
  { 0,             0,                0,              0, "Other Actions:",                                  },
  { "discard",     KEY_DISCARD,      0,              0, "Discard matching flows.",                         },
  { "copy",        KEY_COPY,         0,              0, "Copy matching flows to the CPU.",                 },
  { 0 }
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
  /* Get the INPUT argument from `argp_parse', which we
     know is a pointer to our arguments structure. */
  arguments_t *arguments = state->input;
  ofdpaPolicyAclFlowEntry_t *flowEntry = &arguments->flow.flowData.policyAclFlowEntry;
  ofdpaPolicyAclFlowMatch_t *aclMatch = &flowEntry->match_criteria;
  struct in_addr addr;
  struct in6_addr addr6;
  uint32_t prefixLen;
  uint32_t i;
  union
  {
    unsigned char  bytes[6];
    unsigned short shorts[3];
  } mac;

  switch (key)
  {
    case KEY_SOURCEIP6:                 /* source IPv6 address */
      if (0 == inet_pton(AF_INET6, arg, &addr6))
      {
        argp_error(state, "Invalid Source IPv6 address \"%s\"", arg);
        return errno;
      }
      else
      {
        memcpy(aclMatch->sourceIp6.s6_addr, addr6.s6_addr, sizeof(aclMatch->sourceIp6.s6_addr));
        arguments->ipv6Found = 1;
      }
      break;
    case KEY_SRCIP6PREFIX:
      errno = 0;
      prefixLen = strtoul(arg, NULL, 0);
      if (errno != 0 || prefixLen > 128)
      {
       argp_error(state, "Invalid Source IPv6 prefix length \"%s\"", arg);
       return errno;
      }
      for (i = 0; i < prefixLen / 32; i++)
      {
        aclMatch->sourceIp6Mask.s6_addr32[i] = ~0;
      }
      if (0 != (prefixLen % 32))
      {
        aclMatch->sourceIp6Mask.s6_addr32[i] = (~0 << (prefixLen % 32));
      }
      break;
    case KEY_DESTIP6:                   /* destination IPv6 address */
      if (0 == inet_pton(AF_INET6, arg, &addr6))
      {
        argp_error(state, "Invalid Destination IPv6 address \"%s\"", arg);
        return errno;
      }
      else
      {
        memcpy(aclMatch->destIp6.s6_addr, addr6.s6_addr, sizeof(aclMatch->destIp6.s6_addr));
        arguments->ipv6Found = 1;
      }
      break;

    case KEY_DESTIP6PREFIX:
      errno = 0;
      prefixLen = strtoul(arg, NULL, 0);
      if (errno != 0 || prefixLen > 128)
      {
       argp_error(state, "Invalid Destination IPv6 prefix length \"%s\"", arg);
       return errno;
      }
      for (i = 0; i < prefixLen / 32; i++)
      {
        aclMatch->destIp6Mask.s6_addr32[i] = ~0;
      }
      if (0 != (prefixLen % 32))
      {
        aclMatch->destIp6Mask.s6_addr32[i] = (~0 << (prefixLen % 32));
      }
      break;
    case KEY_COUNT:                     /* count */
      errno = 0;
      arguments->count = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid count \"%s\"", arg);
        return errno;
      }
      break;

    case KEY_PRIORITY:                  /* priority */
      errno = 0;
      arguments->flow.priority = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid priority \"%s\"", arg);
        return errno;
      }
      break;

    case KEY_DESTIP4:                   /* destination IPv4 address */
      if (1 == inet_pton(AF_INET, arg, &addr))
      {
        aclMatch->destIp4 = ntohl(addr.s_addr);
      }
      else
      {
        errno = 0;
        aclMatch->destIp4 = strtoul(arg, NULL, 0);
        if (errno != 0)
        {
          argp_error(state, "Invalid Destination IPv4 address \"%s\"", arg);
          return errno;
        }
      }

      if (0 == aclMatch->destIp4)
      {
        aclMatch->destIp4Mask = 0;
      }
      else
      {
        arguments->ipv4Found = 1;
      }
      break;
    case KEY_DESTIP4PREFIX:
      errno = 0;
      prefixLen = strtoul(arg, NULL, 0);
      if (errno != 0 || prefixLen > 32)
      {
       argp_error(state, "Invalid Destination IPv4 prefix length \"%s\"", arg);
       return errno;
      }
      aclMatch->destIp4Mask = (~0 << (32 - prefixLen));
      break;

    case KEY_ETHERTYPE:                 /* Ethertype */
      errno = 0;
      aclMatch->etherType = strtoul(arg, NULL, 0);
      if ((errno != 0) ||
          (aclMatch->etherType >= INVALID_ETHERTYPE))
      {
        argp_error(state, "Invalid Ethertype \"%s\"", arg);
        return errno;
      }
      break;

    case KEY_SRCMAC:                    /* source MAC address */
      if (6 != sscanf(arg, " %2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx ",
                      &mac.bytes[0], &mac.bytes[1], &mac.bytes[2], &mac.bytes[3], &mac.bytes[4], &mac.bytes[5]))
      {
        if (6 != sscanf(arg, " %2hhx-%2hhx-%2hhx-%2hhx-%2hhx-%2hhx ",
                        &mac.bytes[0], &mac.bytes[1], &mac.bytes[2], &mac.bytes[3], &mac.bytes[4], &mac.bytes[5]))
        {
          if (3 != sscanf(arg, " %4hx.%4hx.%4hx ", &mac.shorts[0], &mac.shorts[1], &mac.shorts[2]))
          {
            argp_error(state, "Invalid source MAC address \"%s\"", arg);
            return errno;
          }
          else
          {
            /* successfully converted address string to short int values, need to handle endian issues here */
            mac.shorts[0] = htons(mac.shorts[0]);
            mac.shorts[1] = htons(mac.shorts[1]);
            mac.shorts[2] = htons(mac.shorts[2]);
          }
        }
      }
      memcpy(aclMatch->srcMac.addr, mac.bytes, sizeof(aclMatch->srcMac.addr));

      if (0 == memcmp(aclMatch->srcMac.addr, noMacMask.addr, sizeof(aclMatch->srcMac.addr)))
      {
        memcpy(aclMatch->srcMacMask.addr, noMacMask.addr, sizeof(aclMatch->srcMacMask.addr));
      }
      break;

    case KEY_SRCMACMASK:
      if (6 != sscanf(arg, " %2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx ",
                     &mac.bytes[0], &mac.bytes[1], &mac.bytes[2], &mac.bytes[3], &mac.bytes[4], &mac.bytes[5]))
      {
        if (6 != sscanf(arg, " %2hhx-%2hhx-%2hhx-%2hhx-%2hhx-%2hhx ",
                        &mac.bytes[0], &mac.bytes[1], &mac.bytes[2], &mac.bytes[3], &mac.bytes[4], &mac.bytes[5]))
        {
          if (3 != sscanf(arg, " %4hx.%4hx.%4hx ", &mac.shorts[0], &mac.shorts[1], &mac.shorts[2]))
          {
            argp_error(state, "Invalid source MAC Mask \"%s\"", arg);
            return errno;
          }
          else
          {
            /* successfully converted address string to short int values, need to handle endian issues here */
            mac.shorts[0] = htons(mac.shorts[0]);
            mac.shorts[1] = htons(mac.shorts[1]);
            mac.shorts[2] = htons(mac.shorts[2]);
          }
        }
      }
      memcpy(aclMatch->srcMacMask.addr, mac.bytes, sizeof(aclMatch->srcMacMask.addr));
      break;

    case KEY_INTF:                      /* interface number */
      if (0 == strcasecmp("all", arg))
      {
        aclMatch->inPort = 0;
        aclMatch->inPortMask = 0;
      }
      else
      {
        errno = 0;
        aclMatch->inPort = strtoul(arg, NULL, 0);
        if (errno != 0)
        {
          argp_error(state, "Invalid interface number \"%s\"", arg);
          return errno;
        }
        aclMatch->inPortMask = DEFAULT_IFNUMMASK;
      }
      break;

    case KEY_VLANPRI:                   /* VLAN Priority */
      errno = 0;
      aclMatch->vlanPcp = strtoul(arg, NULL, 0);
      if ((errno != 0) ||
          (aclMatch->vlanPcp > VLANPRIO_MAX))
      {
        argp_error(state, "Invalid VLAN ID \"%s\"", arg);
        return errno;
      }
      aclMatch->vlanPcpMask = 0x7;
      break;

    case KEY_DESTMAC:                   /* destination MAC address */
      if (6 != sscanf(arg, " %2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx ",
                      &mac.bytes[0], &mac.bytes[1], &mac.bytes[2], &mac.bytes[3], &mac.bytes[4], &mac.bytes[5]))
      {
        if (6 != sscanf(arg, " %2hhx-%2hhx-%2hhx-%2hhx-%2hhx-%2hhx ",
                        &mac.bytes[0], &mac.bytes[1], &mac.bytes[2], &mac.bytes[3], &mac.bytes[4], &mac.bytes[5]))
        {
          if (3 != sscanf(arg, " %4hx.%4hx.%4hx ", &mac.shorts[0], &mac.shorts[1], &mac.shorts[2]))
          {
            argp_error(state, "Invalid destination MAC address \"%s\"", arg);
            return errno;
          }
          else
          {
            /* successfully converted address string to short int values, need to handle endian issues here */
            mac.shorts[0] = htons(mac.shorts[0]);
            mac.shorts[1] = htons(mac.shorts[1]);
            mac.shorts[2] = htons(mac.shorts[2]);
          }
        }
      }
      memcpy(aclMatch->destMac.addr, mac.bytes, sizeof(aclMatch->destMac.addr));
      if (0 == memcmp(aclMatch->destMac.addr, noMacMask.addr, sizeof(aclMatch->destMac.addr)))
      {
        memcpy(aclMatch->destMacMask.addr, noMacMask.addr, sizeof(aclMatch->destMacMask.addr));
      }
      break;
    case KEY_DESTMACMASK:
      if (6 != sscanf(arg, " %2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx ",
                      &mac.bytes[0], &mac.bytes[1], &mac.bytes[2], &mac.bytes[3], &mac.bytes[4], &mac.bytes[5]))
      {
        if (6 != sscanf(arg, " %2hhx-%2hhx-%2hhx-%2hhx-%2hhx-%2hhx ",
                        &mac.bytes[0], &mac.bytes[1], &mac.bytes[2], &mac.bytes[3], &mac.bytes[4], &mac.bytes[5]))
        {
          if (3 != sscanf(arg, " %4hx.%4hx.%4hx ", &mac.shorts[0], &mac.shorts[1], &mac.shorts[2]))
          {
            argp_error(state, "Invalid destination MAC mask \"%s\"", arg);
            return errno;
          }
          else
          {
            /* successfully converted address string to short int values, need to handle endian issues here */
            mac.shorts[0] = htons(mac.shorts[0]);
            mac.shorts[1] = htons(mac.shorts[1]);
            mac.shorts[2] = htons(mac.shorts[2]);
          }
        }
      }
      memcpy(aclMatch->destMacMask.addr, mac.bytes, sizeof(aclMatch->destMacMask.addr));
      break;

    case KEY_PROTOCOL:                  /* IP protocol */
      errno = 0;
      aclMatch->ipProto = strtoul(arg, NULL, 0);
      if ((errno != 0) ||
          (aclMatch->ipProto >= INVALID_PROTOCOL))
      {
        argp_error(state, "Invalid IP Protocol \"%s\"", arg);
        return errno;
      }
      if (0 == aclMatch->ipProto)
      {
        aclMatch->ipProtoMask = 0;
      }
      break;

    case KEY_DSCP:                      /* DSCP */
      errno = 0;
      aclMatch->dscp = strtoul(arg, NULL, 0);
      if ((errno != 0) ||
          (aclMatch->dscp > DSCP_MAX))
      {
        argp_error(state, "Invalid DSCP \"%s\"", arg);
        return errno;
      }
      aclMatch->dscpMask = 0xff;
      break;

    case KEY_SRCPORT:                   /* source L4 port */
      errno = 0;
      aclMatch->srcL4Port = strtoul(arg, NULL, 0);
      if ((errno != 0) ||
          (aclMatch->srcL4Port > PORTNUM_MAX))
      {
        argp_error(state, "Invalid source L4 port \"%s\"", arg);
        return errno;
      }
      if (0 == aclMatch->srcL4Port)
      {
        aclMatch->srcL4PortMask = 0;
      }
      break;

    case KEY_SOURCEIP4:                 /* source IPv4 address */
      if (1 == inet_pton(AF_INET, arg, &addr))
      {
        aclMatch->sourceIp4 = ntohl(addr.s_addr);
      }
      else
      {
        errno = 0;
        aclMatch->sourceIp4 = strtoul(arg, NULL, 0);
        if (errno != 0)
        {
          argp_error(state, "Invalid Source IPv4 address \"%s\"", arg);
          return errno;
        }
      }

      if (0 == aclMatch->sourceIp4)
      {
        aclMatch->sourceIp4Mask = 0;
      }
      else
      {
        arguments->ipv4Found = 1;
      }
      break;

    case KEY_SRCIP4PREFIX:
      errno = 0;
      prefixLen = strtoul(arg, NULL, 0);
      if (errno != 0 || prefixLen > 32)
      {
       argp_error(state, "Invalid source IPv4 prefix length \"%s\"", arg);
       return errno;
      }

      aclMatch->sourceIp4Mask = (~0 << (32 - prefixLen));
      break;

    case KEY_DSTPORT:                   /* destination L4 port */
      errno = 0;
      aclMatch->destL4Port = strtoul(arg, NULL, 0);
      if ((errno != 0) ||
          (aclMatch->destL4Port > PORTNUM_MAX))
      {
        argp_error(state, "Invalid destination L4 port \"%s\"", arg);
        return errno;
      }
      if (0 == aclMatch->destL4Port)
      {
        aclMatch->destL4PortMask = 0;
      }
      break;

    case KEY_ICMPTYPE:
      errno = 0;
      aclMatch->icmpType = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid ICMP type \"%s\"", arg);
        return errno;
      }
      aclMatch->icmpTypeMask = 0xff;
      break;

    case KEY_ICMPCODE:
      errno = 0;
      aclMatch->icmpCode = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid ICMP code \"%s\"", arg);
        return errno;
      }
      aclMatch->icmpCodeMask = 0xff;
      break;

    case KEY_VLANID:                    /* VLAN */
      errno = 0;
      aclMatch->vlanId = strtoul(arg, NULL, 0);
      if ((errno != 0) ||
          (aclMatch->vlanId > VLANID_MAX))
      {
        argp_error(state, "Invalid VLAN ID \"%s\"", arg);
        return errno;
      }
      if (0 == aclMatch->vlanId)
      {
        aclMatch->vlanIdMask = 0;
      }
      break;

    case KEY_TUNNELID:
      errno = 0;
      aclMatch->tunnelId = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid tunnel ID \"%s\"", arg);
        return errno;
      }
      break;

    case KEY_SETGROUP:
      errno = 0;
      flowEntry->groupID = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid group ID \"%s\"", arg);
        return errno;
      }
      break;

    case KEY_SETQUEUE:
      errno = 0;
      flowEntry->queueID = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid queue ID \"%s\"", arg);
        return errno;
      }
      flowEntry->queueIDAction = 1;
      break;

    case KEY_SETVLANPRI:
      errno = 0;
      flowEntry->vlanPcp = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid VLAN priority value \"%s\"", arg);
        return errno;
      }
      flowEntry->vlanPcpAction = 1;
      break;

    case KEY_SETDSCP:
      errno = 0;
      flowEntry->dscp = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid DSCP value \"%s\"", arg);
        return errno;
      }
      flowEntry->dscpAction = 1;
      break;

    case KEY_OUTPUT_TUNNEL_PORT:
      errno = 0;
      flowEntry->outputTunnelPort = strtoul(arg, NULL, 0);
      if (errno != 0)
      {
        argp_error(state, "Invalid output Tunnel Port \"%s\"", arg);
        return errno;
      }
      break;

    case KEY_DISCARD:
      flowEntry->clearActions = 1;
      break;

    case KEY_COPY:
      flowEntry->outputPort = OFDPA_PORT_CONTROLLER;
      break;

    case ARGP_KEY_ARG:
      if (0 == strcasecmp(ARG_DELETE, arg))
      {
        arguments->delete = 1;
      }
      else if (0 == strcasecmp(ARG_LIST, arg))
      {
        arguments->list = 1;
      }
      else
      {
        argp_error(state, "Unknown option \"%s\"", arg);
      }
      break;

    case ARGP_KEY_NO_ARGS:
      break;
    case ARGP_KEY_END:
      if ((arguments->ipv6Found == 1) && (aclMatch->etherType == ETHERTYPE_IP))
      {
        argp_error(state, " Incorrect ethertype for IPv6 address");
        return EINVAL;
      }
      else if ((arguments->ipv4Found == 1) && (aclMatch->etherType == ETHERTYPE_IPV6))
      {
        argp_error(state, " Incorrect ethertype for IPv4 address");
        return EINVAL;
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static void displayAcl(ofdpaFlowEntry_t *flow)
{
  char buf[INET6_ADDRSTRLEN + 1];
  struct in_addr ipv4Addr;
  ofdpaPolicyAclFlowEntry_t *flowEntry = &flow->flowData.policyAclFlowEntry;
  ofdpaPolicyAclFlowMatch_t *aclMatch  = &flow->flowData.policyAclFlowEntry.match_criteria;

  printf("\tPriority                = %u\r\n", flow->priority);
  if (0 != aclMatch->inPortMask)
  {
    printf("\tInterface               = %u\r\n", aclMatch->inPort);
  }
  if (0 != aclMatch->vlanIdMask)
  {
    printf("\tVLAN ID                 = %u\r\n", aclMatch->vlanId);
  }
  if (0 != aclMatch->tunnelId)
  {
    printf("\tTunnel ID               = %u\r\n", aclMatch->tunnelId);
  }
  if (0 != aclMatch->vlanPcpMask)
  {
    printf("\tVLAN Priority           = %u\r\n", aclMatch->vlanPcp);
  }
  if (0 != memcmp(aclMatch->srcMacMask.addr, noMacMask.addr, sizeof(aclMatch->srcMacMask.addr)))
  {
    printf("\tSource MAC address      = %2.2x%2.2x.%2.2x%2.2x.%2.2x%2.2x\r\n", aclMatch->srcMac.addr[0],
           aclMatch->srcMac.addr[1], aclMatch->srcMac.addr[2], aclMatch->srcMac.addr[3],
           aclMatch->srcMac.addr[4], aclMatch->srcMac.addr[5]);

    printf("\tSource MAC Mask         = %2.2x%2.2x.%2.2x%2.2x.%2.2x%2.2x\r\n", aclMatch->srcMacMask.addr[0],
           aclMatch->srcMacMask.addr[1], aclMatch->srcMacMask.addr[2], aclMatch->srcMacMask.addr[3],
           aclMatch->srcMacMask.addr[4], aclMatch->srcMacMask.addr[5]);
  }

  if (0 != memcmp(aclMatch->destMacMask.addr, noMacMask.addr, sizeof(aclMatch->destMacMask.addr)))
  {
    printf("\tDestination MAC address = %2.2x%2.2x.%2.2x%2.2x.%2.2x%2.2x\r\n", aclMatch->destMac.addr[0],
           aclMatch->destMac.addr[1], aclMatch->destMac.addr[2], aclMatch->destMac.addr[3],
           aclMatch->destMac.addr[4], aclMatch->destMac.addr[5]);

    printf("\tDestination MAC Mask    = %2.2x%2.2x.%2.2x%2.2x.%2.2x%2.2x\r\n", aclMatch->destMacMask.addr[0],
           aclMatch->destMacMask.addr[1], aclMatch->destMacMask.addr[2], aclMatch->destMacMask.addr[3],
           aclMatch->destMacMask.addr[4], aclMatch->destMacMask.addr[5]);
  }

  printf("\tEthertype               = 0x%4.4x\r\n", aclMatch->etherType);

  ipv4Addr.s_addr = htonl(aclMatch->sourceIp4);
  if ((0 != aclMatch->sourceIp4Mask) &&
      (NULL != inet_ntop(AF_INET, &ipv4Addr, buf, sizeof(buf))))
  {
    printf("\tSource IPv4 address     = %s\r\n", buf);

    ipv4Addr.s_addr = htonl(aclMatch->sourceIp4Mask);
    if (NULL != inet_ntop(AF_INET, &ipv4Addr, buf, sizeof(buf)))
    {
      printf("\tSource IPv4 Mask        = %s\r\n", buf);
    }
  }

  ipv4Addr.s_addr = htonl(aclMatch->destIp4);
  if ((0 != aclMatch->destIp4Mask) &&
      (NULL != inet_ntop(AF_INET, &ipv4Addr, buf, sizeof(buf))))
  {
    printf("\tDestination IPv4 address = %s\r\n", buf);

    ipv4Addr.s_addr = htonl(aclMatch->destIp4Mask);
    if (NULL != inet_ntop(AF_INET, &ipv4Addr, buf, sizeof(buf)))
    {
      printf("\tDestination IPv4 Mask    = %s\r\n", buf);
    }
  }
  if ((0 != memcmp(&aclMatch->sourceIp6Mask, &in6addr_null, sizeof(aclMatch->sourceIp6Mask))) &&
      (NULL != inet_ntop(AF_INET6, &aclMatch->sourceIp6, buf, sizeof(buf))))
  {
    printf("\tSource IPv6 address      = %s\r\n", buf);

    if (NULL != inet_ntop(AF_INET6, &aclMatch->sourceIp6Mask, buf, sizeof(buf)))
    {
      printf("\tSource IPv6 Mask         = %s\r\n", buf);
    }
  }
  if ((0 != memcmp(&aclMatch->destIp6Mask, &in6addr_null, sizeof(aclMatch->destIp6Mask))) &&
      (NULL != inet_ntop(AF_INET6, &aclMatch->destIp6, buf, sizeof(buf))))
  {
    printf("\tDestination IPv6 address = %s\r\n", buf);

    if (NULL != inet_ntop(AF_INET6, &aclMatch->destIp6Mask, buf, sizeof(buf)))
    {
      printf("\tDestination IPv6 Mask    = %s\r\n", buf);
    }

  }
  if (0 != aclMatch->dscpMask)
  {
    printf("\tDSCP                     = %u\r\n", aclMatch->dscp);
  }
  if (0 != aclMatch->ipProtoMask)
  {
    printf("\tIP Protocol              = 0x%2.2x\r\n", aclMatch->ipProto);
  }
  if (0 != aclMatch->srcL4PortMask)
  {
    printf("\tSource L4 Port           = %u\r\n", aclMatch->srcL4Port);
  }
  if (0 != aclMatch->destL4PortMask)
  {
    printf("\tDestination L4 Port      = %u\r\n", aclMatch->destL4Port);
  }
  if (0 != aclMatch->icmpTypeMask)
  {
    printf("\tICMP Type                = %u\r\n", aclMatch->icmpType);
  }
  if (0 != aclMatch->icmpCodeMask)
  {
    printf("\tICMP Code                = %u\r\n", aclMatch->icmpCode);
  }
  printf("\tIdle Time                = %d\r\n", flow->idle_time);
  printf("  Actions:\r\n");
  if (OFDPA_PORT_CONTROLLER == flowEntry->outputPort)
  {
    printf("\tCopy to controller\r\n");
  }
  if (flowEntry->clearActions)
  {
    printf("\tDrop packet\r\n");
  }
  else
  {
    if (flowEntry->groupID)
    {
      printf("\tSet output group ID = 0x%8x\r\n", flowEntry->groupID);
    }
    if (flowEntry->queueIDAction)
    {
      printf("\tSet CoS queue = %d\r\n", flowEntry->queueID);
    }
    if (flowEntry->vlanPcpAction)
    {
      printf("\tSet VLAN priority = %d\r\n", flowEntry->vlanPcp);
    }
    if (flowEntry->dscpAction)
    {
      printf("\tSet DSCP = %d\r\n", flowEntry->dscp);
    }
    if (flowEntry->outputTunnelPort)
    {
      printf("\tOutput tunnel port = 0x%x\r\n", flowEntry->outputTunnelPort);
    }
  }
}

static void incrementMac(ofdpaMacAddr_t *mac)
{
  int i;

  mac->addr[5]++;
  for (i = 5; i > 0; i--)
  {
    if (0 == mac->addr[i])
    {
      mac->addr[i - 1]++;
    }
    else
    {
      break;
    }
  }
}

int main(int argc, char *argv[])
{
  int                   i;
  int                   rc;
  char                  client_name[] = "ofdpa Policy ACL client";
  char                  buf[INET6_ADDRSTRLEN + 1];
  char                  docBuffer[300];
  char                  versionBuf[100];
  ofdpaFlowEntry_t      flow;
  ofdpaFlowEntryStats_t flowStats;

  arguments_t arguments =
    {
      .count                                                          = DEFAULT_COUNT,
      .delete                                                         = 0,
      .list                                                           = 0,
      .flow.priority                                                  = DEFAULT_PRIORITY,
      .flow.flowData.policyAclFlowEntry.match_criteria.inPort         = DEFAULT_IFNUM,
      .flow.flowData.policyAclFlowEntry.match_criteria.inPortMask     = DEFAULT_IFNUMMASK,
      .flow.flowData.policyAclFlowEntry.match_criteria.etherType      = DEFAULT_ETHERTYPE,
      .flow.flowData.policyAclFlowEntry.match_criteria.srcMac         = DEFAULT_SRCMAC,
      .flow.flowData.policyAclFlowEntry.match_criteria.srcMacMask     = DEFAULT_SRCMACMASK,
      .flow.flowData.policyAclFlowEntry.match_criteria.destMac        = DEFAULT_DESTMAC,
      .flow.flowData.policyAclFlowEntry.match_criteria.destMacMask    = DEFAULT_DESTMACMASK,
      .flow.flowData.policyAclFlowEntry.match_criteria.vlanId         = DEFAULT_VLANID,
      .flow.flowData.policyAclFlowEntry.match_criteria.vlanIdMask     = DEFAULT_VLANIDMASK,
      .flow.flowData.policyAclFlowEntry.match_criteria.tunnelId       = DEFAULT_TUNNELID,
      .flow.flowData.policyAclFlowEntry.match_criteria.vlanPcp        = DEFAULT_VLANPRIO,
      .flow.flowData.policyAclFlowEntry.match_criteria.vlanPcpMask    = DEFAULT_VLANPRIOMASK,
      .flow.flowData.policyAclFlowEntry.match_criteria.sourceIp4      = DEFAULT_SOURCEIP4,
      .flow.flowData.policyAclFlowEntry.match_criteria.sourceIp4Mask  = DEFAULT_SOURCEIP4MASK,
      .flow.flowData.policyAclFlowEntry.match_criteria.destIp4        = DEFAULT_DESTIP4,
      .flow.flowData.policyAclFlowEntry.match_criteria.destIp4Mask    = DEFAULT_DESTIP4MASK,

      .flow.flowData.policyAclFlowEntry.match_criteria.ipProto        = DEFAULT_PROTOCOL,
      .flow.flowData.policyAclFlowEntry.match_criteria.ipProtoMask    = DEFAULT_PROTOCOLMASK,
      .flow.flowData.policyAclFlowEntry.match_criteria.dscp           = DEFAULT_DSCP,
      .flow.flowData.policyAclFlowEntry.match_criteria.dscpMask       = DEFAULT_DSCPMASK,
      .flow.flowData.policyAclFlowEntry.match_criteria.srcL4Port      = DEFAULT_SRCPORT,
      .flow.flowData.policyAclFlowEntry.match_criteria.srcL4PortMask  = DEFAULT_SRCPORTMASK,
      .flow.flowData.policyAclFlowEntry.match_criteria.destL4Port     = DEFAULT_DSTPORT,
      .flow.flowData.policyAclFlowEntry.match_criteria.destL4PortMask = DEFAULT_DSTPORTMASK,
      .flow.flowData.policyAclFlowEntry.groupID                       = 0,
      .flow.flowData.policyAclFlowEntry.queueIDAction                 = 0,
      .flow.flowData.policyAclFlowEntry.vlanPcpAction                 = 0,
      .flow.flowData.policyAclFlowEntry.dscpAction                    = 0,
      .flow.flowData.policyAclFlowEntry.outputPort                    = 0,
      .flow.flowData.policyAclFlowEntry.outputTunnelPort              = 0,
      .flow.flowData.policyAclFlowEntry.clearActions                  = 0,

    };

  /* Our argp parser. */
  struct argp argp =
    {
      .doc     = docBuffer,
      .options = options,
      .parser  = parse_opt,
      .args_doc = "[" ARG_DELETE "] [" ARG_LIST "]",
    };

  sprintf(versionBuf, "%s v%.1f", basename(strdup(__FILE__)), VERSION);
  argp_program_version = versionBuf;

  strcpy(docBuffer, "\nAdds, deletes or lists Policy ACL flows.\vDefault values:\n");
  i = strlen(docBuffer);
  i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "COUNT     = %d\n", DEFAULT_COUNT);
  i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "IFNUM     = %d\n", DEFAULT_IFNUM);
  if (0 != memcmp(arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcMacMask.addr, noMacMask.addr,
                  sizeof(arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcMacMask.addr)))
  {
    i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "SRCMAC    = %2.2x%2.2x.%2.2x%2.2x.%2.2x%2.2x\r\n",
                 arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcMac.addr[0], arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcMac.addr[1],
                 arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcMac.addr[2], arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcMac.addr[3],
                 arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcMac.addr[4], arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcMac.addr[5]);
  }
  if (0 != memcmp(arguments.flow.flowData.policyAclFlowEntry.match_criteria.destMacMask.addr, noMacMask.addr,
                  sizeof(arguments.flow.flowData.policyAclFlowEntry.match_criteria.destMacMask.addr)))
  {
    i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "DESTMAC   = %2.2x%2.2x.%2.2x%2.2x.%2.2x%2.2x\r\n",
                 arguments.flow.flowData.policyAclFlowEntry.match_criteria.destMac.addr[0], arguments.flow.flowData.policyAclFlowEntry.match_criteria.destMac.addr[1],
                 arguments.flow.flowData.policyAclFlowEntry.match_criteria.destMac.addr[2], arguments.flow.flowData.policyAclFlowEntry.match_criteria.destMac.addr[3],
                 arguments.flow.flowData.policyAclFlowEntry.match_criteria.destMac.addr[4], arguments.flow.flowData.policyAclFlowEntry.match_criteria.destMac.addr[5]);
  }
  if (0 != arguments.flow.flowData.policyAclFlowEntry.match_criteria.vlanIdMask)
  {
    i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "VLANID    = %d\n", DEFAULT_VLANID);
  }

  i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "Tunnel ID   = %d\n", DEFAULT_TUNNELID);

  i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "ETHERTYPE = 0x%4.4x\n", DEFAULT_ETHERTYPE);

  if (0 != arguments.flow.flowData.policyAclFlowEntry.match_criteria.dscpMask)
  {
    i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "DSCP      = %d\n", arguments.flow.flowData.policyAclFlowEntry.match_criteria.dscp);
  }
  if ((0 != arguments.flow.flowData.policyAclFlowEntry.match_criteria.sourceIp4Mask) &&
      (NULL != inet_ntop(AF_INET, &arguments.flow.flowData.policyAclFlowEntry.match_criteria.sourceIp4, buf, sizeof(buf))))
  {
    i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "SOURCEIP  = %s\n", buf);
  }
  if ((0 != arguments.flow.flowData.policyAclFlowEntry.match_criteria.destIp4Mask) &&
      (NULL != inet_ntop(AF_INET, &arguments.flow.flowData.policyAclFlowEntry.match_criteria.destIp4, buf, sizeof(buf))))
  {
    i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "DESTIP    = %s\n", buf);
  }
  if (0 != arguments.flow.flowData.policyAclFlowEntry.match_criteria.ipProtoMask)
  {
    i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "PROTOCOL  = %u\n", arguments.flow.flowData.policyAclFlowEntry.match_criteria.ipProto);
  }
  if (0 != arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcL4PortMask)
  {
    i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "SRCPORT   = %u\n", arguments.flow.flowData.policyAclFlowEntry.match_criteria.srcL4Port);
  }
  if (0 != arguments.flow.flowData.policyAclFlowEntry.match_criteria.destL4PortMask)
  {
    i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "DSTPORT   = %u\n", arguments.flow.flowData.policyAclFlowEntry.match_criteria.destL4Port);
  }
  i += snprintf(&docBuffer[i], sizeof(docBuffer) - i, "\n");

  /* Parse our arguments; every option seen by `parse_opt' will be reflected in
     `arguments'. */
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  rc = ofdpaClientInitialize(client_name);
  if (rc != OFDPA_E_NONE)
  {
    return rc;
  }

  rc = ofdpaFlowEntryInit(OFDPA_FLOW_TABLE_ID_ACL_POLICY, &flow);
  if (rc != OFDPA_E_NONE)
  {
    printf("\r\nFailed to initialize Policy ACL Flow Table.(rc = %d)\n", rc);
    return rc;
  }
  flow.idle_time = 30;

  if (0 == arguments.list)
  {
    memcpy(&flow.flowData.policyAclFlowEntry, &arguments.flow.flowData.policyAclFlowEntry, sizeof(flow.flowData.policyAclFlowEntry));
    flow.priority = arguments.flow.priority;
  }

  if (arguments.list || arguments.delete)
  {
    printf("%s up to %u Policy ACL flows.\r\n", arguments.list ? "Listing" : "Deleting", arguments.count);
  }
  else
  {
      printf("Adding %u Policy ACL flows with the following parameters:\r\n", arguments.count);
      displayAcl(&flow);
  }

  if (arguments.list || arguments.delete)
  {
    i = 0;
	
    rc = ofdpaFlowStatsGet(&flow, &flowStats);
    if (rc != OFDPA_E_NONE)
    {
      rc = ofdpaFlowNextGet(&flow, &flow);
    }
    while (rc == OFDPA_E_NONE)
    {
      i++;
      printf("%slow number %d.\r\n", arguments.delete ? "Deleting f": "F", i);
      displayAcl(&flow);

      if (arguments.delete)
      {
        rc = ofdpaFlowDelete(&flow);
        if (rc != 0)
        {
          printf("\r\nError deleting Policy ACL flow entry rc = %d.\r\n", rc);
        }
      }
      if ((arguments.count == 0) || (i < arguments.count))
      {
        rc = ofdpaFlowNextGet(&flow, &flow);
      }
      else
      {
        rc = OFDPA_E_NOT_FOUND;
      }
    }
    if ((1 == arguments.list) && (OFDPA_E_NOT_FOUND == rc) && (i < arguments.count))
    {
      printf("\r\nNo more entries found.\r\n");
    }
  }
  else
  {
    printf("\r\nDestination MAC address is incremented in each additional flow.\r\n\r\n");

    for (i = 0; i < arguments.count; i++)
    {
      rc = ofdpaFlowAdd(&flow);

      if (rc != 0)
      {
        printf("\r\nFailed to add Policy ACL flow entry. rc = %d.\r\n", rc);
        displayAcl(&flow);
        break;
      }
      incrementMac(&flow.flowData.policyAclFlowEntry.match_criteria.destMac);
    }
  }

  return rc;
}

