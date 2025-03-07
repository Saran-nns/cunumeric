# Copyright 2021 NVIDIA Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from itertools import permutations, product

import numpy as np

from legate.core import LEGATE_MAX_DIM


def scalar_gen(lib, val):
    """
    Generates different kinds of scalar-like arrays that contain the given
    value.
    """
    # pure scalar values
    yield lib.array(val)
    # ()-shape arrays
    yield lib.full((), val)
    for ndim in range(1, LEGATE_MAX_DIM + 1):
        # singleton arrays
        yield lib.full(ndim * (1,), val)
        # singleton slices of larger arrays
        yield lib.full(ndim * (5,), val)[ndim * (slice(1, 2),)]


def mk_0to1_array(lib, shape):
    """
    Constructs an array of the required shape, containing (in C order)
    sequential real values uniformly spaced in the range (0,1].
    """
    size = np.prod(shape)
    if size == 1:
        # Avoid zeros, since those are more likely to cause arithmetic issues
        # or produce degenerate outputs.
        return lib.full(shape, 0.5)
    return mk_seq_array(lib, shape) / size


def mk_seq_array(lib, shape):
    """
    Constructs an array of the required shape, containing (in C order)
    sequential integer values starting from 1.
    """
    arr = lib.zeros(shape, dtype=int)
    size = np.prod(shape)
    # Don't return the reshaped array directly, instead use it to update
    # the contents of an existing array of the same shape, thus producing a
    # Store without transformations, that has been tiled in the natural way
    arr[:] = lib.arange(1, size + 1).reshape(shape)
    return arr


def broadcasts_to(lib, tgt_shape, mk_array=mk_0to1_array):
    """
    Generates a collection of arrays that will broadcast to the given shape.
    """
    past_first = False
    for mask in product([True, False], repeat=len(tgt_shape)):
        if not past_first:
            past_first = True
            continue
        src_shape = tuple(
            d if keep else 1 for (d, keep) in zip(tgt_shape, mask)
        )
        yield mk_array(lib, src_shape)


def permutes_to(lib, tgt_shape, mk_array=mk_0to1_array):
    """
    Generates all the possible ways that an array can be transposed to meet a
    given shape.

    All arrays returned will have the same shape, `tgt_shape`, but are the
    result of tranposing a base array of a different shape.
    """
    past_first = False
    for axes in permutations(range(len(tgt_shape))):
        if not past_first:
            past_first = True
            continue
        src_shape = [-1] * len(tgt_shape)
        for (i, j) in enumerate(axes):
            src_shape[j] = tgt_shape[i]
        src_shape = tuple(src_shape)
        yield mk_array(lib, src_shape).transpose(axes)
