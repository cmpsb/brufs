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

#include <cstdlib>
#include <cstring>
#include <cassert>

#include <algorithm>
#include <vector>

namespace Brufuse {

static constexpr unsigned int SIZE_SIZE = 4;
static constexpr unsigned int SEQUENCE_SIZE = 4;
static constexpr unsigned int REQUEST_TYPE_SIZE = 1;
static constexpr unsigned int STATUS_CODE_SIZE = 1;
static constexpr unsigned int PADDING_SIZE = 6;

static constexpr unsigned int SIZE_INDEX = 0;
static constexpr unsigned int SEQUENCE_INDEX = SIZE_SIZE;
static constexpr unsigned int REQUEST_TYPE_INDEX = SEQUENCE_INDEX + SEQUENCE_SIZE;
static constexpr unsigned int STATUS_CODE_INDEX = REQUEST_TYPE_INDEX + REQUEST_TYPE_SIZE;

static constexpr unsigned int HEADER_SIZE =
        SIZE_SIZE + SEQUENCE_SIZE + REQUEST_TYPE_SIZE + STATUS_CODE_SIZE + PADDING_SIZE;
static_assert(HEADER_SIZE % 8 == 0);

/**
 * The type of request the client wishes to perform.
 */
enum RequestType : char {
    /**
     * Not an actual request type. Marks the start of the types.
     */
    RT_START,

    /**
     * A request to do nothing.
     */
    NONE,

    /**
     * A request to mount a root at a specific mountpoint.
     */
    MOUNT,

    /**
     * A request to check the status of the disk.
     */
    STATUS,

    /**
     * A request to stop the daemon.
     */
    STOP,

    /**
     * Not an actual request type. Marks the end of the types.
     */
    RT_END
};

enum StatusCode : char {
    /**
     * Not an actual status code. Marks the start of the codes.
     */
    SC_START,

    /**
     * The request completed successfully.
     */
    OK,

    /**
     * The client sent a bad request.
     */
    BAD_REQUEST,

    /**
     * The program encountered an internal error.
     */
    INTERNAL_ERROR,

    /**
     * Not an actual status code. Marks the end of the codes.
     */
    SC_END
};

/**
 * A request sent by a client.
 */
class Message {
private:
    /**
     * The number of bytes in the actual message body.
     */
    size_t message_size = 0;

    /**
     * The message body.
     */
    std::vector<unsigned char> data;

    /**
     * The number of data bytes currently present in the buffer.
     */
    size_t data_present = 0;

public:
    static inline Message *create_reply(Message *req);

    Message() {
        this->data.resize(HEADER_SIZE);

        this->set_sequence(0);
        this->set_type(RequestType::NONE);
        this->set_status(StatusCode::OK);
    }

    /**
     * Reads the next size byte into the message's size.
     *
     * The byte ordering is network order, aka big-endian.
     *
     * @param sz the next size byte
     *
     * @return the number of size bytes read so far.
     */
    int add_next_size_byte(unsigned char sz) {
        this->data[this->data_present] = sz;

        this->message_size = (this->message_size << 8) | sz;
        return ++this->data_present;
    }

    /**
     * Checks whether the full size has been read.
     *
     * @return true if the full size is available, false otherwise
     */
    bool is_size_read() {
        return this->data_present >= SIZE_SIZE;
    }

    /**
     * Checks whether the full header has been read.
     *
     * @return true if the full header is available, false otherwise
     */
    bool is_header_present() {
        return this->data_present >= HEADER_SIZE;
    }

    /**
     * Checks whether the message has been completely read.
     *
     * @return true if the full header and data are available, false otherwise
     */
    bool is_complete() {
        return this->is_header_present() && this->data_present == this->message_size;
    }

    void set_data_size(size_t size) {
        this->message_size = size + HEADER_SIZE;
        this->data.resize(this->message_size);

        this->data[SIZE_INDEX + 0] = (unsigned char) (this->message_size >> 24);
        this->data[SIZE_INDEX + 1] = (unsigned char) (this->message_size >> 16);
        this->data[SIZE_INDEX + 2] = (unsigned char) (this->message_size >>  8);
        this->data[SIZE_INDEX + 3] = (unsigned char) (this->message_size >>  0);
    }

