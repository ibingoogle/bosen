// Copyright (c) 2014, Sailing Lab
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the <ORGANIZATION> nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#pragma once
#include <stdint.h>
#include <bitset>

#include <petuum_ps_common/util/record_buff.hpp>
#include <petuum_ps/thread/context.hpp>
#include <glog/logging.h>

#ifndef PETUUM_MAX_NUM_CLIENTS
#define PETUUM_MAX_NUM_CLIENTS 8
#endif

namespace petuum {

class CallBackSubs {
public:
  CallBackSubs() { }
  ~CallBackSubs() { }

  bool Subscribe(int32_t client_id) {
    bool bit_changed = false;
    if (!subscriptions_.test(client_id)) {
      bit_changed = true;
      subscriptions_.set(client_id);
    }
    return bit_changed;
  }

  bool Unsubscribe(int32_t client_id) {
    bool bit_changed = false;
    if (subscriptions_.test(client_id)) {
      bit_changed = true;
      subscriptions_.reset(client_id);
    }
    return bit_changed;
  }

  bool AppendRowToBuffs(int32_t client_id_st,
    boost::unordered_map<int32_t, RecordBuff> *buffs,
    const void *row_data, size_t row_size, int32_t row_id,
    int32_t *failed_bg_id, int32_t *failed_client_id) {
    // Some simple tests show that iterating bitset isn't too bad.
    // For bitset size below 512, it takes 200~300 ns on an Intel i5 CPU.
    int32_t head_bg_id;
    int32_t bg_id;
    int32_t client_id;
    for (client_id = client_id_st;
         client_id < GlobalContext::get_num_clients(); ++client_id) {
      if (subscriptions_.test(client_id)) {
        //VLOG(0) << "Append to client " << client_id;
        head_bg_id = GlobalContext::get_head_bg_id(client_id);
        bg_id = head_bg_id + GlobalContext::GetBgPartitionNum(row_id);
        //VLOG(0) << "Append to bg " << bg_id
        //      << " (*buffs).size() = " << (*buffs).size();
        bool suc = (*buffs)[bg_id].Append(row_id, row_data, row_size);
        if (!suc) {
          //VLOG(0) << "Append failed, failed bg id = " << bg_id;
          *failed_bg_id = bg_id;
          *failed_client_id = client_id;
          return false;
        }
      }
    }
    //VLOG(0) << "Return true";
    return true;
  }

private:
  std::bitset<PETUUM_MAX_NUM_CLIENTS> subscriptions_;
};

}  //namespace petuum
