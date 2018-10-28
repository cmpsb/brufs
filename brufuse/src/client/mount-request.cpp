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

static uv_loop_t loop;

static void on_close(uv_handle_t *handle) {
    delete handle;
}

static void on_response(uv_stream_t *stream, Brufuse::Message *res) {
    printf("Response: length %lu; type %u; status %u\n",
        res->get_data_size(),
        res->get_type(),
        res->get_status()
    );

    uv_close(reinterpret_cast<uv_handle_t *>(stream), on_close);
    delete res;
}

static void on_message_sent(uv_write_t *wreq, int status) {
    auto stream = wreq->handle;
    auto msg = static_cast<Brufuse::Message *>(stream->data);

    if (status >= 0) {
        status = Brufuse::read_message(stream, on_response);
        if (status < 0) {
            fprintf(stderr, "Unable to read the response: %s\n", uv_strerror(status));
        }
    } else {
        fprintf(stderr, "Unable to write the message: %s\n", uv_strerror(status));
    }

    delete msg;
    delete wreq;
}

static void on_connect(uv_connect_t *req) {
    auto stream = req->handle;
    delete req;

    auto msg = new Brufuse::Message;
    msg->set_data_size(0);
    msg->set_sequence(0);
    msg->set_type(Brufuse::RequestType::NONE);
    msg->set_status(Brufuse::StatusCode::OK);

    Brufuse::write_message(stream, msg, on_message_sent);
}

int Brufuse::request_mount(
    uv_connect_t *req,
    const std::string &root_name,
    const std::string &mount_point,
    const std::vector<std::string> &fuse_args
) {
    on_connect(req);
    return 1;
}
