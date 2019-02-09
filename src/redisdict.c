#include "../lib/redismodule.h"
#include "redisdict.h"

int rdictAppend(Ctx *ctx, RMSTR *str, const char *toAppend, size_t toAppendLen, RMSTR **res) {
    size_t nativeStrLen;
    const char *nativeStr = RedisModule_StringPtrLen(str, &nativeStrLen);

    RMSTR *prefixStr = RedisModule_CreateString(ctx, SD_PREFIX, SD_PREFIX_LEN);
    if (RedisModule_StringAppendBuffer(ctx, prefixStr, nativeStr, nativeStrLen) == ERR) {
        return ERR;
    }

    if (RedisModule_StringAppendBuffer(ctx, prefixStr, toAppend, toAppendLen) == ERR) {
        return ERR;
    }

    *res = prefixStr;
    return OK;
}

int rdictRange(Ctx *ctx, RMSTR **argv, int argc, int reverse) {
    AUTO(ctx);
    if (argc != 4 && argc != 5) return RedisModule_WrongArity(ctx);

    RMSTR *hashStr = argv[1];
    RMSTR *zsetStr = RedisModule_CreateStringFromString(ctx, hashStr);

    if (rdictAppend(ctx, hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN, &hashStr) == ERR ||
    rdictAppend(ctx, zsetStr, SD_ZSET_APPEND, SD_ZSET_APPEND_LEN, &zsetStr) == ERR) {
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

    if (rdictAppend(ctx, hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN, &hashStr) == ERR || 
    rdictAppend(ctx, zsetStr, SD_ZSET_APPEND, SD_ZSET_APPEND_LEN, &zsetStr) == ERR) {
        RedisModule_ReplyWithError(ctx, "Could not append to key");
        return ERR;
    }

    RedisModuleKey *hashKey = RedisModule_OpenKey(ctx, hashStr, REDISMODULE_WRITE);
    if (RedisModule_HashSet(hashKey, REDISMODULE_HASH_NONE, argv[2], argv[3], NULL) == 0) {
        double score;
        if (RedisModule_StringToDouble(argv[2], &score) == ERR) {
            RedisModule_HashSet(hashKey, REDISMODULE_HASH_NONE, argv[2], REDISMODULE_HASH_DELETE, NULL);
            RedisModule_ReplyWithArray(ctx, "Key must be a number");
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
 * rdict.sddel {NAME} {KEY}
 * Deletes a key in a sorted dictionary
 * Key must be a double number
 */
int RDICT_SDDEL(Ctx *ctx, RMSTR **argv, int argc) {
    AUTO(ctx);
    if (argc != 3) return RedisModule_WrongArity(ctx);

    RMSTR *hashStr = argv[1];
    RMSTR *zsetStr = RedisModule_CreateStringFromString(ctx, hashStr);

    if (rdictAppend(ctx, hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN, &hashStr) == ERR ||
    rdictAppend(ctx, zsetStr, SD_ZSET_APPEND, SD_ZSET_APPEND_LEN, &zsetStr) == ERR) {
        RedisModule_ReplyWithError(ctx, "COuld not append to key");
        return ERR;
    }

    RedisModuleKey *hashKey = RedisModule_OpenKey(ctx, hashStr, REDISMODULE_WRITE);
    if (RedisModule_HashSet(hashKey, REDISMODULE_HASH_NONE, argv[2], REDISMODULE_HASH_DELETE, NULL) != 0) {
        size_t hashLen = RedisModule_ValueLength(hashKey);
        RedisModuleKey *zsetKey = RedisModule_OpenKey(ctx, zsetStr, REDISMODULE_WRITE);

        if (hashLen == 0) {
            RedisModule_DeleteKey(zsetKey);
        }
        else {
            RedisModule_ZsetRem(zsetKey, argv[2], NULL);
        }

        RedisModule_ReplyWithLongLong(ctx, 1);
    }
    else {
        RedisModule_ReplyWithLongLong(ctx, 0);
    }

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
    if (rdictAppend(ctx, hashStr, SD_HASHTABLE_APPEND, SD_HASHTABLE_APPEND_LEN, &hashStr) == ERR) {
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

    if (CreateCMD(ctx, "rdict.sddel", RDICT_SDDEL, "write", 1, 1, 1) == ERR) {
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