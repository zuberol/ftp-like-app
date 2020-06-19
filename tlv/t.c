#include "tlv_chain.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int32_t tlv_chain_add_int32(struct tlv_chain *a, int32_t x)
{
    return tlv_chain_add_raw(a, 1, 4, &x);
}

// add tlv object which contains null terminated string
int32_t tlv_chain_add_str(struct tlv_chain *a,  char *str)
{
    return tlv_chain_add_raw(a, 2, strlen(str) + 1, str);
}

int32_t tlv_chain_add_raw(struct tlv_chain *a, unsigned char type, int16_t size, unsigned char *bytes)
{
    if(a == NULL || bytes == NULL)
        return -1;

    // all elements used in chain?
    if(a->used == 50)
        return -1;

    int index = a->used;
    a->object[index].type = type;
    a->object[index].size = size;
    a->object[index].data = malloc(size);
    memcpy(a->object[index].data, bytes, size);

    // increase number of tlv objects used in this chain
    a->used++;

    // success
    return 0;
}

int32_t tlv_chain_free(struct tlv_chain *a)
{
    if(a == NULL)
        return -1;

    for(int i =0; i < a->used; i++)
    {
        free(a->object[i].data);

        a->object[i].data = NULL;
    }

    return 0;
}

// serialize the tlv chain into byte array
int32_t tlv_chain_serialize(struct tlv_chain *a, unsigned char *dest, /* out */ int32_t* count)
{
    if(a == NULL || dest == NULL)
        return -1;

    // Number of bytes serialized
    int32_t counter = 0;

    for(int i = 0; i < a->used; i++)
    {
        dest[counter] = a->object[i].type;
        counter++;

        memcpy(&dest[counter], &a->object[i].size, 2);
        counter += 2;

        memcpy(&dest[counter], a->object[i].data, a->object[i].size);
        counter += a->object[i].size;
    }

    // Return number of bytes serialized
    *count = counter;

    // success
    return 0;
}

int32_t tlv_chain_deserialize(unsigned char *src, struct tlv_chain *dest, int32_t length)
{
    if(dest == NULL || src == NULL)
        return -1;

    // we want an empty chain
    if(dest->used != 0)
        return -1;

    int32_t counter = 0;
    while(counter < length)
    {
        if(dest->used == 50)
            return -1;

        // deserialize type
        dest->object[dest->used].type = src[counter];
        counter++;

        // deserialize size
        memcpy(&dest->object[dest->used].size, &src[counter], 2);
        counter+=2;

        // deserialize data itself, only if data is not NULL
        if(dest->object[dest->used].size > 0)
        {
            dest->object[dest->used].data = malloc(dest->object[dest->used].size);
            memcpy(dest->object[dest->used].data, &src[counter], dest->object[dest->used].size);
            counter += dest->object[dest->used].size;
        }else
        {
            dest->object[dest->used].data = NULL;
        }

        // increase number of tlv objects reconstructed
        dest->used++;
    }

    // success
    return 0;

}



int32_t tlv_chain_print(struct tlv_chain *a)
{
    if(a == NULL)
        return -1;

    // go through each used tlv object in the chain
    for(int i =0; i < a->used; i++)
    {

        if(a->object[i].type == 1)
        {
            // int32
            int32_t x;
            memcpy(&x, a->object[i].data, sizeof(int32_t));
            printf("%d \n",x);

        }else if(a->object[i].type == 2)
        {
            // string
            printf("%s \n",a->object[i].data);
        }
    }


    return 0;
}



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