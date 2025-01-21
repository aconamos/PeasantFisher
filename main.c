#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "discord.h"
#include "log.h"

#include "structs.c"
#include "die.c"
#include "arena.c"


#define YES_EMOJI "✅"
#define NO_EMOJI "❌"
#define ACTIVE_YEETS_SIZE 20
#define BOT_ID 1326422681955991554

#ifdef DEBUG
    // Debug-specific variables make the bot much faster to debug
    #define GUILD_ID 849505364764524565
    #define VOTE_COUNT 2
    #define YEET_VOTING_SECONDS 20
    #define YEET_DURATION 30
    #define MAX_YEETS 2
    #define YEET_REGEN_S 20
#else
    // Production variables go here
    #define GUILD_ID 849505364764524565
    #define VOTE_COUNT 3
    #define YEET_VOTING_SECONDS 90
    #define YEET_DURATION 300
    #define MAX_YEETS 3
    #define YEET_REGEN_S 3600
#endif


struct yeet *active_yeets[ACTIVE_YEETS_SIZE]; // Just make an array that... should be big enough. If it crashes... oh well.
int remaining_yeet_cnt = MAX_YEETS;

void my_die(void);

/**
 * This method takes a message ID snowflake and returns the index of the active_yeets array that corresponds to the snowflake,
 * if there are any. Returns -1 otherwise.
 */
int
get_yeet_idx_by_id(u64snowflake message_id)
{
    int ret = -1;
    for (int i = 0; i < ACTIVE_YEETS_SIZE; i++) {
        if (active_yeets[i] == NULL) continue;
        if (active_yeets[i]->m_id.message == message_id) {
            ret = i;
            break;
        }
    }
    return ret;
}

/**
 * This method takes an interaction ID snowflake and returns the index of the active_yeets array that corresponds to the snowflake,
 * if there are any. Returns -1 otherwise.
 */
int
get_yeet_idx_by_token(u64snowflake interaction_id) 
{
    int ret = -1;
    for (int i = 0; i < ACTIVE_YEETS_SIZE; i++) {
        if (active_yeets[i] == NULL) continue;
        if (active_yeets[i]->i_id == interaction_id) {
            ret = i;
            break;
        }
    }
    return ret;
}

/**
 * This method returns the index of the first null pointer in the active_yeets array.
 */
int
get_first_null(void)
{
    for (int i = 0; i < ACTIVE_YEETS_SIZE; i++) {
        if (active_yeets[i] == NULL) return i;
    }
    return -1; // God I hope we never reach this
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
}

void 
yeet_succ(struct discord *client, struct discord_response *resp, const struct discord_guild_member *ret)
{
    log_debug("yeet_succ called!");
    struct yeet_with_users *yww_data = resp->data;
    struct yeet *yeet = yww_data->yeet;

    u64unix_ms deltaTimeMs = (cog_timestamp_ms() - yeet->timestamp);
    char *yeet_msg = arena_calloc(yeet->arena, 2000); // TODO get an exact size
    cog_asprintf(&yeet_msg, "User <@!%ld> yeeted in %.2f seconds!\nBrought to you by %s", yeet->victim, (float)deltaTimeMs/1000, yww_data->users_msg);

    discord_delete_message(client, yeet->m_id.channel, yeet->m_id.message, NULL, NULL);
    discord_create_message(client, yeet->m_id.channel, &(struct discord_create_message) {
        .content = yeet_msg,
    }, NULL); // TODO: They will return in + callback
    // TODO: Edit to make it say "they have since returned" or whatever (use a callback in case of failure)
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
    // TODO: Only make this call if the yeet hasn't passed
    discord_delete_message(client, data->channel, data->message, NULL, NULL);

    // At this point, the yeet could either still be pending decision or decided. 
    // If decided, the pointer will be null and removed from the active_yeets array.
    // We will use the message id to search and if the search doesn't return -1,
    // we free the pointer there and set it to NULL.
    int idx = get_yeet_idx_by_id(data->message);
    if (idx != -1) {
        log_debug("murder_message: freeing arena @ %p\n\t\tfreeing yeet @ %p", active_yeets[idx]->arena, active_yeets[idx]);
        arena_free(active_yeets[idx]->arena);
        free(active_yeets[idx]);
        active_yeets[idx] = NULL;
        remaining_yeet_cnt++;
    }
}

/**
 * This callback serves to finish constructing yeet data and start the timer to delete it.
 * It is the last call to construct the yeet following the initial reaction from the bot.
 */
