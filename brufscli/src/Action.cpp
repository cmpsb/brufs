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

#include "Action.hpp"

class BrufsException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

static void apply_option(int sw, char snam, const char *lnam, const char *val, void *pl) {
    auto action = reinterpret_cast<Brufscli::Action *>(pl);

    action->check_option(sw, snam, lnam, val);
    action->apply_option(sw, snam, lnam, val);
}

int Brufscli::Action::run(const int argc, char **argv) {
    auto options = this->get_options();
    options.push_back({0, 0, 0});

    slopt_parse(argc - 1, argv + 1, options.data(), ::apply_option, this);
    return this->run(argv[0]);
}

void Brufscli::Action::check_option(
    int sw, int snam, const std::string &lnam, [[maybe_unused]] const std::string &val
) {
    switch (sw) {
    case SLOPT_MISSING_ARGUMENT:
        throw InvalidArgumentException("Option --" + lnam + " requires an argument");

    case SLOPT_UNEXPECTED_ARGUMENT:
        throw InvalidArgumentException("Option --" + lnam + " accepts no argument");

    case SLOPT_UNKNOWN_SHORT_OPTION:
        throw InvalidArgumentException("Unknown option -" + std::to_string(snam));

    case SLOPT_UNKNOWN_LONG_OPTION:
        throw InvalidArgumentException("Unknown option --" + lnam);
    }
}

void Brufscli::Action::on_error(
    Brufs::Status status, const std::string &on_error, const Brufs::AbstIO &io
) const {
    if (status < Brufs::Status::OK) {
        throw BrufsException(on_error + io.strstatus(status));
    }
}
