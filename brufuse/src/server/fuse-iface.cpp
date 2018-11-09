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

#include <cerrno>

#include <random>

#include "Util.hpp"
#include "service.hpp"

// Not yet in userspace, taken from kernel definitions
#ifndef RENAME_NOREPLACE
#  define RENAME_NOREPLACE (1 << 0)
#endif

#ifndef RENAME_EXCHANGE
#  define RENAME_EXCHANGE (1 << 1)
#endif

#ifndef RENAME_WHITEOUT
#  define RENAME_WHITEOUT (1 << 2)
#endif

static constexpr double DEFAULT_ATTR_TIMEOUT = 1;

fuse_lowlevel_ops Brufuse::fs_ops;

Brufs::Status Brufuse::MountedRoot::open_inode(
    const Brufs::InodeId &id, Brufs::Inode &ino, bool store
) {
    auto found_inode = this->open_inodes.find(id);
    if (found_inode != this->open_inodes.end()) {
        ino = *found_inode->second.inode;

        if (store) {
            ++found_inode->second.open_count;
        }

        return Brufs::Status::OK;
    }

    auto nino = new Brufs::Inode(*this->root);
    auto status = this->root->open_inode(id, *nino);
    if (status < Brufs::Status::OK) return status;

    auto store_anyway = (rand() / static_cast<double>(RAND_MAX)) < 0.8;
    if (store || store_anyway) {
        OpenedInode oino {nino, store ? 1UL : 0UL};
        this->open_inodes[id] = oino;
    }

    ino = *nino;

    return Brufs::Status::OK;
}

Brufs::Status Brufuse::MountedRoot::open_typed_inode(
    const Brufs::InodeId &id, Brufs::Inode &ino, Brufs::InodeType expected_type, bool store
) {
    auto status = this->open_inode(id, ino, store);
    if (status < Brufs::Status::OK) return status;

    if (ino.get_inode_type() != expected_type) return Brufs::Status::E_WRONG_INODE_TYPE;

    return Brufs::Status::OK;
}

Brufs::Status Brufuse::MountedRoot::open_file(
    const Brufs::InodeId &id, Brufs::File &ino, bool store
) {
    return this->open_typed_inode(id, ino, Brufs::InodeType::FILE, store);
}

Brufs::Status Brufuse::MountedRoot::open_directory(
    const Brufs::InodeId &id, Brufs::Directory &ino, bool store
) {
    return this->open_typed_inode(id, ino, Brufs::InodeType::DIRECTORY, store);
}

Brufs::Status Brufuse::MountedRoot::get_inode(const Brufs::InodeId &id, Brufs::Inode &ino) {
    return this->open_inode(id, ino, false);
}

Brufs::Status Brufuse::MountedRoot::get_file(const Brufs::InodeId &id, Brufs::File &ino) {
    return this->open_file(id, ino, false);
}

Brufs::Status Brufuse::MountedRoot::get_directory(const Brufs::InodeId &id, Brufs::Directory &ino) {
    return this->open_directory(id, ino, false);
}

void Brufuse::MountedRoot::update_inode(const Brufs::Inode &other) {
    auto found_inode = this->open_inodes.find(other.get_id());
    if (found_inode == this->open_inodes.end()) return;

    auto &ino = *found_inode->second.inode;
    ino = other;
}

static Brufs::InodeId ino_to_inode_id(fuse_ino_t ino) {
    if (ino == 1) return Brufs::ROOT_DIR_INODE_ID;
    return static_cast<Brufs::InodeId>(ino) << 6;
}

static inline fuse_ino_t inode_id_to_ino(Brufs::InodeId inode_id) {
    return static_cast<fuse_ino_t>(inode_id >> 6);
}

static inline Brufuse::MountedRoot *get_root_handle(fuse_req_t req) {
    return static_cast<Brufuse::MountedRoot *>(fuse_req_userdata(req));
}

/**
 * Maps a Brufs status code to a Unix errno.
 *
 * NB: there is no direct 1:1 relation between the codes, you may have to return a different
 * errno for a status code in some cases.
 *
 * The worst case is E_WRONG_INODE_TYPE, which is translated to ENOTDIR by default since there is no
 * corresponding context-sensitive errno. Tweak this value using the following parameter:
 *
 * @param on_wrong_inode_type the errno to return if the status is E_WRONG_INODE_TYPE
 *
 * @param status [description]
 * @return [description]
 */
