/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: ice.h,v 1.1 2007/07/09 18:39:45 dave.demarle Exp $  */
/*  Date : $Date: 2007/07/09 18:39:45 $ */
/*  Version : $Revision: 1.1 $ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef ice_included
#define ice_included

#if !defined( WIN32 ) || defined(__CYGWIN__)
#define UNIX
#endif /* WIN32 */

#ifndef SWIG
/* System Includes */
# include "stdio.h"
# include "stdlib.h"
# include "sys/types.h"
# include "time.h"
# include "string.h"

# ifdef __hpux
#  include <sys/param.h>
# endif

# ifdef UNIX
#  include "sys/file.h"
#  include "strings.h"
#  define STRCASECMP strcasecmp
#  define STRNCASECMP strncasecmp
#  define STRCMP strcmp
#  define STRNCMP strncmp
# endif

# if defined(WIN32) && !defined(__CYGWIN__)
#  include "winsock.h"
/* String comparison routine. */
#  define STRCASECMP _stricmp
#  define STRNCASECMP _strnicmp
#  define STRCMP strcmp
#  define STRNCMP strncmp
# endif
#endif

/* System Dependent Defines */
#include "IceConfig.h"

#ifndef ICE_SYSTEM 
/* Force An Error */
ATTENTION ERROR_IN_ICE Probably a Bad/Missing IceConfig.h
#endif

#define ICE_MACHINE_TYPE  ICE_SYSTEM


#ifndef SWIG
# ifdef ICE_HAVE_FCNTL
#  include "fcntl.h"
# endif

# ifdef ICE_HAVE_MMAN
#  include "sys/mman.h"
# else
#  define ICE_HAS_NO_MMAP 1
# endif

# ifdef ICE_HAVE_NETINET
#  include        "netinet/in.h"
# endif
#endif

/* Defines */
#define ICE_INT    ICE_32_INT
#define ICE_TRUE       1
#define ICE_FALSE      0

#define ICE_SUCCESS    1
#define ICE_FAIL       -1

#define  ICE_CHAR_TYPE    1
#define  ICE_8_INT_TYPE    2
#define  ICE_32_INT_TYPE    3
#define  ICE_64_INT_TYPE    4
#define  ICE_FLOAT_TYPE    5
#define  ICE_DOUBLE_TYPE    6


/* Macros */
#define WORD2FLOAT( a ) ((a) == NULL ? ICE_FAIL : atof((a)))
#define WORD2INT( a )   ((a) == NULL ? ICE_FAIL : strtol( (a) , ( char **)NULL, 0))

#ifndef MAX
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef abs
#define ABS(a)    ((a) >= 0 ? (a) : -(a))
#endif

#define ICE_ERROR      fprintf(stderr, "Error on line #%d in file %s\n", __LINE__, __FILE__);

#define ICE_PERROR     fprintf(stderr, "Error on line #%d in file %s\n", __LINE__, __FILE__);

#define ICE_PORT(a)  (((a) * 5) + 7788 + 1)


#ifdef ICE_BYTE_ORDER_LITTLE
#define ICE_BYTE_ORDER_LITTLE
#endif
#ifdef ICE_BYTE_ORDER_LITTLE
#define  ICE_x86_BYTEORDER
#else
#define ICE_XDR_BYTEORDER
#endif

/************************************************/
/************************************************/
/*     Byte Order Macros       */
/************************************************/
/************************************************/

#ifdef ICE_have_64_bit_int

typedef ICE_64_INT     ICE_BIG_INT;  /* Biggest Int */

#define ICE_x86_SHIFT64(a)  \
    ( ICE_64_INT )( (( ( ICE_64_INT )(a) & 0xFFU ) << 56 ) | \
    (( ( ICE_64_INT )(a) & 0xFF00U ) << 40 ) | \
    (( ( ICE_64_INT )(a) & 0xFF0000U ) << 24 ) | \
    (( ( ICE_64_INT )(a) & 0xFF000000U ) << 8 ) | \
    (( ( ICE_64_INT )(a) >> 8 ) & 0xFF000000U ) | \
    (( ( ICE_64_INT )(a) >> 24 ) & 0xFF0000U )  | \
    (( ( ICE_64_INT )(a) >> 40 ) & 0xFF00U ) | \
    (( ( ICE_64_INT )(a) >> 56 ) & 0xFFU ) )

#define ICE_XDR_SHIFT64(a)  (a)  /* Already in Correct Order */

#else /* have_64_bit_int */

#define ICE_x86_SHIFT64(a)  (a)
#define ICE_XDR_SHIFT64(a)  (a)

typedef ICE_32_INT     ICE_BIG_INT;

#endif /* have_64_bit_int */


#define ICE_x86_SHIFT32(a)  htonl((a))
#define ICE_XDR_SHIFT32(a)  (a)

#ifdef ICE_x86_BYTEORDER
#define  ICE_SHIFT64(a)  ICE_x86_SHIFT64((a))
#define  ICE_SHIFT32(a)  ICE_x86_SHIFT32((a))
#endif

#ifdef ICE_XDR_BYTEORDER
#define  ICE_SHIFT32(a)  (a)
#define  ICE_SHIFT64(a)  (a)
#endif

#define  ICE_SHIFT(a)  (sizeof((a)) == 8 ? ICE_SHIFT64(a) : ICE_SHIFT32(a))
#define ICE_SHIFT_FROM32(a)    ICE_SHIFT(a)
#define ICE_SHIFT_TO_XDR32(a)    ICE_SHIFT(a)
#define ICE_SHIFT_FROM_XDR64(a)  ICE_SHIFT(a)
#define ICE_SHIFT_TO_XDR64(a)    ICE_SHIFT(a)
#define ICE_SHIFT_FROM_XDR(a)    ICE_SHIFT(a)
#define ICE_SHIFT_TO_XDR(a)    ICE_SHIFT(a)



#endif /* ice_included */
