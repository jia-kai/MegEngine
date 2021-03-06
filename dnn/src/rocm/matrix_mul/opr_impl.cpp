/**
 * \file dnn/src/rocm/matrix_mul/opr_impl.cpp
 * MegEngine is Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Copyright (c) 2014-2021 Megvii Inc. All rights reserved.
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT ARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "hcc_detail/hcc_defs_prologue.h"
#include "src/rocm/matrix_mul/opr_impl.h"

#include "src/rocm/utils.h"
#include "src/rocm/handle.h"
#include "./algos.h"
#include "src/common/algo_chooser.h"

using namespace megdnn;
using namespace rocm;

std::vector<MatrixMulForwardImpl::Algorithm*>
MatrixMulForwardImpl::get_all_algorithms(const TensorLayout& A,
                                         const TensorLayout& B,
                                         const TensorLayout& C) {
    AlgoBase::SizeArgs args{this, A, B, C};
    return megdnn::get_all_algorithms<MatrixMulForwardImpl>(args);
}

MatrixMulForwardImpl::Algorithm* MatrixMulForwardImpl::get_algorithm_heuristic(
        const TensorLayout& A, const TensorLayout& B, const TensorLayout& C,
        size_t workspace_limit_in_bytes, bool reproducible) {
    AlgoBase::SizeArgs args{this, A, B, C};
    if (sm_algo_pack.blas.is_available_reproducible(
                args, reproducible, workspace_limit_in_bytes)) {
        return &sm_algo_pack.blas;
    }
    if (reproducible) {
        return megdnn::get_reproducible_algo<MatrixMulForwardImpl>(
                sm_algo_pack.all_algos, args, workspace_limit_in_bytes,
                "matrix mul forward");
    } else {
        return megdnn::get_usable_algo<MatrixMulForwardImpl>(
                sm_algo_pack.all_algos, args, workspace_limit_in_bytes,
                "matrix mul forward");
    }
}

size_t MatrixMulForwardImpl::get_workspace_in_bytes(const TensorLayout& A,
                                                    const TensorLayout& B,
                                                    const TensorLayout& C) {
    AlgoBase::SizeArgs args{this, A, B, C};
    return megdnn::get_algorithm(this, A, B, C)->get_workspace_in_bytes(args);
}

void MatrixMulForwardImpl::exec(_megdnn_tensor_in A, _megdnn_tensor_in B,
                                _megdnn_tensor_out C,
                                _megdnn_workspace workspace) {
    check_exec(A.layout, B.layout, C.layout, workspace.size);
    AlgoBase::ExecArgs args(this, A, B, C, workspace);
    auto&& algo = get_algorithm(this, A.layout, B.layout, C.layout);
    algo->check_workspace(args, workspace).exec(args);
}

// vim: syntax=cpp.doxygen