static int status_to_errno_trace(
    const char *func, unsigned int line,
    Brufs::Status status, int on_wrong_inode_type = ENOTDIR
) {
    fprintf(stderr, "Translating error %s (%d) into errno in %s:%u\n",
        Brufuse::fs_io->strstatus(status), status, func, line
    );

    switch (status) {
        case Brufs::Status::E_INTERNAL: ;
        case Brufs::Status::E_NO_MEM: return ENOMEM;
        case Brufs::Status::E_DISK_TRUNCATED: return EIO;
        case Brufs::Status::E_BAD_MAGIC: return EIO;
        case Brufs::Status::E_FS_FROM_FUTURE: return EIO;
        case Brufs::Status::E_HEADER_TOO_BIG: return EIO;
        case Brufs::Status::E_HEADER_TOO_SMALL: return EIO;
        case Brufs::Status::E_CHECKSUM_MISMATCH: return EIO;
        case Brufs::Status::E_NO_SPACE: return ENOSPC;
        case Brufs::Status::E_WONT_FIT: return EFBIG;
        case Brufs::Status::E_NOT_FOUND: return ENOENT;
        case Brufs::Status::E_TOO_MANY_RETRIES: return EDEADLK;
        case Brufs::Status::E_AT_MAX_LEVEL: return ENOSPC;
        case Brufs::Status::E_CANT_ADOPT: return ENOSPC;
        case Brufs::Status::E_MISALIGNED: return EINVAL;
        case Brufs::Status::E_NO_FBT: return EIO;
        case Brufs::Status::E_NO_RHT: return EIO;
        case Brufs::Status::E_EXISTS: return EEXIST;
        case Brufs::Status::E_PILEUP: return ENOSPC;
        case Brufs::Status::E_BEYOND_EOF: return EINVAL;
        case Brufs::Status::E_STOPPED: return ECANCELED;
        case Brufs::Status::E_WRONG_INODE_TYPE: return on_wrong_inode_type;
        case Brufs::Status::OK: return 0;
        case Brufs::Status::RETRY: return ERESTART;
        case Brufs::Status::STOP: return 0;
        default: return -1;
    }
}

#define status_to_errno(...) status_to_errno_trace(__FUNCTION__, __LINE__, __VA_ARGS__)

static bool is_correct_perms(int mask, unsigned int mode) {
    return ((!(mask & R_OK)) || ((mode >> 2) & 1))
        && ((!(mask & W_OK)) || ((mode >> 1) & 1))
        && ((!(mask & X_OK)) || ((mode >> 0) & 1));
}

static void on_access(fuse_req_t req, fuse_ino_t ino, int mask) {
    Brufuse::ReadLock lock;
    const auto context = fuse_req_ctx(req);
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    Brufs::Inode inode(*root);
    auto status = root_handle->get_inode(ino_to_inode_id(ino), inode);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status));
        return;
    }

    if (mask == F_OK) {
        fuse_reply_err(req, 0);
        return;
    }

    auto inode_header = inode.get_header();
    int shift = 0;
    if (context->uid == inode_header->owner) shift = 6;
    else if (context->gid == inode_header->group) shift = 3;

    fuse_reply_err(req, is_correct_perms(mask >> shift, inode_header->mode) ? 0 : EACCES);
}

static void on_destroy(void *userdata) {
    Brufuse::WriteLock lock;
    auto root_handle = static_cast<Brufuse::MountedRoot *>(userdata);

    for (const auto &it : root_handle->open_inodes) {
        auto status = it.second.inode->destroy();
        if (status < Brufs::Status::OK) {
            const auto ino_str = Brufuse::Util::pretty_print_inode_id(it.first);
            fprintf(stderr, "Unable to flush %s at exit: %s\n",
                ino_str.c_str(), Brufuse::fs_io->strstatus(status)
            );
        }
    }
}

static void on_flush(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    (void) fi; // Use ino instead

    Brufuse::WriteLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    Brufs::Inode inode(*root);
    auto status = root_handle->get_inode(ino_to_inode_id(ino), inode);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status));
        return;
    }

    status = inode.store();
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status));
        return;
    }

    fuse_reply_err(req, 0);
}

