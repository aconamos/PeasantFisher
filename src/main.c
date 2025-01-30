#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "discord.h"
#include "log.h"

#include "project_vars.h"

#include "infra/structs.c"
#include "infra/arena.c"
#include "infra/die.c"
#include "infra/yeet_array.c"

#include "commands/yeet.c"
#include "commands/mod_abuse.c"


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

    struct discord_application_command_option yeet_options[] = {
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
                .size = sizeof(yeet_options) / sizeof *yeet_options,
                .array = yeet_options,
            },
    };
    discord_create_guild_application_command(client, event->application->id,
                                             GUILD_ID, &yeet_params, NULL);

    struct discord_application_command_option abuse_options[] = {
        {
            .type = DISCORD_APPLICATION_OPTION_USER,
            .name = "user",
            .description = "Who to mod abuse",
            .required = true
        },
        {
            .type = DISCORD_APPLICATION_OPTION_STRING,
            .name = "time",
            .description = "How long to mod abuse for (max is 28d)",
            .required = true
        },
        {
            .type = DISCORD_APPLICATION_OPTION_STRING,
            .name = "reason",
            .description = "Reason for mod abuse",
            .required = false
        }
    };

    struct discord_create_guild_application_command abuse_params = {
        .name = "mod_abuse",
        .description = "Shoot someone!",
        .options = 
            &(struct discord_application_command_options) {
                .size = sizeof(abuse_options) / sizeof *abuse_options,
                .array = abuse_options,
            },
    };
    discord_create_guild_application_command(client, event->application->id,
                                             GUILD_ID, &abuse_params, NULL);
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
        yeet_cb(client, event);
    }

    if (strcmp(event->data->name, "mod_abuse") == 0) {
        abuse_cb(client, event);
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

    // Set up a yeet array and add it to the client data
    struct yeet **client_yeets = calloc(ACTIVE_YEETS_SIZE, sizeof(struct yeet*));
    struct instance_data *instance_data = malloc(sizeof(struct instance_data));
    instance_data->active_yeets_size = ACTIVE_YEETS_SIZE;
    instance_data->active_yeets = client_yeets;
    instance_data->yeets_cnt = MAX_YEETS;
    discord_set_data(client, instance_data);

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