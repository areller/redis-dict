#include "../lib/redismodule.h"
#include "redisdict.h"

int rdictAppend(Ctx *ctx, RMSTR **str, const char *toAppend, size_t toAppendLen) {
    size_t nativeStrLen;
    const char *nativeStr = RedisModule_StringPtrLen(*str, &nativeStrLen);

    int bracketStart;
    for (bracketStart = 0; bracketStart < nativeStrLen; bracketStart++) {
        if (nativeStr[bracketStart] == '{') break;
    }

    if (bracketStart < nativeStrLen) {
        return RedisModule_StringAppendBuffer(ctx, *str, toAppend, toAppendLen);
    }

    RMSTR *prefixStr = RedisModule_CreateString(ctx, SD_PREFIX, SD_PREFIX_LEN);
    if (RedisModule_StringAppendBuffer(ctx, prefixStr, nativeStr, nativeStrLen) == ERR ||
        RedisModule_StringAppendBuffer(ctx, prefixStr, SD_SUFFIX, SD_SUFFIX_LEN) == ERR ||
        RedisModule_StringAppendBuffer(ctx, prefixStr, toAppend, toAppendLen) == ERR) {
        return ERR;
    }

    *str = prefixStr;
    return OK;
}

int rdictRange(Ctx *ctx, RMSTR **argv, int argc, int reverse) {
    AUTO(ctx);
    if (argc != 4 && argc != 5) return RedisModule_WrongArity(ctx);

    RMSTR *hashStr = argv[1];
    RMSTR *zsetStr = RedisModule_CreateStringFromString(ctx, hashStr);

    if (rdictAppend(ctx, &hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN) == ERR ||
        rdictAppend(ctx, &zsetStr, SD_ZSET_APPEND, SD_ZSET_APPEND_LEN) == ERR) {
        RedisModule_ReplyWithError(ctx, "Could not append to key");
        return ERR;
    }

    RedisModuleKey *hashKey = RedisModule_OpenKey(ctx, hashStr, REDISMODULE_WRITE);

    RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);
    size_t count = 0;

    RedisModuleCallReply *reply = RedisModule_Call(ctx, reverse == 0 ? "ZRANGE" : "ZREVRANGE", "sss", zsetStr, argv[2], argv[3]);
    RedisModuleCallReply *elem;

    while ((elem = RedisModule_CallReplyArrayElement(reply, count)) != NULL) {
        RMSTR *key = RedisModule_CreateStringFromCallReply(elem);
        RedisModule_ReplyWithString(ctx, key);
        RMSTR *val;
        RedisModule_HashGet(hashKey, REDISMODULE_HASH_NONE, key, &val, NULL);
        RedisModule_ReplyWithString(ctx, val);
        count++;
    }

    RedisModule_ReplySetArrayLength(ctx, count * 2);
    return OK;
}

/**
 * rdict.sdset {NAME} {KEY} {VALUE}
 * Sets a KV Pair in a sorted dictionary
 * Key must be a double number
 */
int RDICT_SDSET(Ctx *ctx, RMSTR **argv, int argc) {
    AUTO(ctx);
    if (argc != 4) return RedisModule_WrongArity(ctx);

    RMSTR *hashStr = argv[1];
    RMSTR *zsetStr = RedisModule_CreateStringFromString(ctx, hashStr);

    if (rdictAppend(ctx, &hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN) == ERR || 
        rdictAppend(ctx, &zsetStr, SD_ZSET_APPEND, SD_ZSET_APPEND_LEN) == ERR) {
        RedisModule_ReplyWithError(ctx, "Could not append to key");
        return ERR;
    }

    RedisModuleKey *hashKey = RedisModule_OpenKey(ctx, hashStr, REDISMODULE_WRITE);
    if (RedisModule_HashSet(hashKey, REDISMODULE_HASH_NONE, argv[2], argv[3], NULL) == 0) {
        double score;
        if (RedisModule_StringToDouble(argv[2], &score) == ERR) {
            RedisModule_HashSet(hashKey, REDISMODULE_HASH_NONE, argv[2], REDISMODULE_HASH_DELETE, NULL);
            RedisModule_ReplyWithError(ctx, "Key must be a number");
            return ERR;
        }

        RedisModuleKey *zsetKey = RedisModule_OpenKey(ctx, zsetStr, REDISMODULE_WRITE);
        RedisModule_ZsetAdd(zsetKey, score, argv[2], NULL);
        RedisModule_ReplyWithLongLong(ctx, 1);
    }
    else {
        RedisModule_ReplyWithLongLong(ctx, 0);
    }

    return OK;
}

