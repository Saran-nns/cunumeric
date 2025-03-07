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

from __future__ import division

import cunumeric as np


def test():
    word_size = 10
    hidden_size = 10
    sentence_length = 2
    batch_size = 3
    X = np.random.randn(sentence_length, batch_size, hidden_size)
    h0 = np.random.randn(1, hidden_size)
    WLSTM = np.random.randn(
        word_size + hidden_size, 4 * hidden_size
    ) / np.sqrt(word_size + hidden_size)

    xphpb = WLSTM.shape[0]
    d = hidden_size
    n = sentence_length
    b = batch_size

    Hin = np.zeros((n, b, xphpb))
    Hout = np.zeros((n, b, d))
    IFOG = np.zeros((n, b, d * 4))
    IFOGf = np.zeros((n, b, d * 4))
    C = np.zeros((n, b, d))
    Ct = np.zeros((n, b, d))

    for t in range(0, n):
        if t == 0:
            prev = np.tile(h0, (b, 1))
        else:
            prev = Hout[t - 1]

        Hin[t, :, :word_size] = X[t]
        Hin[t, :, word_size:] = prev
        # compute all gate activations. dots:
        IFOG[t] = Hin[t].dot(WLSTM)
        # non-linearities
        IFOGf[t, :, : 3 * d] = 1.0 / (
            1.0 + np.exp(-IFOG[t, :, : 3 * d])
        )  # sigmoids these are the gates
        IFOGf[t, :, 3 * d :] = np.tanh(IFOG[t, :, 3 * d :])  # tanh
        # compute the cell activation
        C[t] = IFOGf[t, :, :d] * IFOGf[t, :, 3 * d :]
        if t > 0:
            C[t] += IFOGf[t, :, d : 2 * d] * C[t - 1]
        Ct[t] = np.tanh(C[t])
        Hout[t] = IFOGf[t, :, 2 * d : 3 * d] * Ct[t]

    return


if __name__ == "__main__":
    test()
