/*
 * Copyright (c) 2017-2018 Luc Everse <luc@cmpsb.net>
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
#include <cstdint>
#include <string>

template <typename N>
N prompt_number(const std::string &qry, const std::string &def, const char *format) {
    for (;;) {
        fprintf(stderr, "%s? ", qry.c_str());

        if (def.size() > 0) {
            fprintf(stderr, "[%s] ", def.c_str());
        }

        fprintf(stderr, "> ");

        N value;
        int num_read = scanf(format, &value);
        if (num_read == 0) continue;
        if (num_read == EOF) {
            if (def.size() == 0) continue;
            sscanf(def.c_str(), format, &value);
        }

        return value;
    }
}

std::string prompt_string(const std::string &qry, const std::string &def, size_t max_len) {
    for (;;) {
        fprintf(stderr, "%s? ", qry.c_str());

        if (def.size() > 0) {
            fprintf(stderr, "[%s] ", def.c_str());
        }

        fprintf(stderr, "> ");

        auto buf = new char[max_len + 2];
        auto res = fgets(buf, max_len + 2, stdin);
        if (res == nullptr) continue;

        auto ret = std::string(buf);
        ret.pop_back();

        delete[] buf;

        return ret;
    }
}
