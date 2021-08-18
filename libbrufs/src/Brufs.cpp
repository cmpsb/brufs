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

#include <string.h>
#include <assert.h>

#include <unistd.h>

#include "xxhash/xxhash.h"

#include "internal.hpp"
#include "Brufs.hpp"
#include "File.hpp"
#include "Directory.hpp"

static constexpr unsigned long MEGABYTE = 1024 * 1024;
static constexpr unsigned long INITIAL_FREE_EXTENT_LENGTH = 2 * MEGABYTE;

struct pl {
    Brufs::RootHeader *coll;
    Brufs::Size count;
};

namespace Brufs { namespace BmTree {

template <>
bool equiv_values(const RootHeader *current, const RootHeader *replacement) {
    return strncmp(current->label, replacement->label, MAX_LABEL_LENGTH) == 0;
}

}}

Brufs::Brufs::Brufs(Disk *dsk) :
        dsk(dsk), raw_header(nullptr),
        fbt(this, nullptr, BmTree::ALLOC_FBT_BLOCK), rht(this, nullptr)
{
    Header temp_header;
    memset(&temp_header, 0, sizeof(temp_header));

    this->stt = static_cast<Status>(dread(dsk, &temp_header, sizeof(temp_header), 0));
    if (this->stt < 0) return;

    this->stt = static_cast<Status>(temp_header.validate(dsk));
    if (this->stt < 0) return;

    this->raw_header = static_cast<char *>(malloc(temp_header.cluster_size));
    if (this->raw_header == nullptr) {
        this->stt = Status::E_NO_MEM;
        return;
    }

    this->stt = static_cast<Status>(dread(dsk, this->raw_header, temp_header.cluster_size, 0));
    if (this->stt < 0) return;

    this->fbt.set_target(&this->hdr->fbt_address);
    this->stt = this->fbt.update_root(this->hdr->fbt_address, this->hdr->cluster_size);
    if (this->stt < Status::OK) return;

    this->rht.set_target(&this->hdr->rht_address);
    this->stt = this->rht.update_root(this->hdr->rht_address, this->hdr->cluster_size);
    if (this->stt < Status::OK) return;

    this->stt = Status::OK;
}

Brufs::Brufs::~Brufs() {
    free(this->raw_header);
}

Brufs::Status Brufs::Brufs::store_header() {
    this->hdr->checksum = 0;
    this->hdr->checksum = XXH64(this->hdr, this->hdr->header_size, CHECKSUM_SEED);

    SSize sstt = dwrite(this->dsk, this->raw_header, this->hdr->cluster_size, 0);
    if (sstt < 0) return static_cast<Status>(sstt);

    return Status::OK;
}

/*
 * Initialization
 */

Brufs::Status Brufs::Brufs::init(Header &protoheader) {
    free(this->raw_header);
    this->raw_header = static_cast<char *>(malloc(1 << protoheader.cluster_size_exp));
    assert(this->raw_header);
    memset(this->raw_header, 0, (1 << protoheader.cluster_size_exp));

    memcpy(this->hdr->magic, MAGIC_STRING, MAGIC_STRING_LENGTH);

    auto lib_version = Version::get();

    this->hdr->ver = lib_version;
    this->hdr->header_size = sizeof(Header);
    this->hdr->checksum = 0;

    this->hdr->cluster_size = (1 << protoheader.cluster_size_exp);
    this->hdr->cluster_size_exp = protoheader.cluster_size_exp;

    this->hdr->num_blocks = this->dsk->io->get_size();

    this->hdr->sc_low_mark = protoheader.sc_low_mark;
    this->hdr->sc_high_mark = protoheader.sc_high_mark;
    this->hdr->sc_count = 0;

    // Provide some free extents for the FBT
    Extent *spares = this->get_spare_clusters();
    for (int i = 0; i < this->hdr->sc_high_mark; ++i) {
        spares[i] = {(i + 1) * this->hdr->cluster_size, this->hdr->cluster_size};
    }
    this->hdr->sc_count = this->hdr->sc_high_mark;

    // Initialize the FBT with 1-megabyte blocks
    Address dyn_start = (this->hdr->sc_high_mark + 1) * this->hdr->cluster_size;
    Size remaining = this->dsk->io->get_size() - dyn_start;

    this->fbt.set_target(&this->hdr->fbt_address);
    Status stt = this->fbt.init(this->hdr->cluster_size);
    if (stt < Status::OK) return stt;

    while (remaining > INITIAL_FREE_EXTENT_LENGTH) {
        Extent free_ext{dyn_start, INITIAL_FREE_EXTENT_LENGTH};

        stt = this->fbt.insert(INITIAL_FREE_EXTENT_LENGTH, free_ext);
        if (stt < Status::OK) return stt;

        dyn_start += INITIAL_FREE_EXTENT_LENGTH;
        remaining -= INITIAL_FREE_EXTENT_LENGTH;
    }

    if (remaining > 0) {
        Extent free_ext{dyn_start, remaining};
        stt = this->fbt.insert(INITIAL_FREE_EXTENT_LENGTH, free_ext);
        if (stt < Status::OK) return stt;
    }

    // Initialize the RHT
    this->rht.set_target(&this->hdr->rht_address);
    stt = this->rht.init(this->hdr->cluster_size);
    if (stt < Status::OK) return stt;

    // Store the header
    return this->store_header();
}

/*
 * Free cluster management
 */

