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

#pragma once

#include "Logger.hpp"

#include "Action.hpp"
#include "BrufsOpener.hpp"
#include "PathValidator.hpp"

namespace Brufscli {

class AddRootAction : public Action {
private:
    Slog::Logger &logger;
    const BrufsOpener &opener;

    const Brufs::PathParser &path_parser;
    const PathValidator &path_validator;

    std::string spec;

    uint16_t inode_size = 128;
    int max_extent_length = 8;

    int mode = -1;

    int owner = -1;
    int group = -1;

    void assert_valid_inode_size() const;

public:
    /**
     * Creates a new add root action.
     *
     * @param logger the logger to write to
     * @param opener the service that opens the filesystem
     * @param parser the path parser to use
     * @param validator the path validator to use
     */
    AddRootAction(
        Slog::Logger &logger,
        const BrufsOpener &opener,
        const Brufs::PathParser &parser,
        const PathValidator &validator
    ) :
        logger(logger), opener(opener), path_parser(parser), path_validator(validator)
    {}

    std::vector<std::string> get_names() const override;
    std::vector<slopt_Option> get_options() const override;
    void apply_option(int sw, int snam, const std::string &lnam, const std::string &value) override;
    int run(const std::string &name) override;
};

}
