#include "photo_service.h"
#include <stdlib.h>
#include <string.h>

void photo_result_clear(photo_result_t *result)
{
    if (!result) return;
    photo_file_list_free(&result->files);
    free(result->pixels);
    memset(result, 0, sizeof(*result));
}
