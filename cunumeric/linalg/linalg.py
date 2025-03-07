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

import numpy as np
from cunumeric.array import ndarray
from cunumeric.module import sqrt as _sqrt


def cholesky(a):
    lg_array = ndarray.convert_to_cunumeric_ndarray(a)
    shape = lg_array.shape
    if len(shape) < 2:
        raise ValueError(
            f"{len(shape)}-dimensional array given. "
            "Array must be at least two-dimensional"
        )
    elif shape[-1] != shape[-2]:
        raise ValueError("Last 2 dimensions of the array must be square")

    if len(shape) > 2:
        raise NotImplementedError(
            "cuNumeric needs to support stacked 2d arrays"
        )
    return lg_array.cholesky(stacklevel=2)


def norm(x, ord=None, axis=None, keepdims=False, stacklevel=1):
    lg_array = ndarray.convert_to_cunumeric_ndarray(x)
    if (axis is None and lg_array.ndim == 1) or type(axis) == int:
        # Handle the weird norm cases
        if ord == np.inf:
            return abs(lg_array).max(
                axis=axis, keepdims=keepdims, stacklevel=(stacklevel + 1)
            )
        elif ord == -np.inf:
            return abs(lg_array).min(
                axis=axis, keepdims=keepdims, stacklevel=(stacklevel + 1)
            )
        elif ord == 0:
            # Check for where things are not zero and convert to integer
            # for sum
            temp = (lg_array != 0).astype(np.int64)
            return temp.sum(
                axis=axis, keepdims=keepdims, stacklevel=(stacklevel + 1)
            )
        elif ord == 1:
            return abs(lg_array).sum(
                axis=axis, keepdims=keepdims, stacklevel=stacklevel + 1
            )
        elif ord is None or ord == 2:
            s = (lg_array.conj() * lg_array).real
            return _sqrt(
                s.sum(axis=axis, keepdims=keepdims, stacklevel=stacklevel + 1)
            )
        elif isinstance(ord, str):
            raise ValueError(f"Invalid norm order '{ord}' for vectors")
        elif type(ord) == int:
            absx = abs(lg_array)
            absx **= ord
            ret = absx.sum(
                axis=axis, keepdims=keepdims, stacklevel=(stacklevel + 1)
            )
            ret **= 1 / ord
            return ret
        else:
            raise ValueError("Invalid 'ord' argument passed to norm")
    else:
        raise NotImplementedError(
            "cuNumeric needs support for other kinds of norms"
        )
