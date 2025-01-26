#include "discord.h"

#include "../infra/structs.c"
#include "../infra/arena.c"
#include "../infra/yeet_array.c"
#include "../project_vars.h"


#ifndef COMMAND_YEET
#define COMMAND_YEET

void
edit_message_returned_timer_cb(struct discord *client, struct discord_timer *timer)
{
    log_debug("edit_message_returned_timer_cb called!");
    struct yeet_with_users *yww = timer->data;

    char *yeet_msg = arena_calloc(yww->yeet->arena, 2000);
    cog_asprintf(&yeet_msg, "User <@!%ld> was yeeted in %.2f seconds! They have since returned.\nBrought to you by %s", yww->yeet->victim, (float)yww->yeet->deltatime_ms/1000, yww->users_msg);

    discord_edit_message(client, yww->yeet->m_id.channel, yww->yeet->m_id.message, &(struct discord_edit_message) {
        .content = yeet_msg,
    }, NULL);

    free(yeet_msg);

    struct yeet *yeet = yww->yeet;
    log_debug("edit_message_returned_timer_cb: freeing arena @ %p\n\t\tfreeing yeet @ %p", yeet->arena, yeet);
    arena_free(yeet->arena);
    nullifree_yeet(client, yeet);
}

void
edit_message_done(struct discord *client, struct discord_response *resp, const struct discord_message *ret)
{
    log_debug("edit_message_done called!");
    struct yeet_with_users *yww = resp->data;
    yww->yeet->m_id.channel = ret->channel_id;
    yww->yeet->m_id.message = ret->id;

    discord_timer(client, edit_message_returned_timer_cb, NULL, resp->data, YEET_DURATION * 1000);
}

