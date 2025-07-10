#pragma once

// 优先使用RT-Thread的实现
#include "../../../rtsmart/userapps/sdk/rt-thread/include/libc/libc_signal.h"

// 屏蔽系统定义
#define _SYS_WAIT_H_
#define __idtype_t_defined

