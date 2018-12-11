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

#include "LsAction.hpp"
#include "Util.hpp"

std::vector<std::string> Brufscli::LsAction::get_names() const {
    return {"ls", "ll", "la", "dir"};
}

std::vector<slopt_Option> Brufscli::LsAction::get_options() const {
    return {
        {'a', "all", SLOPT_DISALLOW_ARGUMENT},
        {'l', "list", SLOPT_DISALLOW_ARGUMENT}
    };
}

void Brufscli::LsAction::apply_option(
    int sw, int snam, [[maybe_unused]] const std::string &lnam, const std::string &val
) {
    if (sw == SLOPT_DIRECT && this->spec.empty()) {
        this->spec = val;
        return;
    }

    if (sw == SLOPT_DIRECT) {
        throw InvalidArgumentException("Unexpected value " + val + " (path is " + this->spec + ")");
    }

    switch (snam) {
    case 'a':
        this->all = true;
        break;

    case 'l':
        this->list = true;
        break;
    }
}

int Brufscli::LsAction::run(const std::string &name) {
    if (name == "ll") {
        this->list = true;
    }

    if (name == "la") {
        this->list = true;
        this->all = true;
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

    Brufs::Inode inode(root);
    status = root.open_inode(path, inode);
    this->on_error(status, "Unable to open the inode: ", io);

    Brufs::Vector<Brufs::DirectoryEntry> raw_entries;
    if (inode.has_type(Brufs::InodeType::DIRECTORY)) {
        Brufs::Directory dir(inode);

        status = dir.collect(raw_entries);
        this->on_error(status, "Unable to read the directory: ", io);
    } else {
        raw_entries.push_back({path.get_components().back(), inode.get_id()});
    }

    std::vector<Brufs::DynamicDirectoryEntry> entries;
    std::transform(raw_entries.begin(), raw_entries.end(), std::back_inserter(entries),
        [](const auto &entry) { return Brufs::DynamicDirectoryEntry(entry); }
    );

    if (this->list) {
        this->print_as_list(brufs, root, entries);
    } else {
        this->print_as_line(brufs, root, entries);
    }

    return 0;
}

void Brufscli::LsAction::print_as_line(
    BrufsInstance &brufs, Brufs::Root &root,
    std::vector<Brufs::DynamicDirectoryEntry> entries
) {
    const auto &io = brufs.get_io();

    auto hdr = root.create_inode_header();

    bool print_sep = false;

    for (const auto &entry : entries) {
        if (print_sep) printf(" \x1E ");
        else print_sep = true;

        Brufs::String inode_str = Util::pretty_print_inode_id(entry.get_inode_id());

        auto status = root.find_inode(entry.get_inode_id(), hdr);
        if (status < Brufs::Status::OK) {
            this->logger.warn("Unable to load inode %s: %s",
                inode_str.c_str(), io.strstatus(status)
            );
            continue;
        }

        auto is_dir = hdr->type == Brufs::InodeType::DIRECTORY;
        printf("%s%s", entry.get_label().c_str(), is_dir ? "/" : "");
    }

    root.destroy_inode_header(hdr);

    printf("\n");
}

void Brufscli::LsAction::print_as_list(
    BrufsInstance &brufs, Brufs::Root &root,
    std::vector<Brufs::DynamicDirectoryEntry> entries
) {
    const auto &io = brufs.get_io();

    std::sort(entries.begin(), entries.end(), [](const auto &left, const auto &right) {
        const auto &llabel = left.get_label();
        const auto &rlabel = right.get_label();

        return std::string_view(llabel.c_str(), llabel.length())
             < std::string_view(rlabel.c_str(), rlabel.length());
    });

    if (!this->all) {
        entries.erase(std::remove_if(entries.begin(), entries.end(), [](const auto &entry) {
            return entry.get_label() == "." || entry.get_label() == "..";
        }), entries.end());
    }

    std::vector<EntryInodeCombo> inodes;
    inodes.reserve(entries.size());
    for (const auto &entry : entries) {
        Brufs::Inode inode(root);

        const auto inode_str = Util::pretty_print_inode_id(entry.get_inode_id());

        auto status = root.open_inode(entry.get_inode_id(), inode);
        if (status < Brufs::Status::OK) {
            this->logger.warn("Unable to load inode %s: %s",
                inode_str.c_str(), io.strstatus(status)
            );
            continue;
        }

        inodes.push_back({entry, inode});
    }

    for (const auto &entry : inodes) {
        if (entry.inode.get_inode_type() != Brufs::InodeType::DIRECTORY) continue;
        this->print_list_item(entry);
    }

    for (const auto &entry : inodes) {
        if (entry.inode.get_inode_type() == Brufs::InodeType::DIRECTORY) continue;
        this->print_list_item(entry);
    }
}

void Brufscli::LsAction::print_list_item(const EntryInodeCombo &entry) {
    auto mtime_str = Util::pretty_print_timestamp(entry.inode.get_header()->last_modified);

    auto is_dir = entry.inode.get_inode_type() == Brufs::InodeType::DIRECTORY;
    auto mode_str = Util::pretty_print_mode(is_dir, entry.inode.get_header()->mode);

    printf("%s %s %5lu  %s%s\n",
        mode_str.c_str(), mtime_str.c_str(),
        entry.inode.get_header()->file_size,
        entry.entry.get_label().c_str(),
        is_dir ? "/" : ""
    );
}
