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

#include <stdexcept>
#include <string>
#include <vector>

#include "libbrufs.hpp"

#include "opt.h"

namespace Brufscli {

/**
 * An exception generated if the user passed an invalid argument.
 */
class InvalidArgumentException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * An action to be executed on the command line interface.
 */
class Action {
public:
    /**
     * Returns the names of the action.
     *
     * These names are used by the argument parser to identify the action to execute.
     *
     * This function has no default implementation.
     */
    virtual std::vector<std::string> get_names() const = 0;

    /**
     * Returns the command line options accepted by the action.
     *
     * The list must follow slopt conventions. It should not contain any entry colliding with
     * global options.
     *
     * By default the function returns an empty list.
     */
    virtual std::vector<slopt_Option> get_options() const {
        return {};
    }

    /**
     * Checks whether the passed option is valid.
     *
     * By default, this catches cases that slopt indicates as erroneous (based on the list of
     * options given by the action). This behavior can be overridden or extended by individual
     * actions.
     *
     * @param sw the kind of option passed
     * @param snam the short name of the option
     * @param lnam the long name of the option
     * @param value the value of the option
     */
    virtual void check_option(int sw, int snam, const std::string &lnam, const std::string &value);

    /**
     * Applies an option passed through the command line interface.
     *
     * This function should follow slopt conventions.
     *
     * Unlike pure C callbacks, this function may throw exceptions upon encountering an error.
     *
     * By default, this function ignores its parameters.
     *
     * @param sw the kind of option passed
     * @param snam the short name of the option
     * @param lnam the long name of the option
     * @param value the value of the option
     */
    virtual void apply_option(int sw, int snam, const std::string &lnam, const std::string &value) {
        (void) sw;
        (void) snam;

        throw InvalidArgumentException("Unexpected argument " + lnam + " " + value);
    }

    /**
     * Executes the action.
     *
     * Automatically parses the arguments passed to the action.
     *
     * This function cannot be overriden, you're looking for #run(const std::string &name).
     *
     * @param argc the number of arguments
     * @param argv the arguments
     *
     * @return zero on success, nonzero on error
     */
    int run(const int argc, char **argv);

    /**
     * Executes the action.
     *
     * This function has no default implementation.
     *
     * @param name the name this action is executed as (one of the names as returned in #get_names)
     * @param nargs the number of arguments passed by slopt
     *
     * @return zero on success, nonzero on error
     */
    virtual int run(const std::string &name) = 0;

    /**
     * Asserts that a Brufs status is not an error.
     *
     * @param on_error the string to include if an error occurs
     * @param io the I/O abstractor used to generate error messages
     * @param status the status to check
     */
    void on_error(
        Brufs::Status status, const std::string &on_error, const Brufs::AbstIO &io
    ) const;
};

}
