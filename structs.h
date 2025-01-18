#include <time.h>

#ifndef DISCORD_H
#include "discord.h"
#endif

#ifndef STRUCTS
#define STRUCTS
struct message_identifier {
    u64snowflake message;
    u64snowflake channel;
};

struct yeet {
    struct message_identifier m_id;
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
#endif