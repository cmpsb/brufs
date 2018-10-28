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

#include "message-io.hpp"

struct MessageContainer {
    Brufuse::Message *message;
    Brufuse::OnFullMessage cb;
};

static void on_close(uv_handle_t *handle) {
    delete handle;
}

int Brufuse::write_message(uv_stream_t *stream, Message *msg, OnMessageWritten cb) {
    stream->data = static_cast<void *>(msg);

    uv_buf_t buf;
    buf.base = const_cast<char *>(reinterpret_cast<const char *>(msg->get_buffer()));
    buf.len = msg->get_size();

    uv_write_t *whandle = new uv_write_t;
    int status = uv_write(whandle, stream, &buf, 1, cb);
    if (status < 0) {
        fprintf(stderr, "Unable to write message %u: %s\n",
            msg->get_sequence(), uv_strerror(status)
        );

        delete whandle;

        uv_close(reinterpret_cast<uv_handle_t *>(stream), on_close);
    }

    return status;
}

/**
 * Allocates memory to be used by libuv while reading.
 */
static void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void) handle;

    buf->base = new char[suggested_size]{0};
    buf->len = suggested_size;
}

/**
 * Called by libuv as soon as some bytes have been read.
 */
void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    auto ct = reinterpret_cast<MessageContainer *>(stream->data);
    auto req = ct->message;

    if (nread == 0) return;
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "An error occurred while reading a message: %s\n", uv_strerror(nread));
        }

        uv_read_stop(stream);
        uv_close(reinterpret_cast<uv_handle_t *>(stream), on_close);

        delete[] buf->base;
        delete ct->message;
        delete ct;

        return;
    }

    auto cbuf = reinterpret_cast<unsigned char *>(buf->base);
    ssize_t i = 0;

    while (!req->is_size_read() && i < nread) req->add_next_size_byte(cbuf[i++]);

    auto is_full = req->fill(cbuf + i, nread - i);
    if (is_full) {
        uv_read_stop(stream);

        delete[] buf->base;

        ct->cb(stream, req);

        delete ct;
    }
}

int Brufuse::read_message(uv_stream_t *stream, OnFullMessage cb) {
    auto ct = new MessageContainer { new Message, cb };
    stream->data = static_cast<void *>(ct);

    auto status = uv_read_start(stream, on_alloc, on_read);
    if (status < 0) {
        delete ct->message;
        delete ct;
    }

    return status;
}
