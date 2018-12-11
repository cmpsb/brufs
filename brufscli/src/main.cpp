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

#include <cstdio>

#include <map>
#include <random>
#include <string>

#include "Logger.hpp"

#include "InodeIdGenerator.hpp"
#include "AddRootAction.hpp"
#include "CheckAction.hpp"
#include "CopyInAction.hpp"
#include "CopyOutAction.hpp"
#include "InitAction.hpp"
#include "LsAction.hpp"
#include "MkdirAction.hpp"
#include "TouchAction.hpp"
#include "VersionAction.hpp"

using namespace Brufscli;

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
    Slog::Logger logger("brufs", Slog::Level::TRACE);
    const Brufs::PathParser path_parser;
    const Brufscli::PathValidator path_validator;
    const Brufscli::BrufsOpener brufs_opener;
    const Brufscli::InodeIdGenerator inode_id_generator;
    const Brufs::EntityCreator entity_creator(inode_id_generator);

    std::vector<std::shared_ptr<Brufscli::Action>> actions = {
        std::make_shared<AddRootAction>(logger, brufs_opener, path_parser, path_validator),
        std::make_shared<CheckAction>(logger, brufs_opener, path_parser, path_validator),
        std::make_shared<CopyInAction>(
            logger, brufs_opener, entity_creator, path_parser, path_validator
        ),
        std::make_shared<CopyOutAction>(logger, brufs_opener, path_parser, path_validator),
        std::make_shared<InitAction>(logger, brufs_opener),
        std::make_shared<LsAction>(logger, brufs_opener, path_parser, path_validator),
        std::make_shared<MkdirAction>(
            logger, brufs_opener, entity_creator, path_parser, path_validator
        ),
        std::make_shared<TouchAction>(
            logger, brufs_opener, entity_creator, path_parser, path_validator
        ),
        std::make_shared<VersionAction>(logger)
    };

    std::map<std::string, std::shared_ptr<Brufscli::Action>> actions_by_name;

    for (const auto &action : actions) {
        for (const auto &name : action->get_names()) {
            actions_by_name[name] = action;
        }
    }

    if (argc == 1) {
        logger.error("Insufficient number of arguments");
        print_usage(argv[0]);

        return 1;
    }

    const std::string action = argv[1];
    if (actions_by_name.find(action) == actions_by_name.end()) {
        logger.error("Unknown action %s", action.c_str());
        print_usage(argv[0]);

        return 1;
    }

    return actions_by_name[action]->run(argc - 1, argv + 1);
}