void 
init_react_cb_done_get(struct discord *client, struct discord_response *resp, const struct discord_message *ret) 
{
    discord_create_reaction(client, ret->channel_id, ret->id, 0, YES_EMOJI, NULL);
    discord_create_reaction(client, ret->channel_id, ret->id, 0, NO_EMOJI, NULL);

    log_debug("Interaction ID (CB): %ld", ret->interaction->id);
    int idx = get_yeet_idx_by_token(ret->interaction->id);
    struct yeet *yeet = active_yeets[idx];
    yeet->m_id.channel = ret->channel_id;
    yeet->m_id.message = ret->id;

    struct message_identifier *p_message_identifier = arena_alloc(yeet->arena, sizeof(struct message_identifier));
    p_message_identifier->channel = ret->channel_id;
    p_message_identifier->message = ret->id;

    discord_timer(client, murder_message, NULL, p_message_identifier, YEET_VOTING_SECONDS * 1000);
}

/**
 * This callback serves to call the other, more useful callback.
 * I'm not sure why it's required; I copied it from somewhere.
 */
void 
init_react_cb_done(struct discord *client, struct discord_response *resp, const struct discord_interaction_response *ret) 
{
    const struct discord_interaction *event = resp->keep;

    discord_get_original_interaction_response(client, BOT_ID, event->token, &(struct discord_ret_message) {
        .done = init_react_cb_done_get,
    });
}

/**
 * This serves to build the application commands necessary for the bot to function.
 */
void 
on_ready(struct discord *client, const struct discord_ready *event) 
{
    // This ping command is probably going to stay a while for ensuring slash commands work.
    struct discord_create_guild_application_command params = {
        .name = "ping",
        .description = "Ping command!"
    };
    discord_create_guild_application_command(client, event->application->id,
                                             GUILD_ID, &params, NULL);

    struct discord_application_command_option options[] = {
        {
            .type = DISCORD_APPLICATION_OPTION_USER,
            .name = "user",
            .description = "Who to yeet",
            .required = true
        },
    };

    struct discord_create_guild_application_command yeet_params = {
        .name = "yeet",
        .description = "Yeet someone!",
        .options = 
            &(struct discord_application_command_options) {
                .size = sizeof(options) / sizeof *options,
                .array = options,
            },
    };
    discord_create_guild_application_command(client, event->application->id,
                                             GUILD_ID, &yeet_params, NULL);
}

void 
on_interaction(struct discord *client, const struct discord_interaction *event) 
{
    if (event->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
        return; /* return if interaction isn't a slash command */

    if (strcmp(event->data->name, "ping") == 0) {
          struct discord_interaction_response params = {
                .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
                .data = &(struct discord_interaction_callback_data){
                      .content = "pong"
                }
          };
          discord_create_interaction_response(client, event->id,
                                              event->token, &params, &(struct discord_ret_interaction_response) {
                                                .keep = event,
                                                .done = init_react_cb_done,
                                             });
    }

    // -- DATA COMMANDS BELOW THIS LINE --
    /* Return in case user input is missing for some reason */
    if (!event->data || !event->data->options) return;

    if (strcmp(event->data->name, "yeet") == 0) {
        log_info("Yeet called!");
        if (remaining_yeet_cnt <= 0) {
            discord_create_interaction_response(client, event->id, event->token, &(struct discord_interaction_response) {
                .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
                .data = &(struct discord_interaction_callback_data) {
                    .content = "Out of yeets!",
                    .flags = DISCORD_MESSAGE_EPHEMERAL,
                },
            }, NULL);
            return;
        }
        remaining_yeet_cnt--;

        // Build yeet
        int free_yeet_idx = get_first_null();
        if (free_yeet_idx == -1) my_die();
        struct yeet *free_yeet = malloc(sizeof(struct yeet));
        active_yeets[free_yeet_idx] = free_yeet;
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
        free_yeet->token = arena_alloc(free_yeet->arena, strlen(event->token) + 1);
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
    int idx = get_yeet_idx_by_id(m_id);
    if (idx == -1) return;

    struct yeet *yeet = active_yeets[idx];
    
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

void 
replenish_yeet(struct discord *client, struct discord_timer *ev)
{
    log_debug("Replenishing yeet!");
    if (remaining_yeet_cnt < 3) remaining_yeet_cnt++;
}

int main(int argc, char* argv[]) {
    // Load config file
    const char *config_file;
    if (argc > 1)
        config_file = argv[1];
    else
        config_file = "./config.json";
    
    // Initialize the client using some magic function calls
    ccord_global_init();
    struct discord *client = discord_config_init(config_file);
    assert(NULL != client && "Couldn't initialize client");

    // Register events (I actually love the function reference style C uses)
    discord_set_on_ready(client, &on_ready);
    discord_set_on_interaction_create(client, &on_interaction);
    discord_set_on_message_reaction_add(client, &on_react);

    // Make the max yeets replenishment timer
    discord_timer_interval(
        client,
        replenish_yeet,
        NULL,
        NULL,
        1000,
        YEET_REGEN_S * 1000,
        -1
    );

    // Run and wait for input?    
    discord_run(client);
    fgetc(stdin);

    discord_cleanup(client);
    ccord_global_cleanup();
}

void
my_die()
{
    dumps_yeet_arr(active_yeets);
    die("CRITICAL: active_yeets array must be full! No null pointers found, can't initialize a new yeet!");
}