static void on_forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup) {
    Brufuse::WriteLock lock;
    auto root_handle = get_root_handle(req);

    auto inode_id = ino_to_inode_id(ino);

    auto found_inode = root_handle->open_inodes.find(inode_id);
    if (found_inode == root_handle->open_inodes.end()) {
        fuse_reply_none(req);
        return;
    }

    auto &opened_inode = found_inode->second;
    opened_inode.open_count -= std::min(opened_inode.open_count, nlookup);

    if (opened_inode.open_count != 0) {
        fuse_reply_none(req);
        return;
    }

    auto deleted = opened_inode.inode->get_header()->num_links == 0;

    if (deleted) {
        auto status = opened_inode.inode->destroy();
        if (status < Brufs::Status::OK) {
            fprintf(stderr, "Unable to destroy inode: %s\n", Brufuse::fs_io->strstatus(status));
        }
    }

    if (deleted || (rand() / static_cast<double>(RAND_MAX)) < 0.2) {
        delete opened_inode.inode;
        root_handle->open_inodes.erase(inode_id);
    }

    fuse_reply_none(req);
}

static mode_t inode_header_to_mode(Brufs::InodeHeader *inode_header) {
    mode_t mode = inode_header->mode;

    switch (inode_header->type) {
        case Brufs::InodeType::NONE: break;
        case Brufs::InodeType::FILE: mode |= S_IFREG; break;
        case Brufs::InodeType::DIRECTORY: mode |= S_IFDIR; break;
        case Brufs::InodeType::SOFT_LINK: mode |= S_IFLNK; break;
        default: assert(false);
    }

    return mode;
}

static void inode_header_to_stat(
    Brufs::InodeId inode_id, Brufs::InodeHeader *inode_header, struct stat &attr
) {
    attr.st_ino = inode_id_to_ino(inode_id);
    attr.st_mode = inode_header_to_mode(inode_header);
    attr.st_nlink = inode_header->num_links ? inode_header->num_links : 1;
    attr.st_uid = static_cast<uid_t>(inode_header->owner);
    attr.st_gid = static_cast<gid_t>(inode_header->group);
    attr.st_size = inode_header->file_size;
    attr.st_blocks = inode_header->file_size / 512;

    attr.st_atim.tv_sec = inode_header->last_modified.seconds;
    attr.st_atim.tv_nsec = inode_header->last_modified.nanoseconds;

    attr.st_mtim.tv_sec = inode_header->last_modified.seconds;
    attr.st_mtim.tv_nsec = inode_header->last_modified.nanoseconds;

    attr.st_ctim.tv_sec = inode_header->created.seconds;
    attr.st_ctim.tv_nsec = inode_header->created.nanoseconds;
}

static int get_attr(fuse_req_t req, Brufs::InodeId inode_id, struct stat &attr) {
    Brufuse::ReadLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    Brufs::Inode inode(*root);
    auto status = root_handle->get_inode(inode_id, inode);
    if (status < Brufs::Status::OK) {
        return status_to_errno(status);
    }

    inode_header_to_stat(inode_id, inode.get_header(), attr);

    return 0;
}

static void on_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    (void) fi; // "for future use, currently always NULL"

    struct stat attr;
    int status = get_attr(req, ino_to_inode_id(ino), attr);
    if (status < 0) {
        fuse_reply_err(req, status);
        return;
    }

    fuse_reply_attr(req, &attr, DEFAULT_ATTR_TIMEOUT);
}

static void on_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
    Brufuse::ReadLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    Brufs::Directory dir(*root);
    auto status = root_handle->get_directory(ino_to_inode_id(parent), dir);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status, ENOTDIR));
        return;
    }

    Brufs::DirectoryEntry entry;
    status = dir.look_up(name, entry);
    if (status < Brufs::Status::OK) {
        fprintf(stderr, "Unable to look up %s\n", name);
        fuse_reply_err(req, status_to_errno(status));
        return;
    }

    struct fuse_entry_param fuse_entry;
    fuse_entry.ino = inode_id_to_ino(entry.inode_id);
    fuse_entry.generation = 1;
    fuse_entry.attr_timeout = DEFAULT_ATTR_TIMEOUT;
    fuse_entry.entry_timeout = DEFAULT_ATTR_TIMEOUT;

    int estatus = get_attr(req, entry.inode_id, fuse_entry.attr);
    if (estatus < 0) {
        fuse_reply_err(req, estatus);
        return;
    }

    fuse_reply_entry(req, &fuse_entry);
}

