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

#include <uv.h>

#include "types.hpp"
#include "Message.hpp"

namespace Brufuse {

/**
 * Reads a message from a stream.
 *
 * The message will be heap-allocated.
 *
 * @param stream the stream to read the message from
 * @param cb the function to call once the full message has been read
 *
 * @return the libuv status code
 */
int read_message(uv_stream_t *stream, OnFullMessage cb);

/**
 * Reads a message from a pipe.
 *
 * The message will be heap-allocated.
 *
 * @param ppe the pipe to read the message from
 * @param cb the function to call once the full message has been read
 *
 * @return the libuv status code
 */
static inline int read_message(uv_pipe_t *ppe, OnFullMessage cb) {
    return read_message(reinterpret_cast<uv_stream_t *>(ppe), cb);
}

/**
 * Writes a message out to the given stream.
 *
 * If an error occurs while writing, the stream will be closed and eventually freed.
 *
 * @param stream the stream to write the message to
 * @param message the message to write
 * @param cb the function to call once the message has been written
 *
 * @return the libuv status code
 */
int write_message(uv_stream_t *stream, Message *message, OnMessageWritten cb);

}
