# PeasantFisher
Ever thought:
> "I really like [coravacav's KingFisher](https://github.com/coravacav/uofu-cs-discord-bot) bot in my discord, but it's just too memory-safe."
>
> "Oh, and I only want one feature from the bot."

Me too!
Introducing **PeasantFisher**, a ripoff that implements KingFisher's
yeet functionality in C99 using [cogmasters/concord](https://github.com/cogmasters/concord).

It has a grand total of two commands:
1. `/ping` - Pong! (This should probably be removed, but I like it.)
2. `/yeet <user>` - Start a vote to timeout the given user.

## Installation
You need the following prerequisites:
- `curl`: `libcurl4-openssl-dev` on Debian
- `jsmn`: `libjsmn-dev` on Debian

### Configuration
#### Discord bot
Discord bot creation won't be covered here; there are plenty of tutorials online. You need it to have permissions to send messages and manage users.

The token goes in `config.json`.

#### Functionality
Assuming you already have a bot application invited to your server with the send messages and manage users permissions (that setup won't be covered here),
Edit `main.c` to adjust the following preprocessor macros:
- `GUILD_ID`: the guild this bot should function in
- `BOT_ID`: the user ID of the bot
- `VOTE_COUNT`: the amount of votes for a yeet to pass
- `YEET_VOTING_SECONDS`: number of seconds to allow voting
- `YEET_DURATION`: the duration to timeout victims
- `MAX_YEETS`: by default, there is a limited number of yeets over time; this controls that limit
- `YEET_REGEN_S`: how many seconds to give one yeet back



### Build
Run `make` 🙂

> Note on Makefile: This assumes you haven't cloned the submodules already. If you have, you *might* have to `rm -rf concord && mkdir concord`.

Binaries are found in `bin`.
- `release`: -O2 optimized version
- `debug`: -O0 with debug symbols
- `asan`: `debug` but with address sanitizer

## Contributing
Why?
