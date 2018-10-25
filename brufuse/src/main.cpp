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

#include <string>
#include <vector>

#include "slopt/opt.h"

static slopt_Option options[] = {
    {'s', "socket", SLOPT_REQUIRE_ARGUMENT},
    {'o', "option", SLOPT_REQUIRE_ARGUMENT},
    {'d', "daemon", SLOPT_ALLOW_ARGUMENT},
    {0, nullptr, SLOPT_DISALLOW_ARGUMENT}
};

static std::string socket_path;
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

int main(int argc, char **argv) {
    int nargs = slopt_parse(argc - 1, argv + 1, options, apply_option, nullptr);
    (void) nargs;
}