static void on_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode) {
    Brufuse::WriteLock lock;
    const auto context = fuse_req_ctx(req);
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;
    auto parent_inode_id = ino_to_inode_id(parent);

    Brufs::Directory dir(*root);
    auto status = root_handle->get_directory(parent_inode_id, dir);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status, ENOTDIR));
        return;
    }

    Brufs::Directory new_dir(*root);

    auto rdh = new_dir.get_header();
    rdh->created = Brufs::Timestamp::now();
    rdh->last_modified = Brufs::Timestamp::now();
    rdh->owner = context->uid;
    rdh->group = context->gid;
    rdh->num_links = 1;
    rdh->type = Brufs::InodeType::DIRECTORY;
    rdh->flags = 0;
    rdh->num_extents = 0;
    rdh->file_size = 0;
    rdh->checksum = 0;
    rdh->mode = mode;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint64_t> dist;

    auto new_inode_id = ino_to_inode_id(dist(mt));

    status = root->insert_inode(new_inode_id, new_dir);
    if (status < Brufs::Status::OK) goto reply_status;

    status = new_dir.init(new_inode_id, rdh);
    if (status < Brufs::Status::OK) goto reply_status;

    Brufs::DirectoryEntry entry;
    entry.set_label(name);
    entry.inode_id = new_inode_id;

    status = dir.insert(entry);
    if (status < Brufs::Status::OK) goto clean_on_error;
    root_handle->update_inode(dir);

    Brufs::DirectoryEntry dot_entry;
    dot_entry.set_label(".");
    dot_entry.inode_id = new_inode_id;
    status = new_dir.insert(dot_entry);
    if (status < Brufs::Status::OK) goto clean_on_error;

    Brufs::DirectoryEntry dot_dot_entry;
    dot_dot_entry.set_label("..");
    dot_dot_entry.inode_id = parent_inode_id;
    status = new_dir.insert(dot_dot_entry);
    if (status < Brufs::Status::OK) goto clean_on_error;

    struct fuse_entry_param fuse_entry;
    fuse_entry.ino = inode_id_to_ino(new_inode_id);
    fuse_entry.generation = 1;
    fuse_entry.attr_timeout = DEFAULT_ATTR_TIMEOUT;
    fuse_entry.entry_timeout = DEFAULT_ATTR_TIMEOUT;

    inode_header_to_stat(new_inode_id, rdh, fuse_entry.attr);

    fuse_reply_entry(req, &fuse_entry);
    return;

clean_on_error:
    (void) new_dir.destroy();
    (void) root->remove_inode(new_inode_id, new_dir);

    // fall through
reply_status:
    fuse_reply_err(req, status_to_errno(status));
    return;
}

static void on_mknod(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, dev_t rdev) {
    (void) rdev; // Not supported by Brufs

    Brufuse::WriteLock lock;
    const auto context = fuse_req_ctx(req);
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;
    auto parent_inode_id = ino_to_inode_id(parent);

    Brufs::Directory dir(*root);
    auto status = root_handle->get_directory(parent_inode_id, dir);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status, ENOTDIR));
        return;
    }

    Brufs::File new_file(*root);

    auto rdh = new_file.get_header();
    rdh->created = Brufs::Timestamp::now();
    rdh->last_modified = Brufs::Timestamp::now();
    rdh->owner = context->uid;
    rdh->group = context->gid;
    rdh->num_links = 1;
    rdh->type = Brufs::InodeType::FILE;
    rdh->flags = 0;
    rdh->num_extents = 0;
    rdh->file_size = 0;
    rdh->checksum = 0;
    rdh->mode = mode;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint64_t> dist;

    auto new_inode_id = ino_to_inode_id(dist(mt));

    status = root->insert_inode(new_inode_id, new_file);
    if (status < Brufs::Status::OK) goto reply_status;

    status = new_file.init(new_inode_id, rdh);
    if (status < Brufs::Status::OK) goto reply_status;

    Brufs::DirectoryEntry entry;
    entry.set_label(name);
    entry.inode_id = new_inode_id;

    status = dir.insert(entry);
    if (status < Brufs::Status::OK) goto clean_on_error;
    root_handle->update_inode(dir);

    struct fuse_entry_param fuse_entry;
    fuse_entry.ino = inode_id_to_ino(new_inode_id);
    fuse_entry.generation = 1;
    fuse_entry.attr_timeout = DEFAULT_ATTR_TIMEOUT;
    fuse_entry.entry_timeout = DEFAULT_ATTR_TIMEOUT;

    inode_header_to_stat(new_inode_id, rdh, fuse_entry.attr);

    fuse_reply_entry(req, &fuse_entry);
    return;

