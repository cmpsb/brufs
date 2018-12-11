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

namespace Brufscli {

class InitAction : public Action {
private:
    Slog::Logger &logger;
    const BrufsOpener &opener;

    std::string dev_path;

    uint32_t cluster_size_exp = 14;

    uint8_t sc_low_mark = 12;
    uint8_t sc_high_mark = 24;

    void validate_cluster_size();
    void validate_spare_marks();

    uint8_t suggest_sc_low_mark();

public:
    /**
     * Constructs a new initialization action.
     *
     * @param logger the logger to write information to
     * @param opener the service to open the filesystem with
     */
    InitAction(Slog::Logger &logger, const BrufsOpener &opener) : logger(logger), opener(opener) {}

    std::vector<std::string> get_names() const override;
    std::vector<slopt_Option> get_options() const override;
    void apply_option(int sw, int snam, const std::string &lnam, const std::string &value) override;
    int run(const std::string &name) override;
};

}