    /**
     * Returns the size of the message body.
     *
     * @return the size of the message body
     */
    size_t get_size() const {
        return this->message_size;
    }

    /**
     * Returns the size of the message data, i.e. any data after the message type.
     *
     * @return the size of the message data.
     */
    size_t get_data_size() const {
        return this->message_size - HEADER_SIZE;
    }

    /**
     * Returns the sequence number of the message.
     *
     * @return the sequence number
     */
    unsigned int get_sequence() const {
        return (((unsigned int) this->data[SEQUENCE_INDEX + 0]) << 24)
             | (((unsigned int) this->data[SEQUENCE_INDEX + 1]) << 16)
             | (((unsigned int) this->data[SEQUENCE_INDEX + 2]) <<  8)
             | (((unsigned int) this->data[SEQUENCE_INDEX + 3]) <<  0);
    }

    /**
     * Sets the sequence number of the message.
     *
     * @param seq the sequence number
     */
    void set_sequence(unsigned int seq) {
        this->data[SEQUENCE_INDEX + 0] = (unsigned char) (seq >> 24);
        this->data[SEQUENCE_INDEX + 1] = (unsigned char) (seq >> 16);
        this->data[SEQUENCE_INDEX + 2] = (unsigned char) (seq >>  8);
        this->data[SEQUENCE_INDEX + 3] = (unsigned char) (seq >>  0);
    }

    /**
     * Copies a partial body buffer into the message's body.
     *
     * The number of bytes in the partial body plus the current size read may exceed the declared
     * message size. In this case the trailing bytes will be discarded.
     *
     * @param buf the partial body
     * @param nread the number of bytes available in the partial body
     *
     * @return true if the full message body has been read (this fill completed the body),
     *         false otherwise
     */
    bool fill(unsigned char *buf, ssize_t nread) {
        auto to_copy = std::min(
            this->message_size - this->data_present, static_cast<size_t>(nread)
        );

        memcpy(this->data.data() + this->data_present, buf, to_copy);

        this->data_present += to_copy;

        return this->data_present == this->message_size;
    }

    /**
     * Returns the first byte of the message body: the request type.
     *
     * @return the message type
     */
    RequestType get_type() const {
        return static_cast<RequestType>(this->data[REQUEST_TYPE_INDEX]);
    }

    /**
     * Sets the first byte of the message body: the request type.
     *
     * @param type the new type
     */
    void set_type(RequestType type) {
        assert(type > RequestType::RT_START && type < RequestType::RT_END);
        this->data[REQUEST_TYPE_INDEX] = type;
    }

    /**
     * Returns the second byte of the message body: the status code.
     *
     * @return the status code
     */
    StatusCode get_status() const {
        return static_cast<StatusCode>(this->data[STATUS_CODE_INDEX]);
    }

    /**
     * Sets the second byte of the message body: the status code.
     *
     * @param status the new status code
     */
    void set_status(StatusCode status) {
        assert(status > StatusCode::SC_START && status < StatusCode::SC_END);
        this->data[STATUS_CODE_INDEX] = status;
    }

    /**
     * Returns the actual data of the message past the header.
     *
     * @return the message data
     */
    const unsigned char *get_data() const { return this->data.data() + HEADER_SIZE; }

    /**
     * Returns the actual data of the message past the header.
     *
     * @return the message data
     */
    unsigned char *get_data() { return this->data.data() + HEADER_SIZE; }

    /**
     * Returns the raw buffer, including the header.
     *
     * @return the raw buffer
     */
    const unsigned char *get_buffer() const { return this->data.data(); }
};

Message *Message::create_reply(Message *req) {
    Message *res = new Message;
    res->set_sequence(req->get_sequence() + 1);

    return res;
}

}