clean_on_error:
    (void) new_file.destroy();
    (void) root->remove_inode(new_inode_id, new_file);

    // fall through
reply_status:
    fuse_reply_err(req, status_to_errno(status));
    return;
}

static void on_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    Brufuse::ReadLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;
    auto inode_id = ino_to_inode_id(ino);

    Brufs::File file(*root);
    auto status = root_handle->open_file(inode_id, file);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status, EISDIR));
        return;
    }

    fi->direct_io = false;
    fi->keep_cache = true;

    fuse_reply_open(req, fi);
}

static void on_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    Brufuse::ReadLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;
    auto inode_id = ino_to_inode_id(ino);

    Brufs::Directory dir(*root);
    auto status = root_handle->open_directory(inode_id, dir);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status, ENOTDIR));
        return;
    }

    fi->direct_io = false;
    fi->keep_cache = true;

    fuse_reply_open(req, fi);
}

static void on_read(
    fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi
) {
    (void) fi; // Unused; uses ino instead

    Brufuse::ReadLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    Brufs::File file(*root);
    auto status = root_handle->get_file(ino_to_inode_id(ino), file);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status));
        return;
    }

    auto uoff = static_cast<size_t>(off);
    if (off < 0 || uoff > file.get_header()->file_size) {
        fuse_reply_err(req, EINVAL);
        return;
    }

    auto true_size = std::min(uoff + size, file.get_header()->file_size) - uoff;
    auto buf = new char[true_size];

    size_t total = 0;
    while (total < true_size) {
        auto num_read = file.read(buf + total, true_size - total, uoff + total);
        if (num_read < Brufs::Status::OK) {
            delete[] buf;
            fuse_reply_err(req, status_to_errno(static_cast<Brufs::Status>(num_read)));
            return;
        }

        total += num_read;
    }

    fuse_reply_buf(req, buf, total);
    delete[] buf;
}

static void on_readdir(
    fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi
) {
    (void) fi; // Unused; uses ino instead

    Brufuse::ReadLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    Brufs::Directory dir(*root);
    auto status = root_handle->get_directory(ino_to_inode_id(ino), dir);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status));
        return;
    }

    Brufs::Vector<Brufs::DirectoryEntry> entries;
    status = dir.collect(entries);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status));
        return;
    }

    auto buf = new char[size];
    size_t total = 0;
    Brufs::Inode hdr(*root);

    for (size_t i = static_cast<size_t>(off); i < entries.get_size(); ++i) {
        const auto &entry = entries[i];
        char label[Brufs::MAX_LABEL_LENGTH + 1];
        memcpy(label, entry.label, Brufs::MAX_LABEL_LENGTH);
        label[Brufs::MAX_LABEL_LENGTH] = 0;

        status = root_handle->get_inode(entry.inode_id, hdr);
        if (status < Brufs::Status::OK) {
            fprintf(stderr, "readdir: find_inode %s %lX:%lX -> %s\n",
                label,
                (uint64_t) (entry.inode_id >> 64), (uint64_t) (entry.inode_id),
                Brufuse::fs_io->strstatus(status)
            );
            memset(hdr.get_header(), 0, sizeof(Brufs::InodeHeader));
        }

        struct stat attr;
        inode_header_to_stat(entry.inode_id, hdr.get_header(), attr);
        size_t entry_size = fuse_add_direntry(
            req, buf + total, size - total, label, &attr, static_cast<off_t>(i + 1)
        );

        if (entry_size > size - total) break;

        total += entry_size;
    }

    fuse_reply_buf(req, buf, total);

    delete[] buf;
}

void on_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    // Wait until the kernel forgets the inode
    (void) ino;
    (void) fi;

    fuse_reply_err(req, 0);
}

void on_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    // Wait until the kernel forgets the inode
    (void) ino;
    (void) fi;

    fuse_reply_err(req, 0);
}

