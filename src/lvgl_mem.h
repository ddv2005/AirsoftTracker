#pragma once
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
void *lvgl_malloc(size_t size);
void lvgl_free(void *ptr);
#ifdef __cplusplus
}
#endif
