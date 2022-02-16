#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct bheader{
    bool used;
    size_t size;
    // next header
    struct bheader* next;
}bheader;