void on_rename(
    fuse_req_t req,
    fuse_ino_t old_parent_ino, const char *old_name,
    fuse_ino_t new_parent_ino, const char *new_name,
    unsigned int flags
) {
    Brufuse::WriteLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    bool new_already_exists;

    Brufs::Directory old_parent(*root);
    Brufs::Directory new_parent(*root);

    auto status = root_handle->get_directory(ino_to_inode_id(old_parent_ino), old_parent);
    if (status < Brufs::Status::OK) goto reply_status;

    status = root_handle->get_directory(ino_to_inode_id(new_parent_ino), new_parent);
    if (status < Brufs::Status::OK) goto reply_status;

    Brufs::DirectoryEntry new_entry;
    status = new_parent.look_up(new_name, new_entry);

    new_already_exists = status == Brufs::Status::OK;
    if (new_already_exists && (flags & RENAME_NOREPLACE)) {
        fuse_reply_err(req, EEXIST);
        return;
    }

    if (status != Brufs::Status::OK && status != Brufs::Status::E_NOT_FOUND) goto reply_status;

    Brufs::DirectoryEntry old_entry;
    if (flags & RENAME_EXCHANGE) {
        status = old_parent.look_up(old_name, old_entry);
    } else {
        status = old_parent.remove(old_name, old_entry);
        root_handle->update_inode(old_parent);
    }

    if (status < Brufs::Status::OK) goto reply_status;

    std::swap(new_entry.inode_id, old_entry.inode_id);

    if (new_already_exists) {
        status = new_parent.update(new_entry);
    } else {
        new_entry.set_label(new_name);
        status = new_parent.insert(new_entry);
    }
    if (status < Brufs::Status::OK) goto reply_status;

    root_handle->update_inode(new_parent);

    if (flags & RENAME_EXCHANGE) {
        status = old_parent.update(old_entry);
        if (status < Brufs::Status::OK) goto reply_status;
    } else if (new_already_exists) {
        Brufs::Inode old_inode(*root);
        status = root_handle->get_inode(old_entry.inode_id, old_inode);
        if (status < Brufs::Status::OK) goto reply_status;

        if (old_inode.get_header()->num_links > 0) --old_inode.get_header()->num_links;

        root_handle->update_inode(old_inode);
    }

    fuse_reply_err(req, 0);
    return;

reply_status:
    fuse_reply_err(req, status_to_errno(status));
    return;
}

static void on_rmdir(fuse_req_t req, fuse_ino_t parent_ino, const char *name) {
    Brufuse::WriteLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    Brufs::Directory parent(*root);
    Brufs::Directory dir(*root);

    auto status = root_handle->get_directory(ino_to_inode_id(parent_ino), parent);
    if (status == Brufs::Status::E_WRONG_INODE_TYPE) goto reply_status;

    Brufs::DirectoryEntry child_entry;
    status = parent.look_up(name, child_entry);
    if (status < Brufs::Status::OK) goto reply_status;

    status = root_handle->get_directory(child_entry.inode_id, dir);
    if (status < Brufs::Status::OK) goto reply_status;

    if (dir.count() > 2) { // Directories always contain . and ..
        fuse_reply_err(req, ENOTEMPTY);
        return;
    }

    status = parent.remove(child_entry);
    if (status < Brufs::Status::OK) goto reply_status;

    if (dir.get_header()->num_links > 0) --dir.get_header()->num_links;

    root_handle->update_inode(parent);
    root_handle->update_inode(dir);

    fuse_reply_err(req, 0);
    return;

reply_status:
    fuse_reply_err(req, status_to_errno(status));
    return;
}

