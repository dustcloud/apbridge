/*
 * Copyright (c) 2016, Linear Technology. All rights reserved.
 */

/* !!! This file must be include to project only ONE time !!! */

#include "dn_mesh.h"
const dn_ipv6_addr_t DN_MOTE_IPV6_BCAST_ADDR = {
   {
      0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
   }
};

const dn_ipv6_addr_t DN_MGR_IPV6_MULTICAST_ADDR = {
   {
      0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
   }
};

const dn_ipv6_addr_t DN_IPV6_LOCALHOST_ADDR = {
   {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
   }
};

const dn_macaddr_t DN_MGR_LONG_ADDR = {
   0x00, 0x17, 0x0D, 0x00, 0x00, 0x00, 0xFF, 0xFE
};

const dn_macaddr_t DN_BCAST_LONG_ADDR = {
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF 
};

const dn_macaddr_t DN_ZERO_LONG_ADDR = {0};

const dn_asn_t DN_ZERO_ASN = {{0}};

const dn_macprefix_t DN_DUST_MAC_PREFIX = {
   0x00, 0x17, 0x0D
};