Brufs::Status Brufs::Brufs::allocate_blocks(Size length, Extent &target) {
    if (length != BLOCK_SIZE && (length % this->hdr->cluster_size != 0)) {
        return Status::E_MISALIGNED;
    }

    Extent result;
    Status status = this->fbt.remove(length, result);
    if (status == Status::E_NOT_FOUND) return Status::E_WONT_FIT;
    if (status < 0) return status;

    target = {result.offset, length};

    if (result.length > length) {
        Extent residual {result.offset + length, result.length - length};
        status = this->fbt.insert(residual.length, residual);
        if (status < 0) return status;
    }

    auto list = this->get_spare_clusters();
    while (this->hdr->sc_count < this->hdr->sc_low_mark) {
        Extent replacement;
        status = this->fbt.remove(this->hdr->cluster_size, replacement);
        if (status < Status::OK) return status;

        while (
            replacement.length >= this->hdr->cluster_size
            && this->hdr->sc_count < this->hdr->sc_low_mark
        ) {
            list[this->hdr->sc_count] = replacement;
            replacement.offset += this->hdr->cluster_size;
            replacement.length -= this->hdr->cluster_size;

            ++this->hdr->sc_count;
        }

        status = this->store_header();
        if (status < Status::OK) return status;

        if (replacement.length > 0) {
            status = this->fbt.insert(replacement.length, replacement);
            if (status < Status::OK) return status;
        }
    }

    return this->store_header();
}

Brufs::Status Brufs::Brufs::allocate_tree_blocks(UNUSED Size length, Extent &target) {
    if (this->hdr->sc_count == 0) return Status::E_NO_SPACE;
    --this->hdr->sc_count;

    auto list = this->get_spare_clusters();
    target = list[this->hdr->sc_count];

    return this->store_header();
}

Brufs::Status Brufs::Brufs::free_blocks(const Extent &ext) {
    auto fbt_block_size = this->hdr->cluster_size;

    if (this->hdr->sc_count < this->hdr->sc_high_mark && ext.length >= fbt_block_size) {
        auto list = this->get_spare_clusters();

        list[this->hdr->sc_count++] = {ext.offset, fbt_block_size};

        auto status = this->store_header();
        if (status < Status::OK) return status;

        Extent residual {ext.offset + fbt_block_size, ext.length - fbt_block_size};
        if (ext.length > fbt_block_size) {
            auto status = this->fbt.insert(residual.length, residual);
            if (status < Status::OK) return status;
        }

        return Status::OK;
    }

    return this->fbt.insert(ext.length, ext);
}

Brufs::Status Brufs::Brufs::count_free_blocks(
    Size &standby, Size &available, Size &extents, Size &in_fbt
) {
    standby = this->hdr->sc_count * this->hdr->cluster_size;

    available = 0;
    in_fbt = 0;
    extents = 0;

    auto status = this->fbt.count_used_space(in_fbt);
    if (status < Status::OK) return status;

    status = this->fbt.count_values(extents);
    if (status < Status::OK) return status;

    return this->fbt.walk<Size *>([](UNUSED auto last, auto ext, auto acc) {
        *acc += ext->length;
        return Status::OK;
    }, &available);
}

/*
 * Roots
 */

static Brufs::Status consume_root(UNUSED Brufs::Hash &hash, Brufs::RootHeader *r, pl &p) {
    *p.coll = *r;
    --p.count;
    ++p.coll;

    if (p.count == 0) return Brufs::Status::STOP;
    return Brufs::Status::OK;
}

Brufs::SSize Brufs::Brufs::count_roots() {
    Size count;
    Status stt = this->rht.count_values(count);

    if (stt < 0) return static_cast<SSize>(stt);

    return static_cast<SSize>(count);
}

int Brufs::Brufs::collect_roots(RootHeader *coll, Size count) {
    pl payload{coll, count};

    Status stt = this->rht.walk<pl &>(consume_root, payload);
    if (stt < Status::OK) return static_cast<Status>(stt);

    return static_cast<int>(count - payload.count);
}

Brufs::Status Brufs::Brufs::find_root(const char *name, RootHeader &target) {
    assert(name);

    char lbl[MAX_LABEL_LENGTH + 1];
    strncpy(lbl, name, MAX_LABEL_LENGTH);
    lbl[MAX_LABEL_LENGTH] = 0;

    const Hash hash = XXH64(name, strlen(lbl), HASH_SEED);

    RootHeader roots[MAX_COLLISIONS];
    int num = this->rht.search(hash, roots, MAX_COLLISIONS, true);
    if (num < 0) return static_cast<Status>(num);

    for (int i = 0; i < num; ++i) {
        char candidate_label[MAX_LABEL_LENGTH + 1];
        memcpy(candidate_label, roots[i].label, MAX_LABEL_LENGTH);
        candidate_label[MAX_LABEL_LENGTH] = 0;

        if (strcmp(lbl, candidate_label) != 0) continue;

        target = roots[i];

        return Status::OK;
    }

    return Status::E_NOT_FOUND;
}

Brufs::Status Brufs::Brufs::add_root(const RootHeader &rt) {
    RootHeader dummy;
    Status stt = this->find_root(rt.label, dummy);
    if (stt == Status::OK) return Status::E_EXISTS;
    if (stt != Status::E_NOT_FOUND) return stt;

    return this->rht.insert(rt.hash(), rt);
}

Brufs::Status Brufs::Brufs::update_root(const RootHeader &rt) {
    return this->rht.update(rt.hash(), rt);
}