/**
 * rdict.sdmset {NAME} {KEY} {VALUE} {KEY2} {VALUE2} ...
 * Sets pairs of KV in a sorted dictionary
 * Keys must be double numbers
 */
int RDICT_SDMSET(Ctx *ctx, RMSTR **argv, int argc) {
    AUTO(ctx);
    if (argc < 4 || argc % 2 != 0) return RedisModule_WrongArity(ctx);

    RMSTR *hashStr = argv[1];
    RMSTR *zsetStr = RedisModule_CreateStringFromString(ctx, hashStr);

    if (rdictAppend(ctx, &hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN) == ERR || 
        rdictAppend(ctx, &zsetStr, SD_ZSET_APPEND, SD_ZSET_APPEND_LEN) == ERR) {
        RedisModule_ReplyWithError(ctx, "Could not append to key");
        return ERR;
    }

    RedisModuleKey *hashKey = RedisModule_OpenKey(ctx, hashStr, REDISMODULE_WRITE);
    RedisModuleKey *zsetKey = NULL;

    size_t count = 0;
    int pos;
    for(pos = 2; pos < argc; pos += 2) {
        double score;
        if (RedisModule_HashSet(hashKey, REDISMODULE_HASH_NONE, argv[pos], argv[pos + 1], NULL) == 0 &&
            RedisModule_StringToDouble(argv[pos], &score) != ERR) {
            if (zsetKey == NULL) {
                zsetKey = RedisModule_OpenKey(ctx, zsetStr, REDISMODULE_WRITE);
            }

            RedisModule_ZsetAdd(zsetKey, score, argv[pos], NULL);
            count++;
        }
    }

    RedisModule_ReplyWithLongLong(ctx, count);

    return OK;
}

/**
 * rdict.sddel {NAME} {KEY} {KEY2} ...
 * Deletes keys in a sorted dictionary
 * Key must be a double number
 */
int RDICT_SDDEL(Ctx *ctx, RMSTR **argv, int argc) {
    AUTO(ctx);
    if (argc < 3) return RedisModule_WrongArity(ctx);

    RMSTR *hashStr = argv[1];
    RMSTR *zsetStr = RedisModule_CreateStringFromString(ctx, hashStr);

    if (rdictAppend(ctx, &hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN) == ERR ||
        rdictAppend(ctx, &zsetStr, SD_ZSET_APPEND, SD_ZSET_APPEND_LEN) == ERR) {
        RedisModule_ReplyWithError(ctx, "Could not append to key");
        return ERR;
    }

    RedisModuleKey *hashKey = RedisModule_OpenKey(ctx, hashStr, REDISMODULE_WRITE);
    RedisModuleKey *zsetKey = NULL;

    size_t count = 0;
    int pos;
    for (pos = 2; pos < argc; pos++) {
        if (RedisModule_HashSet(hashKey, REDISMODULE_HASH_NONE, argv[pos], REDISMODULE_HASH_DELETE, NULL) != 0) {
            size_t hashLen = RedisModule_ValueLength(hashKey);
            if (zsetKey == NULL) {
                zsetKey = RedisModule_OpenKey(ctx, zsetStr, REDISMODULE_WRITE);
            }

            if (hashLen == 0) {
                RedisModule_DeleteKey(zsetKey);
            }
            else {
                RedisModule_ZsetRem(zsetKey, argv[pos], NULL);
            }

            count++;
        }
    }

    RedisModule_ReplyWithLongLong(ctx, count);

    return OK;
}

/**
 * rdict.sdadel {NAME} {NAME2} ...
 * Deletes sorted dictionaries
 */