void on_setattr(
    fuse_req_t req, fuse_ino_t ino_num, struct stat *attr, int to_set, struct fuse_file_info *fi
) {
    (void) fi; // Use ino_num instead

    Brufuse::WriteLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    Brufs::Inode ino(*root);
    auto status = root_handle->get_inode(ino_to_inode_id(ino_num), ino);
    if (status < Brufs::Status::OK) {
        fuse_reply_err(req, status_to_errno(status));
        return;
    }

    if (to_set & FUSE_SET_ATTR_MODE) {
        ino.get_header()->mode = attr->st_mode & 0777;
    }

    if (to_set & FUSE_SET_ATTR_UID) {
        ino.get_header()->owner = attr->st_uid;
    }

    if (to_set & FUSE_SET_ATTR_GID) {
        ino.get_header()->group = attr->st_gid;
    }

    if (to_set & FUSE_SET_ATTR_SIZE && ino.get_inode_type() == Brufs::InodeType::FILE) {
        Brufs::File file = ino;
        auto status = file.truncate(attr->st_size);
        if (status < Brufs::Status::OK) {
            fuse_reply_err(req, status_to_errno(status));
            return;
        }
        ino = file;
    } else if (to_set & FUSE_SET_ATTR_SIZE) {
        fuse_reply_err(req, EINVAL);
        return;
    }

    if (to_set & FUSE_SET_ATTR_MTIME) {
        ino.get_header()->last_modified.seconds = attr->st_mtim.tv_sec;
        ino.get_header()->last_modified.nanoseconds = attr->st_mtim.tv_nsec;
    }

    if (to_set & FUSE_SET_ATTR_MTIME_NOW) {
        ino.get_header()->last_modified = Brufs::Timestamp::now();
    }

    if (to_set & FUSE_SET_ATTR_CTIME) {
        ino.get_header()->created.seconds = attr->st_ctim.tv_sec;
        ino.get_header()->created.nanoseconds = attr->st_ctim.tv_nsec;
    }

    root_handle->update_inode(ino);

    struct stat new_attr;
    inode_header_to_stat(ino_to_inode_id(ino_num), ino.get_header(), new_attr);

    fuse_reply_attr(req, &new_attr, DEFAULT_ATTR_TIMEOUT);
}

void on_unlink(fuse_req_t req, fuse_ino_t parent_ino, const char *name) {
    Brufuse::WriteLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    Brufs::Directory parent(*root);
    Brufs::DirectoryEntry entry;
    Brufs::Inode inode(*root);

    auto status = root_handle->get_directory(ino_to_inode_id(parent_ino), parent);
    if (status < Brufs::Status::OK) goto reply_status;

    status = parent.remove(name, entry);
    if (status < Brufs::Status::OK) goto reply_status;

    root_handle->update_inode(parent);

    status = root_handle->get_inode(entry.inode_id, inode);
    if (status < Brufs::Status::OK) goto reply_status;

    if (inode.get_header()->num_links > 0) --inode.get_header()->num_links;

    root_handle->update_inode(inode);

    fuse_reply_err(req, 0);
    return;

reply_status:
    fuse_reply_err(req, status_to_errno(status));
    return;
}

static void on_write(
    fuse_req_t req, fuse_ino_t ino,
    const char *buf, size_t size, off_t off,
    struct fuse_file_info *fi
) {
    (void) fi; // Use ino instead.

    Brufuse::WriteLock lock;
    auto root_handle = get_root_handle(req);
    auto root = root_handle->root;

    if (off < 0) {
        fuse_reply_err(req, EINVAL);
        return;
    }

    auto uoff = static_cast<Brufs::Offset>(off);

    Brufs::File file(*root);
    auto status = root_handle->get_file(ino_to_inode_id(ino), file);

    size_t total = 0;
    while (total < size) {
        auto sstatus = file.write(buf + total, size - total, uoff + total);
        if (sstatus < 0) {
            status = static_cast<Brufs::Status>(sstatus);
            goto reply_status;
        }

        total += sstatus;
    }

    root_handle->update_inode(file);

    fuse_reply_write(req, total);
    return;

reply_status:
    fuse_reply_err(req, status_to_errno(status));
    return;
}

void Brufuse::init_fs_ops() {
    memset(&fs_ops, 0, sizeof(struct fuse_lowlevel_ops));

    fs_ops.access = on_access;
    fs_ops.destroy = on_destroy;
    fs_ops.flush = on_flush;
    fs_ops.forget = on_forget;
    fs_ops.getattr = on_getattr;
    fs_ops.lookup = on_lookup;
    fs_ops.mkdir = on_mkdir;
    fs_ops.mknod = on_mknod;
    fs_ops.open = on_open;
    fs_ops.opendir = on_opendir;
    fs_ops.read = on_read;
    fs_ops.readdir = on_readdir;
    fs_ops.release = on_release;
    fs_ops.releasedir = on_releasedir;
    fs_ops.rename = on_rename;
    fs_ops.rmdir = on_rmdir;
    fs_ops.setattr = on_setattr;
    fs_ops.unlink = on_unlink;
    fs_ops.write = on_write;
}
