/* gdbi - Interactive client for the GDB remote protocol
 * Copyright (C) 2015 Red Hat Inc.
 *
 * This file is part of gdb-toys.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "protocol.h"

static char *
get_command(bool interactive)
{
    if (interactive) {
        char *line = readline("(gdbi) ");
        if (line && *line)
            add_history(line);
        return line;
    } else {
        char *line = NULL;
        size_t line_size = 0;
        ssize_t size = getline(&line, &line_size, stdin);

        if (size < 0) {
            free(line);
            return NULL;
        }

        // strip the newline (to match readline)
        if (size > 0 && line[size - 1] == '\n')
            line[size - 1] = '\0';

        return line;
    }
}

int
main()
{
    bool interactive = isatty(STDIN_FILENO);

    if (interactive) {
        // don't tab-complete filenames
        rl_bind_key('\t', rl_insert);

        // introduction
        puts("gdbi - Interactive client for the GDB remote protocol");
        puts("See <https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html>");
        puts("Connecting to 127.0.0.1:1234...");
    }

    // connect to gdbserver on localhost port 1234
    struct gdb_conn *conn = gdb_begin_inet("127.0.0.1", 1234);

    // read raw commands from stdin
    char *command;
    bool extended = false;
    while ((command = get_command(interactive))) {
        char c = command[0];
        bool cmd_extended = c == '!';
        bool cmd_kill = c == 'k';
        bool cmd_restart = c == 'R';

        // the lower level needs to know about this specially
        if (!strcmp(command, "QStartNoAckMode")) {
            puts(gdb_start_noack(conn));
            continue;
        }

        // send the whole command as one packet
        gdb_send(conn, (const uint8_t *)command, strlen(command));
        free(command);

        // kill never has a reply
        // restart replies blank (unsupported) in basic mode, else no reply
        if (extended && (cmd_kill || cmd_restart))
            continue;
        else if (cmd_kill)
            break;

        // echo the reply on stdout
        size_t size;
        uint8_t *reply = gdb_recv(conn, &size);
        bool ok = size == 2 && !strcmp((const char*)reply, "OK");
        printf("%.*s\n", (int)size, reply);
        free(reply);

        if (cmd_extended && ok)
            extended = true;
    }

    // finish the line after the last prompt
    if (interactive)
        puts("");

    // close the gdbserver connection
    gdb_end(conn);

    return 0;
}
