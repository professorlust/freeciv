/********************************************************************** 
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fcintl.h"

#include "connection.h"

#include "commands.h"
#include "voting.h"

/* Set the synopsis text to don't be translated. */
#define SYN_ORIG_(String) "*" String
/* Test if the synopsis should be translated or not. */
#define SYN_TRANS_(String) ('*' == String[0] ? String + 1 : _(String))

struct command {
  const char *name;       /* name - will be matched by unique prefix   */
  enum cmdlevel level;    /* access level required to use the command  */
  const char *synopsis;   /* one or few-line summary of usage */
  const char *short_help; /* one line (about 70 chars) description */
  const char *extra_help; /* extra help information; will be line-wrapped */
  enum cmd_echo echo;     /* Who will be notified when used. */
  int vote_flags;         /* how to handle votes */
  int vote_percent;       /* percent required, meaning depends on flags */
};

/* Commands must match the values in enum command_id. */
static struct command commands[] = {
  {"start",	ALLOW_BASIC,
   /* no translatable parameters */
   SYN_ORIG_("start"),
   N_("Start the game, or restart after loading a savegame."),
   N_("This command starts the game. When starting a new game, "
      "it should be used after all human players have connected, and "
      "AI players have been created (if required), and any desired "
      "changes to initial server options have been made. "
      "After 'start', each human player will be able to "
      "choose their nation, and then the game will begin. "
      "This command is also required after loading a savegame "
      "for the game to recommence. Once the game is running this command "
      "is no longer available, since it would have no effect."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },

