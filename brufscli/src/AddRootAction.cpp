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
#include <sys/types.h>
#include <sys/stat.h>

#include "AddRootAction.hpp"

namespace Brufscli {

class UnsupportedInodeSizeException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

}

std::vector<std::string> Brufscli::AddRootAction::get_names() const {
    return {"add-root", "addroot"};
}

std::vector<slopt_Option> Brufscli::AddRootAction::get_options() const {
    return {
        {'i', "inode-size", SLOPT_REQUIRE_ARGUMENT},
        {'e', "max-extent-length", SLOPT_REQUIRE_ARGUMENT},
        {'m', "mode", SLOPT_REQUIRE_ARGUMENT},
        {'u', "owner", SLOPT_REQUIRE_ARGUMENT},
        {'g', "group", SLOPT_REQUIRE_ARGUMENT}
    };
}

void Brufscli::AddRootAction::apply_option(
    int sw, int snam, [[maybe_unused]] const std::string &lnam, const std::string &val
) {
    if (sw == SLOPT_DIRECT && this->spec.empty()) {
        this->spec = val;
        return;
    }

    if (sw == SLOPT_DIRECT) {
        throw InvalidArgumentException(
            "Unexpected value " + val + " (path is " + this->spec + ")"
        );
    }

    switch (snam) {
    case 'i': {
        auto lval = std::stoul(val);

        this->inode_size = static_cast<uint16_t>(lval);
        if (this->inode_size != lval) {
            throw InvalidArgumentException("Invalid inode size " + val);
        }

        this->assert_valid_inode_size();

        break;
    }

    case 'e':
        this->max_extent_length = std::stoi(val);
        break;

    case 'm':
        this->mode = std::stoi(val, 0, 8);
        break;

    case 'o':
        this->owner = std::stoi(val);
        break;

    case 'g':
        this->group = std::stoi(val);
        break;
    }
}

void Brufscli::AddRootAction::assert_valid_inode_size() const {
    switch (this->inode_size) {
    case 128:
    case 256:
    case 512:
    case 1024:
    case 2048:
        return;

    default:
        throw UnsupportedInodeSizeException(std::to_string(this->inode_size));
    }
}

int Brufscli::AddRootAction::run([[maybe_unused]] const std::string &name) {
    auto path = this->path_parser.parse({this->spec.c_str(), this->spec.length()});
    this->path_validator.validate(path, true, true);

    auto brufs = this->opener.open_existing(path.get_partition());
    auto &fs = brufs.get_fs();
    const auto &io = brufs.get_io();

    // Fill in the blanks (unset parameters)
    if (this->mode == -1) {
        mode_t mode_mask = umask(0);
        umask(mode_mask);
        mode = 0777 & ~mode_mask;
    }

    if (this->owner == -1) {
        this->owner = geteuid();
    }

    if (this->group == -1) {
        this->group = getegid();
    }

    // Validate and provide feedback on the chosen parameters
    const auto cluster_size = fs.get_header().cluster_size;

    if (2 * this->inode_size > cluster_size) {
        throw UnsupportedInodeSizeException(
            "Inodes of " + std::to_string(this->inode_size) + " bytes "
            + "are too large to fit in " + std::to_string(cluster_size) + " byte "
            + "clusters"
        );
    }

    if (cluster_size / static_cast<double>(this->inode_size) < 6) {
        this->logger.note(
            "The chosen inode size (%hu bytes) is fairly large relative to the cluster size "
            "(%u bytes); this may cause space overhead and bad root performance",
            this->inode_size, cluster_size
        );
    }

    // Create the root
    Brufs::RootHeader root_header;
    strncpy(root_header.label, path.get_root().c_str(), Brufs::MAX_LABEL_LENGTH);

    root_header.inode_size = this->inode_size;
    root_header.inode_header_size = sizeof(Brufs::InodeHeader);
    root_header.max_extent_length = this->max_extent_length * cluster_size;

    Brufs::Root root(fs, root_header);
    auto status = fs.add_root(root);
    this->on_error(status, "Unable to insert the root into the filesystem: ", io);

    status = root.init();
    this->on_error(status, "Unable to initialize the root: ", io);

    // Create the root's actual directory
    Brufs::Directory root_dir(root);
    auto rdh = root_dir.get_header();
    rdh->created = rdh->last_modified = Brufs::Timestamp::now();
    rdh->owner = this->owner;
    rdh->group = this->group;
    rdh->num_links = 1;
    rdh->type = Brufs::InodeType::DIRECTORY;
    rdh->flags = 0;
    rdh->file_size = 0;
    rdh->checksum = 0;
    rdh->mode = this->mode;

    status = root_dir.init(Brufs::ROOT_DIR_INODE_ID, rdh);
    this->on_error(status, "Unable to initialize the root directory: ", io);

    status = root.insert_inode(Brufs::ROOT_DIR_INODE_ID, rdh);
    this->on_error(status, "Unable to insert the root directory into the root: ", io);

    // Create the . and .. links
    status = root_dir.insert(".", Brufs::ROOT_DIR_INODE_ID);
    this->on_error(status, "Unable to insert the . entry of the root directory: ", io);

    status = root_dir.insert("..", Brufs::ROOT_DIR_INODE_ID);
    this->on_error(status, "Unable to insert the .. entry of the root directory: ", io);

    return 0;
}
