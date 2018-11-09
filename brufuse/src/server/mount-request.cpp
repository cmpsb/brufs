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

#include <cstdlib>
#include <cstdint>

#include "service.hpp"
#include "Message.hpp"

std::map<std::string, Brufuse::MountedRoot *> Brufuse::mounted_roots;

static void run_root_thread(void *pl) {
    auto mounted_root = static_cast<Brufuse::MountedRoot *>(pl);
    fuse_session_loop_mt_31(mounted_root->session, true);
    fuse_session_unmount(mounted_root->session);
    fuse_session_destroy(mounted_root->session);

    Brufuse::mounted_roots.erase(mounted_root->root_name);

    delete mounted_root->root;
    delete mounted_root->thread;
    delete mounted_root;
}

void Brufuse::handle_mount_request(const Message &req, Message *res) {
    // Read the arguments
    size_t ridx = 0;

    std::string root_name;
    req.read(ridx, root_name);

    std::string mount_point;
    req.read(ridx, mount_point);

    uint32_t num_fuse_args;
    req.read(ridx, num_fuse_args);

    std::vector<std::string> fuse_args;

    for (uint32_t i = 0; i < num_fuse_args; ++i) {
        std::string arg;
        req.read(ridx, arg);

        fuse_args.push_back(arg);
    }

    // Sanitize the arguments
    auto find_res = mounted_roots.find(root_name);
    if (find_res != mounted_roots.end()) {
        res->set_status(StatusCode::ALREADY_MOUNTED);

        size_t widx = 0;
        res->write(widx,
            "Root \"" + root_name + "\" is already mounted at " + find_res->second->mount_point
        );
        res->write(widx, root_name);
        res->write(widx, find_res->second->mount_point);

        return;
    }

    Brufs::RootHeader root_header;
    auto status = fs->find_root(root_name.c_str(), root_header);
    if (status < Brufs::Status::OK) {
        res->set_status(StatusCode::NOT_FOUND);

        size_t widx = 0;
        res->write(widx, "Unable to find root \"" + root_name + "\"");
        res->write(widx, root_name);

        return;
    }

    struct stat statbuf;
    auto stat_status = stat(mount_point.c_str(), &statbuf);
    if (stat_status) {
        res->set_status(StatusCode::NOT_FOUND);

        size_t widx = 0;
        res->write(widx, "Unable to find mount point " + mount_point);
        res->write(widx, mount_point);

        return;
    }

    // Create the mounted root handle
    auto mounted_root = new MountedRoot;
    mounted_root->root_name = root_name;
    mounted_root->mount_point = mount_point;
    mounted_root->root = new Brufs::Root(*fs, root_header);

    mounted_roots[root_name] = mounted_root;

    // Create the session
    struct fuse_args raw_fuse_args = FUSE_ARGS_INIT(0, nullptr);
    fuse_opt_parse(&raw_fuse_args, nullptr, nullptr, nullptr);
    fuse_opt_add_arg(&raw_fuse_args, "");

    for (unsigned int i = 0; i < fuse_args.size(); ++i) {
        fuse_opt_add_arg(&raw_fuse_args, "-o");
        fuse_opt_add_arg(&raw_fuse_args, fuse_args[i].c_str());
    }

    mounted_root->session = fuse_session_new(
        &raw_fuse_args, &fs_ops, sizeof(struct fuse_lowlevel_ops), mounted_root
    );

    fuse_opt_free_args(&raw_fuse_args);

    // Mount the session
    fuse_session_mount(mounted_root->session, mount_point.c_str());

    // Run the session
    mounted_root->thread = new uv_thread_t;
    uv_thread_create(mounted_root->thread, run_root_thread, mounted_root);
}