  {"help",	ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("help\n"
      "help commands\n"
      "help options\n"
      "help <command-name>\n"
      "help <option-name>"),
   N_("Show help about server commands and server options."),
   N_("With no arguments gives some introductory help. "
      "With argument \"commands\" or \"options\" gives respectively "
      "a list of all commands or all options. "
      "Otherwise the argument is taken as a command name or option name, "
      "and help is given for that command or option. For options, the help "
      "information includes the current and default values for that option. "
      "The argument may be abbreviated where unambiguous."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },

  {"list",	ALLOW_INFO,
   /* no translatable parameters */
   SYN_ORIG_("list\n"
             "list colors\n"
             "list connections\n"
             "list ignored users\n"
             "list map image definitions\n"
             "list players\n"
             "list scenarios\n"
             "list teams\n"
             "list votes\n"),
   N_("Show a list of various things."),
   N_("Show a list of:\n"
      " - the player colors,\n"
      " - connections to the server,\n"
      " - your ignore list,\n"
      " - the list of defined map images,\n"
      " - the list of the players in the game,\n"
      " - the available scenarios,\n"
      " - the teams of players or\n"
      " - the running votes.\n"
      "The argument may be abbreviated, and defaults to 'players' if "
      "absent."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"quit",	ALLOW_HACK,
   /* no translatable parameters */
   SYN_ORIG_("quit"),
   N_("Quit the game and shutdown the server."), NULL,
   CMD_ECHO_ALL, VCF_NONE, 0
  },
  {"cut",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("cut <connection-name>"),
   N_("Cut a client's connection to server."),
   N_("Cut specified client's connection to the server, removing that client "
      "from the game. If the game has not yet started that client's player "
      "is removed from the game, otherwise there is no effect on the player. "
      "Note that this command now takes connection names, not player names."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"explain",	ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("explain\n"
      "explain <option-name>"),
   N_("Explain server options."),
   N_("The 'explain' command gives a subset of the functionality of 'help', "
      "and is included for backward compatibility. With no arguments it "
      "gives a list of options (like 'help options'), and with an argument "
      "it gives help for a particular option (like 'help <option-name>')."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"show",	ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("show\n"
      "show <option-name>\n"
      "show <option-prefix>\n"
      "show all\n"
      "show vital\n"
      "show situational\n"
      "show rare\n"
      "show changed\n"
      "show locked\n"
      "show rulesetdir"),
   N_("Show server options."),
   N_("With no arguments, shows vital server options (or available options, "
      "when used by clients). With an option name argument, show only the "
      "named option, or options with that prefix. With \"all\", it shows "
      "all options. With \"vital\", \"situational\" or \"rare\", a set of "
      "options with this level. With \"changed\", it shows only the options "
      "which have been modified, while with \"locked\" all settings locked "
      "by the ruleset will be listed. With \"ruleset\", it will show the "
      "current ruleset directory name."),
    CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"wall",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("wall <message>"),
   N_("Send message to all connections."),
   N_("For each connected client, pops up a window showing the message "
      "entered."),
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"connectmsg", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("connectmsg <message>"),
   N_("Set message to show to connecting players."),
   N_("Set message to send to clients when they connect.\n"
      "Empty message means that no message is sent."),
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"vote",	ALLOW_BASIC,
   /* TRANS: translate text between [] only; "vote" is as a process */
   N_("vote yes|no|abstain [vote number]"),
   /* TRANS: "vote" as an instance of voting */
   N_("Cast a vote."),
      /* xgettext:no-c-format */
   N_("A player with basic level access issuing a control level command "
      "starts a new vote for the command. The /vote command followed by "
      "\"yes\", \"no\", or \"abstain\", and optionally a vote number, "
      "gives your vote. If you do not add a vote number, your vote applies "
      "to the latest vote. You can only suggest one vote at a time. "
      "The vote will pass immediately if more than half of the voters "
      "who have not abstained vote for it, or fail immediately if at "
      "least half of the voters who have not abstained vote against it."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"debug",	ALLOW_CTRL,
   /* no translatable parameters */
   SYN_ORIG_("debug diplomacy\n"
             "debug ferries\n"
             "debug player <player>\n"
             "debug tech <player>\n"
             "debug city <x> <y>\n"
             "debug units <x> <y>\n"
             "debug unit <id>\n"
             "debug timing\n"
             "debug info"),
   N_("Turn on or off AI debugging of given entity."),
   N_("Print AI debug information about given entity and turn continous "
      "debugging output for this entity on or off."),
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"set",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("set <option-name> <value>"),
   N_("Set server option."), NULL,
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"team",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("team <player> <team>"),
   N_("Change a player's team affiliation."),
   N_("A team is a group of players that start out allied, with shared "
      "vision and embassies, and fight together to achieve team victory "
      "with averaged individual scores. Each player is always a member "
      "of a team (possibly the only member). This command changes which "
      "team a player is a member of. Use \"\" if names contain whitespace."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"rulesetdir", ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("rulesetdir <directory>"),
   N_("Choose new ruleset directory or modpack."),
   N_("Choose new ruleset directory or modpack."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"metamessage", ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("metainfo <meta-line>"),
   N_("Set metaserver info line."),
   N_("Set user defined metaserver info line. If parameter is omitted, "
      "previously set metamessage will be removed. For most of the time "
      "user defined metamessage will be used instead of automatically "
      "generated messages, if it is available."),
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"metapatches", ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("metapatch <meta-line>"),
   N_("Set metaserver patches line."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"metaconnection",	ALLOW_ADMIN,
   /* no translatable parameters */
   SYN_ORIG_("metaconnection u|up\n"
             "metaconnection d|down\n"
             "metaconnection ?"),
   N_("Control metaserver connection."),
   N_("'metaconnection ?' reports on the status of the connection to metaserver. "
      "'metaconnection down' or 'metac d' brings the metaserver connection down. "
      "'metaconnection up' or 'metac u' brings the metaserver connection up."),
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"metaserver",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("metaserver <address>"),
   N_("Set address (URL) for metaserver to report to."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"aitoggle",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("aitoggle <player-name>"),
   N_("Toggle AI status of player."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"take",    ALLOW_INFO,
   /* TRANS: translate text between [] and <> only */
   N_("take [connection-name] <player-name>"),
   N_("Take over a player's place in the game."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to take over a player. If you're not one of these, "
      "only the <player-name> argument is allowed. If '-' is given for the "
      "player name and the connection does not already control a player, one "
      "is created and assigned to the connection."),
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"observe",    ALLOW_INFO,
   /* TRANS: translate text between [] only */
   N_("observe [connection-name] [player-name]"),
   N_("Observe a player or the whole game."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to observe a player. If you're not one of these, "
      "only the [player-name] argument is allowed. If the console gives no "
      "player-name or the connection uses no arguments, then the connection "
      "is attached to a global observer."),
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"detach",    ALLOW_INFO,
   /* TRANS: translate text between <> only */
   N_("detach <connection-name>"),
   N_("Detach from a player."),
   N_("Only the console and connections with cmdlevel 'hack' can force "
      "other connections to detach from a player."),
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"create",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("create <player-name> [ai type]"),
   N_("Create an AI player with a given name."),
   N_("With the 'create' command a new player with the given name is "
      "created.\n"
      "If the game was started, the command checks for free player slots "
      "and, if no free slots are available, it tries to reuse the slots of "
      "dead players. The new player has no units or cities.\n"
      "AI type parameter can be used to select which AI module will be "
      "used for created player. This requires that freeciv has been "
      "compiled with AI module support and respective module has been "
      "loaded."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"away",	ALLOW_BASIC,
   /* no translatable parameters */
   SYN_ORIG_("away"),
   N_("Set yourself in away mode. The AI will watch your back."),
   N_("The AI will govern your nation but do minimal changes."),
   CMD_ECHO_NONE, VCF_NONE, 50
  },
  {"novice",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("novice\n"
      "novice <player-name>"),
   N_("Set one or all AI players to 'novice'."),
   N_("With no arguments, sets all AI players to skill level 'novice', and "
      "sets the default level for any new AI players to 'novice'. With an "
      "argument, sets the skill level for that player only."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"easy",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("easy\n"
      "easy <player-name>"),
   N_("Set one or all AI players to 'easy'."),
   N_("With no arguments, sets all AI players to skill level 'easy', and "
      "sets the default level for any new AI players to 'easy'. With an "
      "argument, sets the skill level for that player only."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"normal",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("normal\n"
      "normal <player-name>"),
   N_("Set one or all AI players to 'normal'."),
   N_("With no arguments, sets all AI players to skill level 'normal', and "
      "sets the default level for any new AI players to 'normal'. With an "
      "argument, sets the skill level for that player only."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"hard",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("hard\n"
      "hard <player-name>"),
   N_("Set one or all AI players to 'hard'."),
   N_("With no arguments, sets all AI players to skill level 'hard', and "
      "sets the default level for any new AI players to 'hard'. With an "
      "argument, sets the skill level for that player only."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"cheating",  ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("cheating\n"
      "cheating <player-name>"),
   N_("Set one or all AI players to 'cheating'."),
   N_("With no arguments, sets all AI players to skill level 'cheating', and "
      "sets the default level for any new AI players to 'cheating'. With an "
      "argument, sets the skill level for that player only."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"experimental",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("experimental\n"
      "experimental <player-name>"),
   N_("Set one or all AI players to 'experimental'."),
   N_("With no arguments, sets all AI players to skill 'experimental', and "
      "sets the default level for any new AI players to this. With an "
      "argument, sets the skill level for that player only. THIS IS ONLY "
      "FOR TESTING OF NEW AI FEATURES! For ordinary servers, this option "
      "has no effect."),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"cmdlevel",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("cmdlevel\n"
      "cmdlevel <level>\n"
      "cmdlevel <level> new\n"
      "cmdlevel <level> first\n"
      "cmdlevel <level> <connection-name>"),
   N_("Query or set command access level access."),
   N_("The command access level controls which server commands are available "
      "to users via the client chatline. The available levels are:\n"
      "    none  -  no commands\n"
      "    info  -  informational or observer commands only\n"
      "    basic -  commands available to players in the game\n"
      "    ctrl  -  commands that affect the game and users\n"
      "    admin -  commands that affect server operation\n"
      "    hack  -  *all* commands - dangerous!\n"
      "With no arguments, the current command access levels are reported. "
      "With a single argument, the level is set for all existing "
      "connections, and the default is set for future connections. "
      "If 'new' is specified, the level is set for newly connecting clients. "
      "If 'first come' is specified, the 'first come' level is set; it will be "
      "granted to the first client to connect, or if there are connections "
      "already, the first client to issue the 'first' command. "
      "If a connection name is specified, the level is set for that "
      "connection only.\n"
      "Command access levels do not persist if a client disconnects, "
      "because some untrusted person could reconnect with the same name. "
      "Note that this command now takes connection names, not player names."
      ),
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"first", ALLOW_BASIC,
   /* no translatable parameters */
   SYN_ORIG_("first"),
   N_("If there is none, become the game organizer with increased permissions."),
   NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"timeoutincrease", ALLOW_CTRL, 
   /* TRANS: translate text between <> only */
   N_("timeoutincrease <turn> <turninc> <value> <valuemult>"), 
   N_("See \"help timeoutincrease\"."),
   N_("Every <turn> turns, add <value> to timeout timer, then add <turninc> "
      "to <turn> and multiply <value> by <valuemult>. Use this command in "
      "concert with the option \"timeout\". Defaults are 0 0 0 1"),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"cancelvote", ALLOW_BASIC,
   /* TRANS: translate text between <> only; "vote" is as a process */
   N_("cancelvote\n"
      "cancelvote <vote number>\n"
      "cancelvote all\n"),
   /* TRANS: "vote" as a process */
   N_("Cancel a running vote.\n"),
   /* TRANS: "vote" as a process */
   N_("With no arguments this command removes your own vote. If you have "
      "an admin access level, you can cancel any vote by vote number, or "
      "all votes with the \'all\' argument."),
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"ignore", ALLOW_INFO,
   /* TRANS: translate text between <> and [] only */
   N_("ignore [type=]<pattern>"),
   N_("Block all messages from users matching the pattern."),
   N_("The given pattern will be added to your ignore list; you will not "
      "receive any messages from this users matching this pattern. The type "
      "may be either \"user\", \"host\", or \"ip\". The default type "
      "(if ommited) is to match against the username. The pattern supports "
      "unix glob style wildcards, i.e. * matches zero or more character, ? "
      "exactly one character, [abc] exactly one of 'a' 'b' or 'c', etc. "
      "To access your current ignore list, issue \"/list ignore\"."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"unignore", ALLOW_INFO,
   /* TRANS: translate text between <> */
   N_("unignore <range>"),
   N_("Remove ignore list entries."),
   N_("The ignore list entries in the given range will be removed; "
      "you will be able to receive messages from the respective users. "
      "The range argument may be a single number or a pair of numbers "
      "separated by a dash '-'. If the first number is ommitted, it is "
      "assumed to be 1, if the last is ommitted, it is assumed to be "
      "the last valid ignore list index. To access your current ignore "
      "list, issue \"/list ignore\"."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"playercolor", ALLOW_ADMIN,
   /* TRANS: translate text between <> */
   N_("playercolor <player-name> <color>\n"
      "playercolor <player-name> reset"),
   N_("Define the color of a player."),
   N_("This command is used to set the color of a player's nation. The "
      "color ist defined using a hexadecimal notation (HEX) for the "
      "combination of Red, Green, and Blue color values (RGB). The lowest "
      "value is 0 (in HEX: 00). The highest value is 255 (in HEX: FF). The "
      "color definition starts with a '#' sign followed be the HEX values "
      "for the three colors, i.e '#ff0000' for red. In server scripts, the "
      "'#' sign must be escaped or the color definition must be quoted.\n"
      "In initial game state the color can only be defined if the "
      "'plrcolormode' setting is set to 'PLR_SET'. The "
      "defined color can be removed using the reset argument.\n"
      "For a running game, this command redefines the player color. The "
      "change will be visible in the following turn.\n"
      "To list the player color use 'list colors'."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"endgame",	ALLOW_ADMIN,
   /* no translatable parameters */
   SYN_ORIG_("endgame"),
   N_("End the game immediately in a draw."), NULL,
   CMD_ECHO_ALL, VCF_NONE, 0
  },
  {"surrender",	ALLOW_BASIC,
   /* no translatable parameters */
   SYN_ORIG_("surrender"),
   N_("Concede the game."),
   N_("This tells everyone else that you concede the game, and if all "
      "but one player (or one team) have conceded the game in this way "
      "then the game ends."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"remove",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("remove <player-name>"),
   N_("Fully remove player from game."),
   N_("This *completely* removes a player from the game, including "
      "all cities and units etc. Use with care!"),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"save",	ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("save\n"
      "save <file-name>"),
   N_("Save game to file."),
   N_("Save the current game to file <file-name>. If no file-name "
      "argument is given saves to \"<auto-save name prefix><year>m.sav[.gz]\". "
      "To reload a savegame created by 'save', start the server with "
      "the command-line argument:\n"
      "    '--file <filename>' or '-f <filename>'\n"
      "and use the 'start' command once players have reconnected."),
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"load",      ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("load\n"
      "load <file-name>"),
   N_("Load game from file."),
   N_("Load a game from <file-name>. Any current data including players, "
      "rulesets and server options are lost."),
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"read",	ALLOW_CTRL,
   /* TRANS: translate text between <> only */
   N_("read <file-name>"),
   N_("Process server commands from file."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"write",	ALLOW_HACK,
   /* TRANS: translate text between <> only */
   N_("write <file-name>"),
   N_("Write current settings as server commands to file."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"reset",	ALLOW_CTRL,
   /* no translatable parameters */
   SYN_ORIG_("reset"),
   N_("Reset all server settings."),
   N_("Reset all settings if it is possible. The following levels are "
      "supported:\n"
      "  game     - using the values defined at the game start\n"
      "  ruleset  - using the values defined in the ruleset\n"
      "  script   - using default values and rereading the start script\n"
      "  default  - using default values\n"),
   CMD_ECHO_ALL, VCF_NONE, 50
  },
  {"lua", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("lua <script>"),
   N_("Evaluate a line of freeciv script in the current game."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"luafile", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("luafile <file>"),
   N_("Evaluate a freeciv script file in the current game."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"kick", ALLOW_CTRL,
   /* TRANS: translate text between <> */
    N_("kick <user>"),
    N_("Cut a connection and disallow reconnect."),
    N_("The connection given by the 'user' argument will be cut from the "
       "server and not allowed to reconnect. The time the user wouldn't be "
       "able to reconnect is controlled by the 'kicktime' setting."),
   CMD_ECHO_ADMINS, VCF_NOPASSALONE, 50
  },
#ifdef DEBUG
  {"oldsave", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("save\n"
      "save <file-name>"),
   N_("Save game to file using the old format."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
#endif /* DEBUG */
  {"delegate", ALLOW_BASIC,
   /* TRANS: translate only text between [] and <> */
   N_("delegate cancel [player-name]\n"
      "delegate restore\n"
      "delegate show <player-name>\n"
      "delegate take <player-name>\n"
      "delegate to <username> [player-name]"),
   N_("Delegate control to another user."),
   N_("This command can be used to delegate the control over a player. The "
      "[player-name] argument can only be used by connections with the "
      "cmdlevel 'admin' or above to force the corresponding change of the "
      "delegation status."),
   CMD_ECHO_NONE, VCF_NONE, 0
  },
  {"fcdb", ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("fcdb [reload]\n"
      "fcdb [lua] <script>"),
   N_("Commands related to the freeciv database support. The argument "
      "[reload] allows a reset of the script file after a change while the "
      "argument [lua] allows to evaluate a line of lua script in the contex "
      "of the lua instance for the freeciv database."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"mapimg",   ALLOW_ADMIN,
   /* TRANS: translate text between <> only */
   N_("mapimg colortest\n"
      "mapimg create <id>|all\n"
      "mapimg define <mapdef>\n"
      "mapimg delete <id>|all\n"
      "mapimg show <id>|all\n"
      "mapimg help"),
   N_("Create image files of the world/player map."),
   N_("This command allows the creation of map images. Supported "
      "arguments:\n"
      "  colortest        - create a image to showing all colors\n"
      "  create <id>|all  - create a specific or all map images\n"
      "  define <mapdef>  - define a map image\n"
      "  delete <id>|all  - delete a specific or all map images\n"
      "  help             - more information about the definition\n"
      "  show <id>|all    - shown a specific or all map images"),
   CMD_ECHO_ADMINS, VCF_NONE, 50
  },
  {"rfcstyle",	ALLOW_HACK,
   /* no translatable parameters */
   SYN_ORIG_("rfcstyle"),
   N_("Switch server output between 'RFC-style' and normal style."), NULL,
   CMD_ECHO_ADMINS, VCF_NONE, 0
  },
  {"serverid",	ALLOW_INFO,
   /* no translatable parameters */
   SYN_ORIG_("serverid"),
   N_("Simply returns the id of the server."),
   CMD_ECHO_NONE, VCF_NONE, 0
  }
};


/**************************************************************************
  ...
**************************************************************************/
const struct command *command_by_number(int i)
{
  fc_assert_ret_val(i >= 0 && i < CMD_NUM, NULL);
  return &commands[i];
}

/**************************************************************************
  ...
**************************************************************************/
const char *command_name(const struct command *pcommand)
{
  return pcommand->name;
}

/**************************************************************************
  ...
**************************************************************************/
const char *command_name_by_number(int i)
{
  return command_by_number(i)->name;
}

/**************************************************************************
  Returns the synopsis text of the command (translated).
**************************************************************************/
const char *command_synopsis(const struct command *pcommand)
{
  return SYN_TRANS_(pcommand->synopsis);
}

/**************************************************************************
  Returns the short help text of the command (translated).
**************************************************************************/
const char *command_short_help(const struct command *pcommand)
{
  return _(pcommand->short_help);
}

/**************************************************************************
  Returns the extra help text of the command (translated).
**************************************************************************/
const char *command_extra_help(const struct command *pcommand)
{
  return _(pcommand->extra_help);
}

/**************************************************************************
  ...
**************************************************************************/
enum cmdlevel command_level(const struct command *pcommand)
{
  return pcommand->level;
}

/****************************************************************************
  Retrurns the flag of the command to notify the users about its usage.
****************************************************************************/
enum cmd_echo command_echo(const struct command *pcommand)
{
  return pcommand->echo;
}

/**************************************************************************
  Returns a bit-wise combination of all vote flags set for this command.
**************************************************************************/
int command_vote_flags(const struct command *pcommand)
{
  return pcommand ? pcommand->vote_flags : 0;
}

/**************************************************************************
  Returns the vote percent required for this command to pass in a vote.
**************************************************************************/
int command_vote_percent(const struct command *pcommand)
{
  return pcommand ? pcommand->vote_percent : 0;
}
