//
//  os.h
//  desktop
//
//  Created by Taras Shchybovyk on 8/10/18.
//  Copyright © 2018 Taras Shchybovyk. All rights reserved.
//

#ifndef os_h
#define os_h

#include "cx.h"
#include <assert.h>
#include <stdio.h>

#define EXCEPTION
#define INVALID_PARAMETER
#define EXCEPTION_OVERFLOW

#define BEGIN_TRY
#define END_TRY
#define TRY
#define CATCH_OTHER(e) if (false)
#define FINALLY

#define PRINTF(X) puts(X)
#define THROW(X)  assert(false)

#define os_memset  memset
#define os_memmove memcpy

#endif /* os_h */
