#ifndef tlv_tlv_chain_h
#define tlv_tlv_chain_h
#include <stdint.h>

// TLV data structure
struct tlv
{
    int8_t type;    // type
    uint8_t * data; // pointer to data
    int16_t size;   // size of data
};

// TLV chain data structure. Contains array of (50) tlv
// objects. 
struct tlv_chain
{
    struct tlv object[50];
    uint8_t used; // keep track of tlv elements used
};

int32_t tlv_chain_add_int32(struct tlv_chain *a, int32_t x);
int32_t tlv_chain_add_str(struct tlv_chain *a, char *str);
int32_t tlv_chain_add_raw(struct tlv_chain *a, unsigned char type, int16_t size, unsigned char *bytes);
int32_t tlv_chain_serialize(struct tlv_chain *a, unsigned char *dest, int32_t *count);
int32_t tlv_chain_deserialize(unsigned char *src, struct tlv_chain *dest, int32_t length);
int32_t tlv_chain_print(struct tlv_chain *a);
int32_t tlv_chain_free(struct tlv_chain *a);

#endif