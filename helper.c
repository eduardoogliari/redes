#include <stdlib.h>
#include <string.h>
#include "helper.h"
#include "defs.h"

//####################################################################
char * alloc_string( const char * src ) {
    if( !src ) { return NULL; }

    const int lenght = strlen( src ) + 1;
    const int countBytes = sizeof(char) * lenght ;

    char * buffer = malloc( countBytes );
    memset( buffer, 0, countBytes );

    strcpy( buffer, src );
    return buffer;
}