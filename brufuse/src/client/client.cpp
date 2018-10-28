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

#include <uv.h>

#include "client.hpp"

static uv_loop_t loop;
static uv_pipe_t *ppe;

int Brufuse::run_client(const std::string &socket_path, uv_connect_cb cb) {
    auto status = uv_loop_init(&loop);
    if (status < 0) {
        fprintf(stderr, "Unable to initialize the event loop: %s\n", uv_strerror(status));
        return 1;
    }

    ppe = new uv_pipe_t;
    status = uv_pipe_init(&loop, ppe, false);
    if (status < 0) {
        fprintf(stderr, "Unable to initialize the pipe to the server: %s\n", uv_strerror(status));
        return 1;
    }

    auto creq = new uv_connect_t;
    uv_pipe_connect(creq, ppe, socket_path.c_str(), cb);

    status = uv_run(&loop, UV_RUN_DEFAULT);
    if (status < 0) {
        fprintf(stderr, "Unable to run the event loop: %s\n", uv_strerror(status));
        return 1;
    }

    return 0;
}
