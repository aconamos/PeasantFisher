#include <time.h>

#include "discord.h"

#include "arena.c" // TODO better way to do this include 


#ifndef STRUCTS
#define STRUCTS

// #ifndef ACTIVE_YEETS_SIZE
// #define ACTIVE_YEETS_SIZE 20 // make compiler happy
// #endif

struct message_identifier {
    u64snowflake message;
    u64snowflake channel;
};

struct yeet {
    struct message_identifier m_id;
    struct Arena *arena;
    u64snowflake author;
    u64snowflake victim;
    u64snowflake i_id;
    time_t timestamp;
    int y_reacts;
    int x_reacts;
    char *token;
};

struct yeet_with_users {
    struct yeet *yeet;
    char *users_msg;
};

struct get_reactions_params {
    struct yeet *yeet;
    char *emoji;
};

struct instance_data {
    struct yeet **active_yeets;
    int active_yeets_size;
    int yeets_cnt;
};
#endif