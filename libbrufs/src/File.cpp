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

#include "internal.hpp"
#include "File.hpp"
#include "Vector.hpp"

Brufs::Status Brufs::File::destroy() {
    return this->truncate(0);
}

Brufs::Size Brufs::Inode::get_data_size() const {
    const auto root_header = this->root.get_header();

    const auto inode_size = root_header.inode_size;
    const auto inode_header_size = root_header.inode_header_size;
    return inode_size - inode_header_size;
}

Brufs::Status Brufs::File::truncate(Size new_size) {
    const auto old_size = this->get_size();
    if (old_size == new_size) return Status::OK;

    const auto inode_data_size = this->get_data_size();

    const auto old_is_big = old_size > inode_data_size;
    const auto new_is_big = new_size > inode_data_size;

    if (!old_is_big && !new_is_big) return this->resize_small_to_small(old_size, new_size);
    if (!old_is_big &&  new_is_big) return this->resize_small_to_big  (old_size, new_size);
    if ( old_is_big && !new_is_big) return this->resize_big_to_small  (old_size, new_size);
    if ( old_is_big &&  new_is_big) return this->resize_big_to_big    (old_size, new_size);

    // All options exhausted, only reachable through memory corruption probably
    assert(false);
}

Brufs::Status Brufs::File::resize_small_to_small(UNUSED const Size old_size, const Size new_size) {
    memset(this->get_data() + new_size, 0, this->get_data_size() - new_size);

    this->set_size(new_size);
    return this->store();
}

Brufs::Status Brufs::File::resize_big_to_small(UNUSED const Size old_size, const Size new_size) {
    Vector<uint8_t> buf(new_size);

    for (Size copied = 0; copied < new_size;) {
        SSize read = this->read(buf.data() + copied, new_size - copied, copied);
        if (read < Status::OK) return static_cast<Status>(read);

        copied += read;
    }

    InodeExtentTree iet(*this);
    Status status = iet.destroy();
    if (status < Status::OK) return status;

    memcpy(this->get_data(), buf.data(), new_size);
    this->set_size(new_size);

    return this->store();
}

Brufs::Status Brufs::File::resize_small_to_big(const Size old_size, const Size new_size) {
    Vector<uint8_t> buf(BLOCK_SIZE);
    memcpy(buf.data(), this->get_data(), old_size);
    memset(buf.data() + old_size, 0, BLOCK_SIZE - old_size);

    InodeExtentTree iet(*this);
    auto status = iet.init();
    if (status < Status::OK) return status;

    auto fs = this->get_root().get_fs();

    Extent block_extent;
    status = fs.allocate_blocks(BLOCK_SIZE, block_extent);
    if (status < Status::OK) return status;

    auto sstatus = dwrite(fs.get_disk(), buf.data(), BLOCK_SIZE, block_extent.offset);
    if (sstatus < Status::OK) return static_cast<Status>(sstatus);

    this->set_size(new_size);

    status = this->store();
    if (status < Status::OK) return status;

    DataExtent data_extent(block_extent, 0);
    return iet.insert(data_extent.get_local_last(), data_extent);
}

Brufs::Status Brufs::File::resize_big_to_big(const Size old_size, const Size new_size) {
    if (new_size > old_size) {
        this->set_size(new_size);
        return this->store();
    }

    auto fs = this->get_root().get_fs();
    InodeExtentTree iet(*this);

    for (auto ptr = old_size; ptr < new_size;) {
        DataExtent cur_extent;
        auto status = iet.search(ptr, cur_extent);
        if (status == Status::E_NOT_FOUND) break;
        if (status < Status::OK) return status;

        if (cur_extent.local_start < old_size) {
            ptr += cur_extent.get_local_end();
            continue;
        }

        status = iet.remove(ptr, cur_extent);
        if (status < Status::OK) return status;

        status = fs.free_blocks(cur_extent);
        if (status < Status::OK) return status;

        ptr += cur_extent.get_local_end();
    }

    this->set_size(new_size);

    return this->store();
}

