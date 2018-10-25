/*
 * Copyright (c) 2017-2018 Luc Everse <luc@wukl.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include "slopt/opt.h"
#include "xxhash/xxhash.h"
#include "libbrufs.hpp"

static slopt_Option options[] = {
    {'s', "socket", SLOPT_REQUIRE_ARGUMENT},
    {'v', "visibility", SLOPT_REQUIRE_ARGUMENT},
    {'m', "mode", SLOPT_REQUIRE_ARGUMENT},
    {'o', "option", SLOPT_REQUIRE_ARGUMENT},
    {'d', "daemon", SLOPT_ALLOW_ARGUMENT},
    {0, nullptr, SLOPT_DISALLOW_ARGUMENT}
};

static std::string socket_path;
static std::string socket_visibility;
static unsigned int socket_mode;
static bool socket_mode_override = false;
static std::string dev_path;
static std::string mount_point;
static std::vector<const char *> fuse_args;
static bool daemonize = true;

static void apply_option(int sw, char snam, const char *lnam, const char *val, void *pl) {
    (void) pl;

    std::string sval;
    if (val) sval = val;

    if (SLOPT_IS_OPT(sw)) {
        switch (snam) {
        case 's':
            socket_path = val;
            return;
        case 'v':
            socket_visibility = val;
            if (socket_visibility != "public" && socket_visibility != "private") {
                fprintf(stderr, "Unknown visibility mode %s\n", val);
                exit(1);
            }
            return;
        case 'm':
            if (sscanf(val, "%o", &socket_mode) != 1) {
                fprintf(stderr, "Unable to parse socket mode %s into octal\n", val);
                exit(1);
            }
            socket_mode_override = true;
            return;
        case 'o':
            fuse_args.push_back(val);
            return;
        case 'd':
            if (sval == "n" || sval == "no" || sval == "0" || sval == "false") daemonize = false;
            return;
        }
    }

    if (sw == SLOPT_DIRECT && dev_path.empty()) {
        dev_path = val;
        return;
    }

    if (sw == SLOPT_DIRECT && mount_point.empty()) {
        mount_point = val;
        return;
    }

    if (sw == SLOPT_DIRECT) {
        fprintf(stderr, "Don't know what to do with %s (device is %s, mount point is %s)\n",
            val, dev_path.c_str(), mount_point.c_str()
        );
        exit(1);
    }

    if (sw == SLOPT_UNKNOWN_SHORT_OPTION) {
        fprintf(stderr, "Unknown switch -%c\n", snam);
        exit(1);
    }

    if (sw == SLOPT_UNKNOWN_LONG_OPTION) {
        fprintf(stderr, "Unknown option --%s\n", lnam);
        exit(1);
    }

    if (sw == SLOPT_MISSING_ARGUMENT) {
        fprintf(stderr, "--%s requires an argument\n", lnam);
        exit(1);
    }

    if (sw == SLOPT_UNEXPECTED_ARGUMENT) {
        fprintf(stderr, "--%s does not allow an argument\n", lnam);
        exit(1);
    }
}

static void set_socket_visibility() {
    if (socket_visibility.empty()) {
        socket_visibility = geteuid() == 0 ? "public" : "private";
    }
}

static void set_socket_mode() {
    if (socket_mode_override) return;

    if (socket_visibility == "public") socket_mode = 0666;
    else socket_mode = 0600;
}

static std::string hash_dev_path() {
    const auto hash = XXH64(dev_path.c_str(), dev_path.length(), Brufs::HASH_SEED);

    constexpr unsigned int BUF_SIZE = 128;
    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, "%hX-%hX-%hX-%hX", 
        (uint16_t) (hash >> 48),
        (uint16_t) (hash >> 32),
        (uint16_t) (hash >> 16),
        (uint16_t) (hash >>  0)
    );

    return {buf};
}

static std::string build_socket_path() {

    if (socket_visibility == "public") {
        mkdir("/run/brufuse", socket_mode);
        return "/run/brufuse/" + hash_dev_path() + ".sock";
    } else {
        std::string run_dir;

        const char *run_dir_raw = getenv("XDG_RUNTIME_DIR");
        if (!run_dir_raw) {
            std::string in_tmp_dir = "/tmp/" + std::to_string(geteuid());
            mkdir(in_tmp_dir.c_str(), socket_mode);
            run_dir = in_tmp_dir + "/brufuse";
        } else {
            run_dir = std::string(run_dir_raw) + "/brufuse";
        }

        mkdir(run_dir.c_str(), socket_mode);
        return run_dir + "/" + hash_dev_path() + ".sock";
    }
}

int main(int argc, char **argv) {
    slopt_parse(argc - 1, argv + 1, options, apply_option, nullptr);

    set_socket_visibility();
    set_socket_mode();
    
    if (socket_path.empty()) socket_path = build_socket_path();
    printf("Socket will be %s at %s\n", socket_visibility.c_str(), socket_path.c_str());
}
