#include "discord.h"

#include "structs.c"


#ifndef YEET_ARRAY
#define YEET_ARRAY

/**
 * This method takes a message ID snowflake and returns the associated yeet in `client`'s yeets.
 * 
 * @param client the discord client with the yeet array
 * @return pointer to the matching yeet, or NULL if not found
 */
struct yeet *
get_yeet_by_id(struct discord *client, u64snowflake message_id)
{
    struct instance_data *i_data = (struct instance_data*)discord_get_data(client);
    struct yeet **yeet_arr = i_data->active_yeets;
    struct yeet *ret = NULL;
    for (int i = 0; i < i_data->active_yeets_size; i++) {
        if (*(yeet_arr + i) == NULL) continue;
        if ((*(yeet_arr + i))->m_id.message == message_id) {
            ret = *(yeet_arr + i);
            break;
        }
    }
    return ret;
}

/**
 * This method takes an interaction ID snowflake and returns the associated yeet in `client`'s yeets.
 * 
 * @param client the discord client with the yeet array
 * @return pointer to the matching yeet, or NULL if not found
 */
struct yeet *
get_yeet_by_token(struct discord *client, u64snowflake interaction_id) 
{
    struct instance_data *i_data = (struct instance_data*)discord_get_data(client);
    struct yeet **yeet_arr = i_data->active_yeets;
    struct yeet *ret = NULL;
    for (int i = 0; i < i_data->active_yeets_size; i++) {
        if (*(yeet_arr + i) == NULL) continue;
        if ((*(yeet_arr + i))->i_id == interaction_id) {
            ret = *(yeet_arr + i);
            break;
        }
    }
    return ret;
}

/**
 * This method finds the first null (empty) yeet in `client`'s yeets.
 * 
 * @param client the discord client with the yeet array
 * @return pointer to the pointer to the matching yeet, or NULL if not found
 */
struct yeet **
get_first_null(struct discord *client)
{
    struct instance_data *i_data = (struct instance_data*)discord_get_data(client);
    struct yeet **yeet_arr = i_data->active_yeets;
    for (int i = 0; i < i_data->active_yeets_size; i++) {
        if (*(yeet_arr + i) == NULL) return (yeet_arr + i);
    }
    return NULL; // God I hope we never reach this
}

/**
 * This method finds the associated yeet and sets it to null after freeing it.
 * 
 * @param client the discord client with the yeet array
 * @param target the yeet to look for
 */
void
nullifree_yeet(struct discord *client, struct yeet *target)
{
    struct instance_data *i_data = (struct instance_data*)discord_get_data(client);
    struct yeet **yeet_arr = i_data->active_yeets;
    for (int i = 0; i < i_data->active_yeets_size; i++) {
        if (*(yeet_arr + i) == target) {
            struct yeet *targ = *(yeet_arr + i);
            free(targ); // Separating the free to make the compiler shut up about use-after-free 
            *(yeet_arr + i) = NULL;
            return;
        }
    }
}

void
mod_yeet_count(struct discord *client, int term)
{
    struct instance_data *instance_data = discord_get_data(client);
    instance_data->yeets_cnt += term;
}
#endif