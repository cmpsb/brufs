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

#include "xxhash/xxhash.h"

static const brufs::address brufs::rootptr::END = 0;
static const brufs::address brufs::rootptr::DELETED = 1;

// brufs::status brufs::find_root(brufs::brufs &fs, const char *name, brufs::root &target) {
//     assert(name);
//     assert(fs.root_ptr_index);
//     assert(fs.root_ptrs);

//     brufs::ssize status;

//     auto num_buckets_per_cluster = fs.header.cluster_size / sizeof(brufs::rootptr);
//     auto num_bucket_clusters = updiv(fs.header.num_root_buckets, num_buckets_per_cluster);

//     brufs::root root;

//     auto hash = XXH64(name, strlen(name), 0);

//     for (auto i = 0; i < fs.header.num_root_buckets; ++i) {
//         auto cidx = ((hash + i) / num_buckets_per_cluster) % num_bucket_clusters;
//         auto idx = (hash + i) % fs.header.num_root_buckets;

//         // Try to load a root pointer block from disk if it's missing
//         if (!fs.root_ptrs[cidx]) {
//             if (!fs.root_ptr_index[cidx]) {
//                 return brufs::status::E_ROOT_NOT_FOUND;
//             }

//             void *buf = malloc(fs.header.cluster_size);
//             if (!buf) {
//                 return brufs::status:::E_NO_MEM;
//             }

//             status = fs.disk->read(buf, fs.header.cluster_size, fs.root_ptr_index[cidx]);
//             if (status < 0) {
//                 free(buf);
//                 return brufs::status::E_DISK_TRUNCATED;
//             }

//             fs.root_ptrs[cidx] = static_cast<brufs::rootptr *>(buf);
//         }

//         auto &ptr = fs.root_ptrs[cidx][idx];

//         // Handle special pointers
//         switch (ptr.location) {
//             case BRUFS_ROOTPTR_END: return BRUFS_E_ROOT_NOT_FOUND;
//             case BRUFS_ROOTPTR_DELETED: continue;
//             default: break;
//         }

//         // Quick-check the stored hash
//         if (ptr.label_hash != hash) continue;

//         // Read the actual entry and verify the label for real
//         status = brufs::dread(fs.disk, &root, sizeof(brufs::root), ptr.location)
//         if (status < 0) return static_cast<brufs::status>(status);

//         if (strncmp(name, root.label, brufs::MAX_LABEL_LENGTH)) continue;

//         // If it's a match, copy it into the output argument and return success
//         target = root;
//         return brufs::status::OK;
//     }
// }


// brufs::status brufs::insert_root(brufs::brufs &fs, const char *name, const brufs::root &target) {
//     assert(name);
//     assert(fs.root_ptr_index);
//     assert(fs.root_ptrs);

//     brufs::ssize status;

//     auto num_buckets_per_cluster = fs.header.cluster_size / sizeof(brufs::rootptr);
//     auto num_bucket_clusters = updiv(fs.header.num_root_buckets, num_buckets_per_cluster);

//     brufs::root root;

//     auto hash = XXH64(name, strlen(name), 0);

//     for (auto i = 0; i < fs.header.num_root_buckets; ++i) {
//         auto cidx = ((hash + i) / num_buckets_per_cluster) % num_bucket_clusters;
//         auto idx = (hash + i) % fs.header.num_root_buckets;

//         // Try to load a root pointer block from disk if it's missing
//         if (!fs.root_ptrs[cidx]) {
//             if (!fs.root_ptr_index[cidx]) {
//                 return brufs::status::E_ROOT_NOT_FOUND;
//             }

//             void *buf = malloc(fs.header.cluster_size);
//             if (!buf) {
//                 return brufs::status:::E_NO_MEM;
//             }

//             status = fs.disk->read(buf, fs.header.cluster_size, fs.root_ptr_index[cidx]);
//             if (status < 0) {
//                 free(buf);
//                 return brufs::status::E_DISK_TRUNCATED;
//             }

//             fs.root_ptrs[cidx] = static_cast<brufs::rootptr *>(buf);
//         }

//         auto &ptr = fs.root_ptrs[cidx][idx];

//         // Handle special pointers
//         switch (ptr.location) {
//             case BRUFS_ROOTPTR_END: return BRUFS_E_ROOT_NOT_FOUND;
//             case BRUFS_ROOTPTR_DELETED: continue;
//             default: break;
//         }

//         // Quick-check the stored hash
//         if (ptr.label_hash != hash) continue;

//         // Read the actual entry and verify the label for real
//         status = brufs::dread(fs.disk, &root, sizeof(brufs::root), ptr.location)
//         if (status < 0) return static_cast<brufs::status>(status);

//         if (strncmp(name, root.label, brufs::MAX_LABEL_LENGTH)) continue;

//         // If it's a match, copy it into the output argument and return success
//         target = root;
//         return brufs::status::OK;
//     }
// }
