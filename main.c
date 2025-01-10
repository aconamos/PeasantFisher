#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "discord.h"
#include "log.h"

#define GUILD_ID 849505364764524565
#define BOT_ID 1326422681955991554
#define YES_EMOJI "✅"
#define NO_EMOJI "❌"
#define VOTE_COUNT 5
#define YEET_SECONDS 90

struct message_identifier {
    u64snowflake message;
    u64snowflake channel;
};

void
murder_message(struct discord *client, struct discord_timer *timer)
{
    struct message_identifier *data = (timer->data);
    discord_delete_message(client, data->channel, data->message, NULL, NULL);
    free(data);
}

void 
react_cb_done_get(struct discord *client, struct discord_response *resp, const struct discord_message *ret) 
{
    discord_create_reaction(client, ret->channel_id, ret->id, 0, YES_EMOJI, NULL);
    discord_create_reaction(client, ret->channel_id, ret->id, 0, NO_EMOJI, NULL);
    struct message_identifier *p_message_identifier = malloc(sizeof(struct message_identifier));
    p_message_identifier->channel = ret->channel_id;
    p_message_identifier->message = ret->id;
    discord_timer(client, murder_message, NULL, p_message_identifier, 5000);
}

void 
react_cb_done(struct discord *client, struct discord_response *resp, const struct discord_interaction_response *ret) 
{
    const struct discord_interaction *event = resp->keep;

    discord_get_original_interaction_response(client, BOT_ID, event->token, &(struct discord_ret_message) {
        .done = react_cb_done_get,
    });
}

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
                                                .done = react_cb_done,
                                             });
    }

    // -- DATA COMMANDS BELOW THIS LINE --
    /* Return in case user input is missing for some reason */
    if (!event->data || !event->data->options) return;

    if (strcmp(event->data->name, "yeet") == 0) {
        /*
        1. Push the message to chat
        2. Add the time of yeet, message id, and user id to the array of active yeets (coming soon^tm)
        3. Register a timer for x seconds out, it will run a function that either traverses the array of yeets and finds any expired or, if possible, just knows which one to do
        */
        log_info("Yeet called!");
        char* user_id = event->data->options->array[0].value;

        char* yeet_message;
        asprintf(&yeet_message, 
            "Do you want to yeet <@!%s>? (%d %s's needed)\n"
            "Or, vote %s to yeet the author: <@!%ld>\n"
            "\n"
            "Otherwise, this will be deleted <t:%ld:R>\n"
        , user_id, VOTE_COUNT, YES_EMOJI, NO_EMOJI, event->member->user->id, time(NULL) + YEET_SECONDS);
        
        struct discord_interaction_response params = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = yeet_message,
            }
        };
        discord_create_interaction_response(client, event->id,
                                              event->token, &params, &(struct discord_ret_interaction_response) {
                                                .keep = event,
                                                .done = react_cb_done,
                                             });
        free(yeet_message);
    }
}

void 
on_react(struct discord *client, const struct discord_message_reaction_add *event) 
{
    char *name = event->emoji->name;
    
    char *str;
    asprintf(&str, "EMOJI REACTED: %s", name);
    log_info(str);
    free(str);

    if (strcmp(name, YES_EMOJI) == 0) {
        log_debug("YES!");
    }
    else if (strcmp(name, NO_EMOJI) == 0) {
        log_debug("NO!");
    }
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

    // Run and wait for input?    
    discord_run(client);
    fgetc(stdin);

    discord_cleanup(client);
    ccord_global_cleanup();
}