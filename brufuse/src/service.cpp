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
#include <cstring>

#include "service.hpp"

static uv_loop_t loop;
static uv_pipe_t pipe;

static void on_new_conn(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "Unable to accept request connection: %s\n", uv_strerror(status));
        return;
    }

    uv_pipe_t *client = new uv_pipe_t;
    status = uv_pipe_init(&loop, client, false);
    if (status < 0) {
        fprintf(stderr, "Unable to initialize the client socket: %s\n", uv_strerror(status));
        return;
    }

    status = uv_accept(server, reinterpret_cast<uv_stream_t *>(client));
    if (status < 0) {
        fprintf(stderr, "Unable to accept the client connection: %s\n", uv_strerror(status));
        return;
    }

    handle_connection(client);
}

int launch_service(
    const std::string &socket_path, 
    const unsigned int socket_mode,
    const std::string &dev_path
) {
    int iofd = open(dev_path.c_str(), O_RDWR);
    if (iofd == -1) {
        fprintf(stderr, "Unable to open %s: %s\n", dev_path.c_str(), strerror(errno));
        return 1;
    }

    auto status = uv_loop_init(&loop);
    if (status < 0) {
        fprintf(stderr, "Unable to initialize the request event loop: %s\n", uv_strerror(status));
        return 1;
    }

    status = uv_pipe_init(&loop, &pipe, false);
    if (status < 0) {
        fprintf(stderr, "Unable to initialize the request socket: %s\n", uv_strerror(status));
        return 1;
    }

    status = uv_pipe_bind(&pipe, socket_path.c_str());
    if (status < 0) {
        fprintf(stderr, "Unable to bind the socket to %s: %s\n", 
            socket_path.c_str(), uv_strerror(status)
        );
        return 1;
    }

    status = uv_listen(reinterpret_cast<uv_stream_t *>(&pipe), 32, on_new_conn);
    if (status < 0) {
        fprintf(stderr, "Unable to listen for requests: %s\n", uv_strerror(status));
        return 1;
    }

    status = uv_run(&loop, UV_RUN_DEFAULT);
    if (status < 0) {
        fprintf(stderr, "Error while listening for requests: %s\n", uv_strerror(status));
        return 1;
    }

    return 0;
}
