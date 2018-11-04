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

#include <string>
#include <vector>
#include <map>

#include <uv.h>

#define FUSE_USE_VERSION 31
#include <fuse3/fuse_lowlevel.h>

#include "libbrufs.hpp"

#include "types.hpp"
#include "Message.hpp"
#include "FdAbst.hpp"

namespace Brufuse {

struct MountedRoot {
    std::string mount_point;
    Brufs::Root *root;
    struct fuse_session *session;
    uv_thread_t *thread;
    std::map<Brufs::InodeId, Brufs::Inode *> open_inodes;
};

extern fuse_lowlevel_ops fs_ops;

extern Brufs::Brufs *fs;
extern FdAbst *fs_io;
extern uv_rwlock_t fs_rwlock;

extern std::map<std::string, MountedRoot *> mounted_roots;

class ReadLock {
public:
    ReadLock() {
        uv_rwlock_rdlock(&fs_rwlock);
    }

    ~ReadLock() {
        uv_rwlock_rdunlock(&fs_rwlock);
    }
};

class WriteLock {
public:
    WriteLock() {
        uv_rwlock_wrlock(&fs_rwlock);
    }

    ~WriteLock() {
        uv_rwlock_wrunlock(&fs_rwlock);
    }
};

int launch_service(
    const std::string &socket_path,
    const unsigned int socket_mode,
    const std::string &dev_path
);

void stop_service() __attribute__((noreturn));

void handle_connection(uv_pipe_t *client);

void handle_mount_request(const Message &req, Message *res);

void open_fs(const std::string &dev_path);
void init_fs_ops();

}
