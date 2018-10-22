/*
 * Copyright (c) 2017-2018 Luc Everse <luc@cmpsb.net>
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

#include <cstdio>
#include <string>

int init(int, char **);
int check(int, char **);
int help(int, char **);
int add_root(int, char **);
int ls(int, char **);
int mkdir(int, char **);

static void print_usage(const char *pname) {
    fprintf(stderr, 
        "USAGE: %s ACTION ARGUMENTS...\n"
        "Actions:\n"
        "init . . . : format a disk\n"
        "check  . . : print diagnostic information\n"
        "help . . . : display help for an action\n",
        pname
    );
}

int main(int argc, char **argv) {
    if (argc == 1) {
        fprintf(stderr, "Insufficient number of arguments\n");
        print_usage(argv[0]);

        return 1;
    }

    std::string action = argv[1];

    if (action == "init") {
        return init(argc - 1, argv + 1);
    } else if (action == "check") {
        return check(argc - 1, argv + 1);
    } else if (action == "add-root") {
        return add_root(argc - 1, argv + 1);
    } else if (action == "ls") {
        return ls(argc - 1, argv + 1);
    } else if (action == "mkdir") {
        return mkdir(argc - 1, argv + 1);
    // } else if (action == "help") {
    //     return help(argc - 1, argv + 1);
    }

    fprintf(stderr, "Unknown action %s\n", argv[1]);
    print_usage(argv[0]);

    return 1;
}