int RDICT_SDADEL(Ctx *ctx, RMSTR **argv, int argc) {
    AUTO(ctx);
    if (argc < 2) return RedisModule_WrongArity(ctx);

    size_t count = 0;
    int pos;
    for (pos = 1; pos < argc; pos++) {
        RMSTR *hashStr = argv[pos];
        RMSTR *zsetStr = RedisModule_CreateStringFromString(ctx, hashStr);

        if (rdictAppend(ctx, &hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN) == ERR ||
            rdictAppend(ctx, &zsetStr, SD_ZSET_APPEND, SD_ZSET_APPEND_LEN) == ERR) {
            continue;
        }

        RedisModuleKey *hashKey = RedisModule_OpenKey(ctx, hashStr, REDISMODULE_WRITE);
        if (RedisModule_ValueLength(hashKey) == 0 || RedisModule_DeleteKey(hashKey) == ERR) continue;

        RedisModuleKey *zsetKey = RedisModule_OpenKey(ctx, zsetStr, REDISMODULE_WRITE);
        RedisModule_DeleteKey(zsetKey);
        
        count++;
    }

    RedisModule_ReplyWithLongLong(ctx, count);

    return OK;
}

/**
 * rdict.sdget {NAME} {KEY}
 * Gets the value of a given key in a sorted dictionary
 * Key must be a double number
 */
int RDICT_SDGET(Ctx *ctx, RMSTR **argv, int argc) {
    AUTO(ctx);
    if (argc != 3) return RedisModule_WrongArity(ctx);

    RMSTR *hashStr = argv[1];
    if (rdictAppend(ctx, &hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN) == ERR) {
        RedisModule_ReplyWithError(ctx, "Could not append to key");
        return ERR;
    }

    RMSTR *rtVal;
    RedisModuleKey *hashKey = RedisModule_OpenKey(ctx, hashStr, REDISMODULE_WRITE);
    if (RedisModule_HashGet(hashKey, REDISMODULE_HASH_NONE, argv[2], &rtVal, NULL) == ERR) {
        RedisModule_ReplyWithError(ctx, "Could not get hash key");
        return ERR;
    }

    if (!rtVal) {
        RedisModule_ReplyWithNull(ctx);
    }
    else {
        RedisModule_ReplyWithString(ctx, rtVal);
    }

    return OK;
}

/**
 * rdict.sdrange {NAME} {START} {STOP}
 * Ranges over the dictionary given start and stop position
 */
int RDICT_SDRANGE(Ctx *ctx, RMSTR **argv, int argc) {
    return rdictRange(ctx, argv, argc, 0);
}

/**
 * rdict.sdrange {NAME} {START} {STOP}
 * Ranges over the dictionary given start and stop position
 * In Reverse!
 */
int RDICT_SDRRANGE(Ctx *ctx, RMSTR **argv, int argc) {
    return rdictRange(ctx, argv, argc, 1);
}

int RedisModule_OnLoad(Ctx *ctx, RMSTR **argv, int argc) {
    UNUSED(argc);
    UNUSED(argv);

    if (RedisModule_Init(ctx, "redisdict", 2, REDISMODULE_APIVER_1) == ERR) {
        return ERR;
    }

    if (CreateCMD(ctx, "rdict.sdset", RDICT_SDSET, "write", 1, 1, 1) == ERR) {
        return ERR;
    }

    if (CreateCMD(ctx, "rdict.sdmset", RDICT_SDMSET, "write", 1, 1, 1) == ERR) {
        return ERR;
    }

    if (CreateCMD(ctx, "rdict.sddel", RDICT_SDDEL, "write", 1, 1, 1) == ERR) {
        return ERR;
    }

    if (CreateCMD(ctx, "rdict.sdadel", RDICT_SDADEL, "write", 1, 1, 1) == ERR) {
        return ERR;
    }

    if (CreateCMD(ctx, "rdict.sdget", RDICT_SDGET, "readonly", 1, 1, 1) == ERR) {
        return ERR;
    }

    if (CreateCMD(ctx, "rdict.sdrange", RDICT_SDRANGE, "readonly", 1, 1, 1) == ERR) {
        return ERR;
    }

    if (CreateCMD(ctx, "rdict.sdrrange", RDICT_SDRRANGE, "readonly", 1, 1, 1) == ERR) {
        return ERR;
    }

    return OK;
}