#include <stdio.h>
#include "allocator.c"

int main(){
    char s1[] = "heya";
    char s2[] = "123";
    char s3[] = "ala ma kota kot ma ale";
    char* p1 = writestr(s1);
    char* p2 = writestr(s3);
    free(p2);
    char* p3 = writestr(s3);
    char* p4 = writestr(s2);
    printf("\n\n\nIWONA\n\n\n");
}
