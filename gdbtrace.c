/* gdbtrace - signal/syscall tracer over the GDB remote protocol
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

#include <err.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "protocol.h"
#include "gdb/signals.h"

static const char *gdb_signal_names[] = {
#define SET(symbol, constant, name, string) \
    [constant] = name,
#include "gdb/signals.def"
#undef SET
  };

static void
print_stop_reason(uint8_t *reply, size_t size)
{
    if (size <= 3) return;

    char *savetok = NULL;
    for (char *info = strtok_r((char *) reply + 3, ";", &savetok);
            info; info = strtok_r(NULL, ";", &savetok)) {
        char *n = strtok(info, ":");
        char *r = strtok(NULL, "");
        if (n && r && !strncmp(n, "syscall_", 8))
            printf("%s %" PRIu64 "\n", n, gdb_decode_hex_str((uint8_t*)r));
    }
}

static uint16_t
print_signal(uint8_t *reply, size_t size)
{
    if (size < 3) return 0;

    uint16_t sig = gdb_decode_hex(reply[1], reply[2]);
    if (sig == GDB_SIGNAL_TRAP) {
        print_stop_reason(reply, size);
        return 0;
    } else if (sig < GDB_SIGNAL_LAST)
        printf("signal %2hu %s\n", sig, gdb_signal_names[sig]);
    else if (sig <= UINT8_MAX)
        printf("signal %2hu ???\n", sig);
    return sig;
}

static bool
print_exit_status(uint8_t sighigh, uint8_t siglow)
{
    uint16_t status = gdb_decode_hex(sighigh, siglow);
    if (status > UINT8_MAX)
        return false;
    printf("exited with %2hu\n", status);
    return true;
}

static void
catch_syscalls(struct gdb_conn *conn)
{
    // ref: https://sourceware.org/ml/gdb-patches/2013-09/msg00992.html
    static const char cmd[] = "QCatchSyscalls:1";
    gdb_send(conn, (const uint8_t *)cmd, sizeof(cmd) - 1);

    size_t size;
    uint8_t *reply = gdb_recv(conn, &size);
    bool ok = size == 2 && !strcmp((const char*)reply, "OK");
    free(reply);

    if (ok)
        puts("syscall tracing enabled");
    else
        puts("syscall tracing not supported");
}

int
main()
{
    // connect to gdbserver on localhost port 1234
    puts("Connecting to 127.0.0.1:1234...");
    struct gdb_conn *conn = gdb_begin_inet("127.0.0.1", 1234);

    gdb_start_noack(conn);
    catch_syscalls(conn);

    bool alive = true;
    gdb_send(conn, (const uint8_t *)"?", 1);
    do {
        size_t size;
        uint8_t *reply = gdb_recv(conn, &size);
        if (size == 0)
            errx(1, "empty reply!?!");

        bool ok = false;
        uint16_t sig = 0;
        switch (reply[0]) {
            case 'S': // signal stop
            case 'T': // extended signal stop
                if (size >= 3) {
                    sig = print_signal(reply, size);
                    ok = sig <= UINT8_MAX;
                }
                break;

            case 'X': // signal termination
                if (size >= 3) {
                    alive = false;
                    printf("exited with ");
                    sig = print_signal(reply, size);
                    ok = sig <= UINT8_MAX;
                }
                break;

            case 'W': // process exit
                if (size >= 3) {
                    ok = print_exit_status(reply[1], reply[2]);
                    alive = false;
                }
                break;

            case 'O': // console output
            case 'F': // host syscall
            default:
                break;
        }
        if (!ok)
            errx(1, "bad/unsupported stop reply: %.*s\n", (int)size, reply);

        free(reply);

        if (alive) {
          if (sig) {
              char cont[4] = "CXX";
              sprintf(cont, "C%02X", (uint8_t)sig);
              gdb_send(conn, (const uint8_t *)cont, 3);
          } else
              gdb_send(conn, (const uint8_t *)"c", 1);
        }
    } while (alive);

    // close the gdbserver connection
    gdb_end(conn);

    return 0;
}