Brufs::SSize Brufs::File::write(const void *buf, Size count, Offset offset) {
    if (count == 0) return 0;

    if (offset + count > this->get_size()) {
        auto status = this->truncate(count + offset);
        if (status < Status::OK) return static_cast<SSize>(status);
    }

    if (this->get_size() <= this->get_data_size()) {
        memcpy(this->get_data() + offset, buf, count);

        auto status = this->store();
        if (status < Status::OK) return static_cast<SSize>(status);

        return count;
    }

    InodeExtentTree iet(*this);

    auto fs = this->get_root().get_fs();

    DataExtent data_extent;
    auto status = iet.search(offset, data_extent);

    const auto offset_fits = status != Status::E_NOT_FOUND;
    if (!offset_fits) status = iet.get_last(data_extent);
    const auto extent_present = status != Status::E_NOT_FOUND;

    if (!offset_fits && extent_present && data_extent.length == BLOCK_SIZE) {
        // Resize the last block to a full cluster
        status = iet.remove(data_extent.get_local_last(), data_extent);
        if (status < Status::OK) return status;

        Vector<uint8_t> small_buf(BLOCK_SIZE);
        auto sstatus = dread(fs.get_disk(), small_buf.data(), BLOCK_SIZE, data_extent.offset);
        if (sstatus < 0) return sstatus;

        status = fs.free_blocks(data_extent);
        if (status < Status::OK) return status;

        Extent new_raw_extent;
        status = fs.allocate_blocks(fs.get_header().cluster_size, new_raw_extent);
        if (status < Status::OK) return status;

        DataExtent new_extent(new_raw_extent, data_extent.local_start);
        sstatus = dwrite(fs.get_disk(), small_buf.data(), BLOCK_SIZE, new_extent.offset);
        if (sstatus < 0) return sstatus;

        status = iet.insert(new_extent.get_local_last(), new_extent);
        if (status < Status::OK) return status;

        data_extent = new_extent;
    }

    if (extent_present && data_extent.contains_local(offset)) {
        // The data fits in the extent
        const auto relative_offset = data_extent.relativize_local(offset);
        const auto true_end = min(offset + count, data_extent.get_local_end());
        const auto length = true_end - offset;

        return dwrite(fs.get_disk(), buf, length, data_extent.offset + relative_offset);
    }

    // The data goes before or after the extent
    const auto cluster_size = fs.get_header().cluster_size;
    const auto max_extent_length = this->get_root().get_header().max_extent_length;
    const auto aligned_end = next_multiple_of<Offset>(offset + count, cluster_size);
    const auto aligned_offset = previous_multiple_of<Offset>(offset, cluster_size);
    const auto aligned_length = min<Size>(aligned_end - aligned_offset, max_extent_length);

    Extent raw_new_extent;
    status = fs.allocate_blocks(aligned_length, raw_new_extent);
    if (status < Status::OK) return status;

    const auto local_offset = offset - aligned_offset;

    DataExtent new_extent(raw_new_extent, aligned_offset);
    auto sstatus = dwrite(fs.get_disk(), buf, count, new_extent.offset + local_offset);
    if (sstatus < 0) return sstatus;

    status = iet.insert(new_extent.get_local_last(), new_extent);
    if (status < Status::OK) return status;

    return sstatus;
}

Brufs::SSize Brufs::File::read(void *vbuf, const Size count, const Offset offset) {
    if (!vbuf) return Status::E_INVALID_ARGUMENT;
    if (offset > this->get_size()) return Status::E_BEYOND_EOF;

    const auto end = min<Size>(this->get_size(), offset + count);
    const auto true_count = end - offset;

    if (true_count == 0) return 0;

    const auto root_header = this->get_root().get_header();

    const auto inode_size = root_header.inode_size;
    const auto inode_header_size = root_header.inode_header_size;
    const Size inode_data_size = inode_size - inode_header_size;

    if (this->get_size() <= inode_data_size) {
        memcpy(vbuf, this->get_data() + offset, true_count);
        return true_count;
    }

    const auto iet_address = this->iet_address();
    if (iet_address == 0) {
        memset(vbuf, 0, true_count);
        return true_count;
    }

    InodeExtentTree iet(*this);
    DataExtent extent;
    const auto status = iet.search(offset, extent);

    if (status == Status::E_NOT_FOUND) {
        memset(vbuf, 0, true_count);
        return true_count;
    }

    if (status < Status::OK) return status;

    if (offset < extent.local_start) {
        const auto zero_count = extent.local_start - offset;
        const auto read_count = min(true_count, zero_count);

        memset(vbuf, 0, read_count);
        return read_count;
    }

    auto fs = this->get_root().get_fs();
    const auto local_offset = extent.relativize_local(offset);
    const auto read_count = min(true_count, extent.length - local_offset);
    return dread(fs.get_disk(), vbuf, read_count, extent.offset + local_offset);
}
