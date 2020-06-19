#include <stdio.h>
#include "tlv_chain.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, const char * argv[])
{
    struct tlv_chain chain1, chain2;
    memset(&chain1, 0, sizeof(chain1));
    memset(&chain2, 0, sizeof(chain2));
    unsigned char chainbuff[2048] = {0};
    int32_t l = 0;

    tlv_chain_add_int32(&chain1, 31144);
    tlv_chain_add_str(&chain1, "george");
    tlv_chain_add_int32(&chain1, 7);
    tlv_chain_add_str(&chain1, "998967-44-33-44-12");
    tlv_chain_add_str(&chain1, "Grand Chamption Atlanta; details: Ave12");
    tlv_chain_add_int32(&chain1, 7900);

    // serialization/deserialization test
    tlv_chain_serialize(&chain1, chainbuff, &l);
    tlv_chain_deserialize(chainbuff, &chain2, l);

    // print the tlv chain contents
    tlv_chain_print(&chain2);

    // free each chain
    tlv_chain_free(&chain1);
    tlv_chain_free(&chain2);

    return 0;
}