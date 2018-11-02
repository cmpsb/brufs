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

#include <uv.h>

#include "Message.hpp"
#include "message-io.hpp"
#include "mount-request.hpp"
#include "uv-helper.hpp"

static void on_response(uv_stream_t *stream, Brufuse::Message *res) {
    printf("Response: length %lu; type %u; status %u\n",
        res->get_data_size(),
        res->get_type(),
        res->get_status()
    );

    if (res->get_status() != Brufuse::StatusCode::OK) {
        size_t ridx = 0;
        std::string message;
        res->read(ridx, message);
        fprintf(stderr, "%s\n", message.c_str());
    }

    Brufuse::close_and_free(stream);
    delete res;
}

static void on_message_sent(uv_write_t *wreq, int status) {
    Brufuse::await_response(wreq, status, on_response);
}

int Brufuse::request_mount(
    uv_connect_t *req,
    const std::string &root_name,
    const std::string &mount_point,
    const std::vector<std::string> &fuse_args
) {
    auto stream = req->handle;
    delete req;

    auto msg = new Brufuse::Message;
    msg->set_data_size(0);
    msg->set_sequence(0);
    msg->set_type(Brufuse::RequestType::MOUNT);
    msg->set_status(Brufuse::StatusCode::OK);

    size_t idx = 0;
    msg->write(idx, root_name);
    msg->write(idx, mount_point);

    uint32_t num_fuse_args = fuse_args.size();
    msg->write(idx, num_fuse_args);
    for (uint32_t i = 0; i < num_fuse_args; ++i) {
        msg->write(idx, fuse_args[i]);
    }

    Brufuse::write_message(stream, msg, on_message_sent);

    return 0;
}