void 
yeet_fail(struct discord *client, struct discord_response *resp)
{
    log_debug("yeet_fail called!");
    struct yeet_with_users *yww_data = resp->data;
    struct yeet *yeet = yww_data->yeet;

    char *yeet_msg = arena_calloc(yeet->arena, 2000); // TODO get an exact size
    cog_asprintf(&yeet_msg, "Sorry %s, but I couldn't yeet <@!%ld>. Shame them publicly instead.", yww_data->users_msg, yeet->victim);

    discord_delete_message(client, yeet->m_id.channel, yeet->m_id.message, NULL, NULL);
    discord_create_message(client, yeet->m_id.channel, &(struct discord_create_message) {
        .content = yeet_msg,
    }, NULL);

    free(yeet_msg);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void 
yeet_succ(struct discord *client, struct discord_response *resp, const struct discord_guild_member *ret)
{
    log_debug("yeet_succ called!");
    struct yeet_with_users *yww_data = resp->data;
    struct yeet *yeet = yww_data->yeet;

    u64unix_ms deltaTimeMs = (cog_timestamp_ms() - yeet->timestamp);
    yeet->deltatime_ms = deltaTimeMs;
    char *yeet_msg = arena_calloc(yeet->arena, 2000); // TODO get an exact size
    cog_asprintf(&yeet_msg, "User <@!%ld> yeeted in %.2f seconds! They will return in <t:%ld:R>\nBrought to you by %s", yeet->victim, (float)deltaTimeMs/1000, time(NULL) + YEET_DURATION, yww_data->users_msg);

    discord_delete_message(client, yeet->m_id.channel, yeet->m_id.message, NULL, NULL);
    discord_create_message(client, yeet->m_id.channel, &(struct discord_create_message) {
        .content = yeet_msg,
    }, &(struct discord_ret_message) {
        .done = edit_message_done,
        .data = yww_data,
    });

    free(yeet_msg);
}

void
get_users_done(struct discord *client, struct discord_response *resp, const struct discord_users *ret) 
{
    struct get_reactions_params *params = resp->data;
    char *emoji = params->emoji;
    struct yeet *yeet = params->yeet;
    int is_backfire = 0;
    u64snowflake target;

    if (strcmp(emoji, YES_EMOJI) == 0) {
        yeet->y_reacts = ret->size;
        is_backfire = ret->size;
    }
    else if (strcmp(emoji, NO_EMOJI) == 0) {
        yeet->x_reacts = ret->size;
        is_backfire = -ret->size;
    }

    if (abs(is_backfire) < VOTE_COUNT) return;
    if (is_backfire > 0) target = yeet->victim;
    else target = yeet->author;
    // TODO: There's some duplicated logic above here that could use a trim
    
    log_debug("TARGET BEING YEETED: %ld", target);
    log_debug("WAS BACKFIRE? %d", is_backfire);
    log_debug("USERS SIZE: %ld", ret->size);

    // Build the list of users string.
    char *list = arena_calloc(yeet->arena, 2000);
    char *_fstr = arena_calloc(yeet->arena, 25);
    for (int i = 0; i < ret->size; i++) {
        if (ret->array[i].id == BOT_ID) continue;
        cog_asprintf(&_fstr, "<@!%ld> ", ret->array[i].id);
        strcat(list, _fstr);
        free(_fstr);
    }

    log_debug("g_u_d: list: %s", list);

    // For some reason, we gotta use heap memory here. 
    struct yeet_with_users *data = arena_alloc(yeet->arena, sizeof(struct yeet_with_users));
    data->users_msg = list;
    data->yeet = yeet;

    discord_modify_guild_member(client, GUILD_ID, target, &(struct discord_modify_guild_member) {
        .communication_disabled_until = (u64unix_ms)((time(NULL) + YEET_DURATION) * 1000), // This needs an extra *1000 because for some reason discord's timestamps have extra precision. /shrug
    }, &(struct discord_ret_guild_member) {
        .done = yeet_succ,
        .fail = yeet_fail,
        .data = data,
    });
}

/**
 * This callback serves to delete a message and, if necessary, remove the corresponding yeet
 * from the list of active yeets. It is called from a discord timer.
 */
void
murder_message(struct discord *client, struct discord_timer *timer)
{
    // Delete the message
    struct message_identifier *data = (timer->data);
    // In a future refactor, it might be structured such that we don't make this call without first checking
    // that the operation will go through. For now, we need the data.
    if (discord_delete_message(client, data->channel, data->message, NULL, NULL) == CCORD_OK) {
        struct yeet *yeet = get_yeet_by_id(client, data->message);
        if (yeet != NULL) {
            log_debug("murder_message: freeing arena @ %p\n\t\tfreeing yeet @ %p", yeet->arena, yeet);
            arena_free(yeet->arena);
            nullifree_yeet(client, yeet);
        }
        mod_yeet_count(client, 1);
    }
}

/**
 * This callback serves to finish constructing yeet data and start the timer to delete it.
 * It is the last call to construct the yeet following the initial reaction from the bot.
 */
#pragma GCC diagnostic ignored "-Wunused-parameter"
void 
init_react_cb_done_get(struct discord *client, struct discord_response *resp, const struct discord_message *ret) 
{
    discord_create_reaction(client, ret->channel_id, ret->id, 0, YES_EMOJI, NULL);
    discord_create_reaction(client, ret->channel_id, ret->id, 0, NO_EMOJI, NULL);

    log_debug("Interaction ID (CB): %ld", ret->interaction->id);
    struct yeet *yeet = get_yeet_by_token(client, ret->interaction->id);
    yeet->m_id.channel = ret->channel_id;
    yeet->m_id.message = ret->id;

    struct message_identifier *p_message_identifier = arena_alloc(yeet->arena, sizeof(struct message_identifier));
    p_message_identifier->channel = ret->channel_id;
    p_message_identifier->message = ret->id;
    // TODO: Figure out how to use concord's built in cleanup ability for this parameter passing

    discord_timer(client, murder_message, NULL, p_message_identifier, YEET_VOTING_SECONDS * 1000);
}

/**
 * This callback serves to call the other, more useful callback.
 * I'm not sure why it's required; I copied it from somewhere.
 */
#pragma GCC diagnostic ignored "-Wunused-parameter"
void 
init_react_cb_done(struct discord *client, struct discord_response *resp, const struct discord_interaction_response *ret) 
{
    const struct discord_interaction *event = resp->keep;

    discord_get_original_interaction_response(client, BOT_ID, event->token, &(struct discord_ret_message) {
        .done = init_react_cb_done_get,
    });
}

void
yeet_cb(struct discord *client, const struct discord_interaction *event)
{
    log_info("Yeet called!");
    struct instance_data *instance_data = discord_get_data(client);
    if (instance_data->yeets_cnt <= 0) {
        discord_create_interaction_response(client, event->id, event->token, &(struct discord_interaction_response) {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data) {
                .content = "Out of yeets!",
                .flags = DISCORD_MESSAGE_EPHEMERAL,
            },
        }, NULL);
        return;
    }
    mod_yeet_count(client, -1);

    // Build yeet
    struct yeet **null_yeet_addr = get_first_null(client);
    if (null_yeet_addr == NULL) my_die(client);
    struct yeet *free_yeet = malloc(sizeof(struct yeet));
    *null_yeet_addr = free_yeet;
    arena_init(&(free_yeet->arena), 16384);

    u64snowflake victim_id = strtoul(event->data->options->array[0].value, NULL, 10);

    // Format message
    char* yeet_message;
    cog_asprintf(&yeet_message, 
        "Do you want to yeet <@!%ld>? (%d %s's needed)\n"
        "Or, vote %s to yeet the author: <@!%ld>\n"
        "\n"
        "Otherwise, this will be deleted <t:%ld:R>\n"
    , victim_id, VOTE_COUNT, YES_EMOJI, NO_EMOJI, event->member->user->id, time(NULL) + YEET_VOTING_SECONDS);


    free_yeet->i_id = event->id;
    free_yeet->author = event->member->user->id;
    free_yeet->victim = victim_id;
    free_yeet->token = arena_calloc(free_yeet->arena, strlen(event->token) + 1);
    strcpy(free_yeet->token, event->token);

    struct discord_interaction_response params = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){
            .content = yeet_message,
        }
    };
    // This goes here for more accurate timekeeping. Of course, it's really only like, 5 clock cycles, but come on.
    free_yeet->timestamp = cog_timestamp_ms();
    discord_create_interaction_response(client, event->id,
                                            event->token, &params, &(struct discord_ret_interaction_response) {
                                            .keep = event,
                                            .done = init_react_cb_done,
                                            });
    free(yeet_message);
}


