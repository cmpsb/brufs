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
#include <cerrno>
#include <cstring>
#include <ctime>

#include "TouchAction.hpp"
#include "Util.hpp"

std::vector<slopt_Option> Brufscli::TouchAction::get_options() const {
    return {
        {'a', nullptr, SLOPT_DISALLOW_ARGUMENT},
        {'c', "no-create", SLOPT_DISALLOW_ARGUMENT},
        {'d', "date", SLOPT_REQUIRE_ARGUMENT},
        {'f', nullptr, SLOPT_DISALLOW_ARGUMENT},
        {'h', "no-dereference", SLOPT_DISALLOW_ARGUMENT},
        {'m', nullptr, SLOPT_DISALLOW_ARGUMENT},
        {'r', "reference", SLOPT_REQUIRE_ARGUMENT},
        {'t', nullptr, SLOPT_REQUIRE_ARGUMENT},
        {'T', "time", SLOPT_REQUIRE_ARGUMENT}
    };
}

std::vector<std::string> Brufscli::TouchAction::get_names() const {
    return {"touch"};
}

void Brufscli::TouchAction::apply_option(
    int sw,
    int snam, [[maybe_unused]] const std::string &lnam,
    const std::string &val
) {
    if (sw == SLOPT_DIRECT && this->spec.empty()) {
        this->spec = val;
        return;
    }

    if (sw == SLOPT_DIRECT) {
        throw InvalidArgumentException(
            "Unexpected value " + val + " (target is " + this->spec + ")"
        );
    }

    switch (snam) {
    case 'a':
        this->access_ts = true;
        break;

    case 'c':
        this->create = false;
        break;

    case 'd':
        throw std::runtime_error("Date parsing is not implemented, use -t STAMP instead");

    case 'm':
        this->modification_ts = true;
        break;

    case 'r':
        this->reference_spec = val;
        break;

    case 't':
        this->timestamp = this->parse_timestamp(val);
        break;

    case 'T':
        if (val == "access" || val == "atime" || val == "use") {
            this->access_ts = true;
        } else if (val == "modify" || val == "mtime") {
            this->modification_ts = true;
        } else {
            this->logger.warn("Unknown --time WORD \"%s\"", val.c_str());
        }
        break;

    case 'h':
        this->logger.note("The -h option is ignored, until Brufs supports symlinks");
        this->create = false;
        break;

    case 'f':
        this->logger.note("The -f option is ignored");
        break;
    }
}

int Brufscli::TouchAction::run([[maybe_unused]] const std::string &name) {
    if (this->access_ts && !this->modification_ts) {
        // The user wants to update only the access time
        this->logger.warn("Brufs does not support access timestamps");
    }

    if (!this->access_ts && !this->modification_ts) {
        // No explicit -a or -m was passed
        this->modification_ts = true;
        this->access_ts = true;
    }

    auto path = this->path_parser.parse({this->spec.c_str(), this->spec.length()});
    this->path_validator.validate(path, true, true);

    auto brufs = this->opener.open_existing(path.get_partition());
    auto &fs = brufs.get_fs();
    const auto &io = brufs.get_io();

    const auto root_name = path.get_root();
    Brufs::RootHeader root_header;
    auto status = fs.find_root(root_name.c_str(), root_header);
    this->on_error(status, std::string("Unable to open root ") + root_name.c_str() + ": ", io);

    Brufs::Root root(fs, root_header);

    if (!this->reference_spec.empty()) {
        auto reference_path = this->path_parser.parse(
          {this->reference_spec.c_str(), this->reference_spec.length()}
        );

        Brufs::Inode reference(root);
        status = root.open_inode(reference_path, reference);
        this->on_error(status, "Unable to open the reference inode: ", io);

        this->timestamp = reference.get_header()->last_modified;
    }

    Brufs::File file(root);
    status = root.open_file(path, file);
    if (status == Brufs::Status::E_NOT_FOUND && this->create) {
        status = this->creator.create_file(path, this->inode_header_builder, file);
        this->on_error(status, "Unable to create the file: ", io);
    }
    this->on_error(status, "Unable to open the file to touch: ", io);

    if (this->modification_ts) {
        file.get_header()->last_modified = this->timestamp;
    }

    status = file.store();
    this->on_error(status, "Unable to modify the file: ", io);

    return 0;
}

Brufs::Timestamp Brufscli::TouchAction::parse_timestamp(const std::string &ts) const {
    struct tm broken_time;
    memset(&broken_time, 0, sizeof(struct tm));

    const auto end = strptime(ts.c_str(), "%Y%m%d%H%M", &broken_time);
    if (end == nullptr || (*end != '.' && *end != 0)) {
        throw InvalidArgumentException("Unable to parse " + ts + " as a timestamp");
    }

    Brufs::Timestamp stamp;
    stamp.seconds = mktime(&broken_time) + ((*end == '.') ? std::stoull(end + 1) : 0);
    stamp.nanoseconds = 0;

    return stamp;
}
