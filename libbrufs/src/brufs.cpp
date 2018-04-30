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

#include <cstring>
#include <cassert>

#include "xxhash/xxhash.h"

#include "internal.hpp"

static constexpr unsigned long MEGABYTE = 1024 * 1024;
static constexpr int MAX_COLLISIONS = 16;

struct pl {
    brufs::root_header *coll;
    brufs::size count;
};

brufs::brufs::brufs(disk *dsk) : 
        dsk(dsk), fbt(this, nullptr, bmtree::ALLOC_FBT_BLOCK), rht(this, nullptr)
{
    header temp_header;

    this->stt = static_cast<status>(dread(dsk, &temp_header, sizeof(temp_header), 0));
    if (this->stt < 0) return;

    this->stt = static_cast<status>(temp_header.validate(dsk));
    if (this->stt < 0) return;

    this->raw_header = static_cast<char *>(malloc(temp_header.cluster_size));
    if (this->raw_header == nullptr) {
        this->stt = status::E_NO_MEM;
        return;
    }

    memcpy(this->raw_header, &temp_header, sizeof(header));

    this->fbt.set_target(&this->hdr->fbt_address);
    this->fbt.update_root(this->hdr->fbt_address, this->hdr->cluster_size);

    this->rht.set_target(&this->hdr->rht_address);
    this->rht.update_root(this->hdr->rht_address, this->hdr->cluster_size);

    this->stt = status::OK;
}

brufs::status brufs::brufs::store_header() {
    this->hdr->checksum = 0;
    this->hdr->checksum = XXH64(this->hdr, this->hdr->header_size, CHECKSUM_SEED);

    ssize sstt = dwrite(this->dsk, this->raw_header, this->hdr->cluster_size, 0);
    if (sstt < 0) return static_cast<status>(sstt);

    return status::OK;
}

/*
 * Initialization
 */

brufs::status brufs::brufs::init(header &protoheader) {
    this->raw_header = static_cast<char *>(malloc(1 << protoheader.cluster_size_exp));
    assert(this->raw_header);
    memset(this->raw_header, 0, (1 << protoheader.cluster_size_exp));

    memcpy(this->hdr->magic, MAGIC_STRING, MAGIC_STRING_LENGTH);

    auto lib_version = version::get();

    this->hdr->ver = lib_version;
    this->hdr->header_size = sizeof(header);
    this->hdr->checksum = 0;
    
    this->hdr->cluster_size = (1 << protoheader.cluster_size_exp);
    this->hdr->cluster_size_exp = protoheader.cluster_size_exp;

    this->hdr->num_blocks = this->dsk->io->get_size();

    this->hdr->sc_low_mark = protoheader.sc_low_mark;
    this->hdr->sc_high_mark = protoheader.sc_high_mark;
    this->hdr->sc_count = 0;

    // Provide some free extents for the FBT
    extent *spares = this->get_spare_clusters();
    for (int i = 0; i < this->hdr->sc_high_mark; ++i) {
        spares[i] = {(i + 1) * this->hdr->cluster_size, this->hdr->cluster_size};
    }
    this->hdr->sc_count = this->hdr->sc_high_mark;

    // Initialize the FBT with 1-megabyte blocks
    address dyn_start = (this->hdr->sc_high_mark + 1) * this->hdr->cluster_size;
    size remaining = this->dsk->io->get_size() - dyn_start;

    this->fbt.set_target(&this->hdr->fbt_address);
    status stt = this->fbt.init(this->hdr->cluster_size);
    if (stt < status::OK) return stt;

    while (remaining > MEGABYTE) {
        extent free_ext{dyn_start, MEGABYTE};

        this->fbt.insert(MEGABYTE, free_ext);
        
        dyn_start += MEGABYTE;
        remaining -= MEGABYTE;
    }

    if (remaining > 0) {
        extent free_ext{dyn_start, remaining};
        this->fbt.insert(MEGABYTE, free_ext);
    }

    // Initialize the RHT
    this->rht.set_target(&this->hdr->rht_address);
    stt = this->rht.init(this->hdr->cluster_size);
    if (stt < status::OK) return stt;

    // Store the header
    return this->store_header();
}

/*
 * Free cluster management
 */

brufs::status brufs::brufs::allocate_blocks(size length, extent &target) {
    extent result;
    status status = this->fbt.remove(length, result);
    if (status < 0) return status;

    if (result.length > length) {
        extent residual {result.offset + length, result.length - length};
        status = this->fbt.insert(result.length, residual);
        if (status  < 0) return status;
    }

    target = {result.offset, length};

    auto list = this->get_spare_clusters();
    while (this->hdr->sc_count < this->hdr->sc_low_mark) {
        extent replacement;
        status = this->fbt.remove(length, replacement);
        if (status != status::OK) break;

        list[this->hdr->sc_count] = replacement;
    }

    return status::OK;
}

brufs::status brufs::brufs::allocate_tree_blocks(UNUSED size length, extent &target) {
    if (this->hdr->sc_count == 0) return status::E_NO_SPACE;
    --this->hdr->sc_count;

    auto list = this->get_spare_clusters();
    target = list[this->hdr->sc_count];

    return status::OK;
}

brufs::status brufs::brufs::free_blocks(const extent &ext) {
    auto fbt_block_size = this->hdr->cluster_size;

    if (this->hdr->sc_count < this->hdr->sc_high_mark && ext.length >= fbt_block_size) {
        auto list = this->get_spare_clusters();

        list[this->hdr->sc_count] = {ext.offset, fbt_block_size};

        extent residual {ext.offset + fbt_block_size, ext.length - fbt_block_size};
        if (residual.length > 0) {
            return this->fbt.insert(residual.length, residual);
        }

        return status::OK;
    }

    return this->fbt.insert(ext.length, ext);
}

/*
 * Roots
 */

static brufs::status consume_root(brufs::root_header &r, pl &p) {
    *p.coll = r;
    --p.count;
    ++p.coll;

    if (p.count == 0) return brufs::status::STOP;
    return brufs::status::OK;
}

brufs::ssize brufs::brufs::count_roots() {
    size count;
    status stt = this->rht.count_values(count);

    if (stt < 0) return static_cast<ssize>(stt);

    return static_cast<ssize>(count);
}

int brufs::brufs::collect_roots(root_header *coll, size count) {
    pl payload{coll, count};

    status stt = this->rht.walk<pl>(consume_root, payload);
    if (stt < status::OK) return static_cast<status>(stt);

    return static_cast<int>(count - payload.count);
}

brufs::status brufs::brufs::find_root(const char *name, root_header &target) {
    assert(name);

    hash hash = XXH64(name, strlen(name), CHECKSUM_SEED);

    root_header roots[MAX_COLLISIONS];
    int num = this->rht.search(hash, roots, MAX_COLLISIONS, true);
    if (num < 0) return static_cast<status>(num);

    for (int i = 0; i < num; ++i) {
        roots[i].label[MAX_LABEL_LENGTH - 1] = 0;

        if (strcmp(name, roots[i].label) != 0) continue;

        target = roots[i];

        return status::OK;
    }

    return status::E_NOT_FOUND;
}

brufs::status brufs::brufs::add_root(root_header &rt) {
    rt.label[MAX_LABEL_LENGTH - 1] = 0;

    root_header dummy;
    status stt = this->find_root(rt.label, dummy);
    if (stt != status::E_NOT_FOUND) return status::E_EXISTS;

    hash hash = XXH64(rt.label, strlen(rt.label), CHECKSUM_SEED);

    return this->rht.insert(hash, rt);
}