/**
 * This is the callback for when any message is reacted to. If the message reacted to has a corresponding yeet in the array,
 * it make a couple calls to the discord API to count reactions and then react accordingly.
 */
void 
on_react(struct discord *client, const struct discord_message_reaction_add *event) 
{
    if (event->member->user->id == BOT_ID) return;

    char *name = event->emoji->name;
    u64snowflake m_id = event->message_id;
    u64snowflake c_id = event->channel_id;
    
    // Check to make sure we're not doing yeet things on a non-yeet message
    struct yeet *yeet = get_yeet_by_id(client, m_id);
    if (yeet == NULL) return;
    
    log_info("EMOJI REACTED: %s", name);
    log_debug("TOKEN %s / MESSAGE_ID %ld", yeet->token, yeet->m_id.message);

    struct get_reactions_params *yes_ret_params = arena_alloc(yeet->arena, sizeof(struct get_reactions_params));
    struct get_reactions_params *no_ret_params = arena_alloc(yeet->arena, sizeof(struct get_reactions_params));
    yes_ret_params->yeet = yeet;
    no_ret_params->yeet = yeet;
    yes_ret_params->emoji = YES_EMOJI;
    no_ret_params->emoji = NO_EMOJI;

    discord_get_reactions(client, c_id, m_id, 0, YES_EMOJI, NULL, &(struct discord_ret_users) {
        .done = get_users_done,
        .keep = event,
        .data = yes_ret_params,
    });
    discord_get_reactions(client, c_id, m_id, 0, NO_EMOJI, NULL, &(struct discord_ret_users) {
        .done = get_users_done,
        .keep = event,
        .data = no_ret_params,
    });
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void 
replenish_yeet(struct discord *client, struct discord_timer *ev)
{
    log_debug("Replenishing yeet!");
    struct instance_data *instance_data = discord_get_data(client);
    if (instance_data->yeets_cnt < 3) mod_yeet_count(client, 1);
}
#endif