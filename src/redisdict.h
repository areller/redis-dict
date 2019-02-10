#include <string.h>
#include "../lib/redismodule.h"

#define OK REDISMODULE_OK
#define ERR REDISMODULE_ERR

#define UNUSED(CTX) REDISMODULE_NOT_USED(CTX)
#define AUTO(CTX) RedisModule_AutoMemory(CTX)
#define CreateCMD(CTX, NAME, FUNC, FLAGS, FIRST_KEY, LAST_KEY, STEP) RedisModule_CreateCommand(CTX, NAME, FUNC, FLAGS, FIRST_KEY, LAST_KEY, STEP)

#define SD_PREFIX "{"
#define SD_PREFIX_LEN strlen(SD_PREFIX)

#define SD_SUFFIX "}"
#define SD_SUFFIX_LEN strlen(SD_SUFFIX)

#define SD_HASHTABLE_APPEND "_sd1"
#define SD_HASHTABLE_APPEND_LEN strlen(SD_HASHTABLE_APPEND)

#define SD_ZSET_APPEND "_sd2"
#define SD_ZSET_APPEND_LEN strlen(SD_ZSET_APPEND)

typedef RedisModuleCtx Ctx;
typedef RedisModuleString RMSTR;
typedef uint8_t RStatus;