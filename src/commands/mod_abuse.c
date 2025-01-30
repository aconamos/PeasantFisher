#include "discord.h"

#include "../infra/arena.c"
#include "../project_vars.h"


#ifndef COMMAND_ABUSE
#define COMMAND_ABUSE

void
abuse_cb(struct discord *client, const struct discord_interaction *event)
{
    // Extract options

    // If the time is invalid, clamp to 28d (does discord API do this for us?)

    // Try to modify user
    // Two potential options:
    //  fail -> laugh at this user blah blah blah
    //  succ -> x was shot by y! reason: "reason"
}

#endif