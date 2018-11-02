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
#include <unistd.h>

#include "service.hpp"

static std::string socket_path;

static uv_loop_t loop;
static uv_pipe_t server_pipe;
static uv_signal_t sigint_handler;
static uv_signal_t sighup_handler;

static void on_close_at_closing(uv_handle_t *handle) {
    (void) handle;
}

static void close_handle(uv_handle_t *handle, void *pl) {
    (void) pl;

    if (!uv_is_closing(handle)) uv_close(handle, on_close_at_closing);
}

void Brufuse::stop_service() {
    int status = 0;

    uv_stop(&loop);
    uv_walk(&loop, close_handle, nullptr);

    status = uv_run(&loop, UV_RUN_ONCE);
    if (status < 0) {
        fprintf(stderr, "Unable to run the loop: %s\n", uv_strerror(status));
    }

    status = uv_loop_close(&loop);
    if (status != UV_EBUSY) {
        fprintf(stderr, "Unable to close the loop: %s\n", uv_strerror(status));
    }

    unlink(socket_path.c_str());

    exit(status >= 0);
}

static void on_sigint(uv_signal_t *handle, int signum) {
    (void) handle;
    (void) signum;

    Brufuse::stop_service();
}

static void on_sighup(uv_signal_t *handle, int signum) {
    (void) handle;
    (void) signum;
}

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

    Brufuse::handle_connection(client);
}

int Brufuse::launch_service(
    const std::string &sock_path,
    const unsigned int socket_mode,
    const std::string &dev_path
) {
    socket_path = sock_path;

    // Initialize the file system and the FUSE interface
    Brufuse::init_fs_ops();
    Brufuse::open_fs(dev_path);

    auto status = uv_loop_init(&loop);
    if (status < 0) {
        fprintf(stderr, "Unable to initialize the request event loop: %s\n", uv_strerror(status));
        return 1;
    }

    // Set up the socket
    status = uv_pipe_init(&loop, &server_pipe, false);
    if (status < 0) {
        fprintf(stderr, "Unable to initialize the request socket: %s\n", uv_strerror(status));
        return 1;
    }

    status = uv_pipe_bind(&server_pipe, socket_path.c_str());
    if (status < 0) {
        fprintf(stderr, "Unable to bind the socket to %s: %s\n",
            socket_path.c_str(), uv_strerror(status)
        );
        return 1;
    }

    status = uv_listen(reinterpret_cast<uv_stream_t *>(&server_pipe), 32, on_new_conn);
    if (status < 0) {
        fprintf(stderr, "Unable to listen for requests: %s\n", uv_strerror(status));
        return 1;
    }

    // Set up the SIGINT signal handler
    status = uv_signal_init(&loop, &sigint_handler);
    if (status < 0) {
        fprintf(stderr, "Unable to initialize the SIGINT handler: %s\n", uv_strerror(status));
        return 1;
    }

    status = uv_signal_start(&sigint_handler, on_sigint, SIGINT);
    if (status < 0) {
        fprintf(stderr, "Unable to start the SIGINT handler: %s\n", uv_strerror(status));
        return 1;
    }

    // Set up the SIGHUP signal handler
    status = uv_signal_init(&loop, &sighup_handler);
    if (status < 0) {
        fprintf(stderr, "Unable to initialize the SIGHUP handler: %s\n", uv_strerror(status));
        return 1;
    }

    status = uv_signal_start(&sighup_handler, on_sighup, SIGHUP);
    if (status < 0) {
        fprintf(stderr, "Unable to start the SIGHUP handler: %s\n", uv_strerror(status));
        return 1;
    }

    status = uv_run(&loop, UV_RUN_DEFAULT);
    if (status < 0) {
        fprintf(stderr, "Error while listening for requests: %s\n", uv_strerror(status));
        return 1;
    }

    return 0;
}
