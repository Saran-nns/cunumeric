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
from test_tools import asserts

import cunumeric as num


def test():
    x = num.array([])
    r = num.sum(x)
    assert r == 0

    x = num.array([1])
    r = num.sum(x)

    assert r == 1

    x = num.eye(3)
    r = num.sum(x)

    assert r == 3

    x = num.array([1, 2, 3, 4.0])
    r = num.sum(x)
    r2 = num.add.reduce(x)

    assert r == r2 == 10

    x = num.array([1, 2, 3, 4.0, 5.0])
    r = num.prod(x)
    r2 = num.multiply.reduce(x)
    assert r == r2 == 120

    asserts.assert_equal(num.sum([]), np.sum([]))
    asserts.assert_equal(num.add.reduce([]), np.add.reduce([]))

    asserts.assert_equal(num.sum([[], []]), np.sum([[], []]))
    asserts.assert_equal(num.add.reduce([[], []]), np.add.reduce([[], []]))

    asserts.assert_equal(num.sum(num.array([0])), np.sum(np.array([0])))
    asserts.assert_equal(
        num.add.reduce(num.array([0])), np.add.reduce(np.array([0]))
    )

    asserts.assert_equal(num.sum([1]), np.sum([1]))
    asserts.assert_equal(num.add.reduce([1]), np.add.reduce([1]))

    asserts.assert_equal(num.sum(0), np.sum(0))
    asserts.assert_equal(num.add.reduce(0), np.add.reduce(0))

    asserts.assert_equal(num.sum(1), np.sum(1))
    asserts.assert_equal(num.add.reduce(1), np.add.reduce(1))

    x = num.array([1, 0, 2, -1, 0, 0, 8])
    x_np = np.array([1, 0, 2, -1, 0, 0, 8])
    asserts.assert_equal(num.sum(x), np.sum(x_np))
    asserts.assert_equal(num.add.reduce(x), np.add.reduce(x_np))

    x = num.array([[0, 1, 0], [2, 0, 3]])
    x_np = np.array([[0, 1, 0], [2, 0, 3]])
    asserts.assert_equal(num.sum(x), np.sum(x_np))
    asserts.assert_equal(num.add.reduce(x), np.add.reduce(x_np))

    x = num.eye(3)
    x_np = np.eye(3)
    asserts.assert_equal(num.sum(x), np.sum(x_np))
    asserts.assert_equal(num.add.reduce(x), np.add.reduce(x_np))

    x = num.array(
        [
            [[0, 1], [1, 1], [7, 0], [1, 0], [0, 1]],
            [[3, 0], [0, 3], [0, 0], [2, 2], [0, 19]],
        ]
    )
    x_np = np.array(
        [
            [[0, 1], [1, 1], [7, 0], [1, 0], [0, 1]],
            [[3, 0], [0, 3], [0, 0], [2, 2], [0, 19]],
        ]
    )
    asserts.assert_equal(num.sum(x, axis=0), np.sum(x_np, axis=0))
    asserts.assert_equal(num.sum(x, axis=1), np.sum(x_np, axis=1))
    asserts.assert_equal(num.sum(x, axis=2), np.sum(x_np, axis=2))
    asserts.assert_equal(
        num.add.reduce(x, axis=0), np.add.reduce(x_np, axis=0)
    )
    asserts.assert_equal(
        num.add.reduce(x, axis=1), np.add.reduce(x_np, axis=1)
    )
    asserts.assert_equal(
        num.add.reduce(x, axis=2), np.add.reduce(x_np, axis=2)
    )
    asserts.assert_equal(num.sum(x), np.sum(x_np))
    asserts.assert_equal(num.add.reduce(x), np.add.reduce(x_np))

    x_np = np.concatenate((x_np,) * 2000, axis=1)
    x = num.array(x_np)
    asserts.assert_equal(num.sum(x, axis=0), np.sum(x_np, axis=0))
    asserts.assert_equal(num.sum(x, axis=1), np.sum(x_np, axis=1))
    asserts.assert_equal(num.sum(x, axis=2), np.sum(x_np, axis=2))
    asserts.assert_equal(num.sum(x), np.sum(x_np))
    asserts.assert_equal(
        num.add.reduce(x, axis=0), np.add.reduce(x_np, axis=0)
    )
    asserts.assert_equal(
        num.add.reduce(x, axis=1), np.add.reduce(x_np, axis=1)
    )
    asserts.assert_equal(
        num.add.reduce(x, axis=2), np.add.reduce(x_np, axis=2)
    )
    asserts.assert_equal(num.add.reduce(x), np.add.reduce(x_np))

    x_np = np.random.randn(100)
    indices = np.random.choice(
        np.arange(x_np.size), replace=False, size=int(x_np.size * 0.2)
    )
    x_np[indices] = 0
    x = num.array(x_np)
    asserts.assert_allclose(num.sum(x), np.sum(x_np))
    asserts.assert_allclose(num.add.reduce(x), np.add.reduce(x_np))

    x_np = x_np.reshape(10, 10)
    x = num.array(x_np)
    asserts.assert_allclose(num.sum(x), np.sum(x_np))
    asserts.assert_allclose(num.add.reduce(x), np.add.reduce(x_np))

    return


if __name__ == "__main__":
    test()
