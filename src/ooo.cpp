// Copyright 2025 Blaise Tine
//
// Licensed under the Apache License;
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <iomanip>
#include <string.h>
#include <assert.h>
#include <util.h>
#include "types.h"
#include "core.h"
#include "debug.h"
#include "processor_impl.h"

using namespace tinyrv;

void Core::issue() {
  if (issue_queue_->empty())
    return;

  auto& is_data = issue_queue_->data();
  auto instr = is_data.instr;
  auto exe_flags = instr->getExeFlags();

  // check for structial hazards
  // TODO: DONE
  int free_rs_index = -1;
  for (int i = 0; i < (int)RS_.size(); i++) 
  {
    auto& entry = RS_.get_entry(i);
    if (!entry.valid) 
    {
      free_rs_index = i;
      break;
    }
  }
  if (free_rs_index == -1) 
  {
    return;
  }

  uint32_t rs1_data = 0;  // rs1 data obtained from register file or ROB
  uint32_t rs2_data = 0;  // rs2 data obtained from register file or ROB
  int rs1_rsid = -1;      // reservation station id for rs1 (-1 indicates data in already available)
  int rs2_rsid = -1;      // reservation station id for rs2 (-1 indicates data is already available)

  auto rs1 = instr->getRs1();
  auto rs2 = instr->getRs2();

  // get rs1 data
  // check the RAT if value is in the registe file
  // if not in the register file, check data is in the ROB
  // else set rs1_rsid to the reservation station id producing the data
  // remember to first check if the instruction actually uses rs1
  // HINT: should use RAT, ROB, RST, and reg_file_
  // TODO: Done
  if (exe_flags.use_rs1)
  {
    if (!RAT_.exists(rs1)) 
    {
      rs1_data = reg_file_.at(rs1);
    }
    else {
      int rob_idx = RAT_.get(rs1);
      auto& rob_entry = ROB_.get_entry(rob_idx);
      if (rob_entry.ready)
      {
        rs1_data = rob_entry.result;
      }
      else
      {
        if (RST_.exists(rs1)) {
                rs1_rsid = RST_.get(rs1);
            }
      }
    }
  }


  // get rs2 data
  // check the RAT if value is in the registe file
  // if not in the register file, check data is in the ROB
  // else set rs1_rsid to the reservation station id producing the data
  // remember to first check if the instruction actually uses rs2
  // HINT: should use RAT, ROB, RST, and reg_file_
  // TODO: Done
  if (exe_flags.use_rs2) {
    if (!RAT_.exists(rs2))
    {
      rs2_data = reg_file_.at(rs2);
    } 
    else 
    {
      int rob_idx = RAT_.get(rs2);
      auto& rob_entry = ROB_.get_entry(rob_idx);
      if (rob_entry.ready) 
      {
        rs2_data = rob_entry.result;
      } 
      else 
      {
        if (RST_.exists(rs2)) {
                rs2_rsid = RST_.get(rs2);
            }
      }
    }
  }

  // allocate new ROB entry and obtain its index
  // TODO: DONE
  int rob_index = ROB_.allocate(instr);

  // update the RAT mapping if this instruction write to the register file
  // TODO: DONE
  if (exe_flags.use_rd) 
  {
    int rd = instr->getRd();
    RAT_.set(rd, rob_index);
  }

  // issue the instruction to free reservation station
  // TODO: DONE
  int rs_index = RS_.issue(
    rob_index,
    (exe_flags.use_rs1 && rs1_rsid != -1) ? rs1_rsid : -1,
    (exe_flags.use_rs2 && rs2_rsid != -1) ? rs2_rsid : -1,
    rs1_data,
    rs2_data,
    instr
  );


  // update RST mapping
  // TODO: DONE???????
  if (exe_flags.use_rd) // unsure if RST_ mapping should be updated within conditional or just regardless because it says update RST mapping but doesn't specicify to check if instruction writes to register file
  {
    int rd = instr->getRd();
    RST_.set(rd, rs_index);
  }


  DT(2, "Issue: " << *instr);

  // pop issue queue
  issue_queue_->pop();
}

void Core::execute() {
  // execute functional units
  for (auto fu : FUs_) {
    fu->execute();
  }

  // find the next functional units that is done executing
  // and push its output result to the common data bus
  // then clear the functional unit.
  // The CDB can only serve one functional unit per cycle
  // HINT: should use CDB_ and FUs_
  for (auto fu : FUs_) {
    // TODO: DONE
    if (fu->done()) 
    {
      auto out = fu->get_output();
      CDB_.push(out.result, out.rob_index, out.rs_index);
      fu->clear();
      break; // break after found first functional unit done as CDB can only serve one function unit per cycle
    }
  }

  // schedule ready instructions to corresponding functional units
  // iterate through all reservation stations, check if the entry is valid, but not running yet,
  // and its operands are ready, and also make sure that is not locked (LSU case).
  // once a candidate is found, issue the instruction to its corresponding functional unit.
  // HINT: should use RS_ and FUs_
  for (int rs_index = 0; rs_index < (int)RS_.size(); ++rs_index) {
    auto& entry = RS_.get_entry(rs_index);
    // TODO: DONE
    if (entry.valid && !entry.running && entry.rs1_index == -1 && entry.rs2_index == -1)
    {
      if (entry.instr->getFUType() == FUType::LSU && RS_.locked(rs_index))
      {
        continue;
      }
      FUType fu_type = entry.instr->getFUType();
      auto fu = FUs_.at((int)fu_type);

      fu->issue(entry.instr, entry.rob_index, rs_index, entry.rs1_data, entry.rs2_data);
      entry.running = true;
    }
  }
}

void Core::writeback() {
  // CDB broadcast
  if (CDB_.empty())
    return;

  auto& cdb_data = CDB_.data();

  // update all reservation stations waiting for operands
  // HINT: use RS::entry_t::update_operands()
  for (int rs_index = 0; rs_index < (int)RS_.size(); ++rs_index) {
    // TODO: DONE
    auto& entry = RS_.get_entry(rs_index);
    if (entry.valid)
    {
      entry.update_operands(cdb_data);
    }
  }

  // free the RS entry associated with this CDB response
  // so that it can be used by other instructions
  // TODO: DONE
  for (int rs_index = 0; rs_index < (int)RS_.size(); rs_index++)
  {
    auto& entry = RS_.get_entry(rs_index);
    if (entry.valid && entry.rob_index == cdb_data.rob_index) 
    {
      RS_.release(rs_index);
      int rd = entry.instr->getRd();
      if (RST_.exists(rd) && RST_.get(rd) == rs_index){
        RST_.clear(rd);
      }
      break;
    }
  }

  // update ROB
  // TODO: DONE
  ROB_.update(cdb_data);

  // clear CDB
  // TODO: DONE
  CDB_.pop();

  RS_.dump();
}

void Core::commit() {
  // commit ROB head entry
  if (ROB_.empty())
    return;

  int head_index = ROB_.head_index();
  auto& rob_head = ROB_.get_entry(head_index);

  // check if the head entry is ready to commit
  if (rob_head.ready) {
    auto instr = rob_head.instr;
    auto exe_flags = instr->getExeFlags();

    // If this instruction writes to the register file,
    // (1) update the register file
    // (2) clear the RAT if still pointing to this ROB head
    // TODO: DONE
    if (exe_flags.use_rd)
    {
      int rd = instr->getRd();
      reg_file_.at(rd) = rob_head.result;
      if (RAT_.exists(rd) && RAT_.get(rd) == head_index){
        RAT_.clear(rd);
      }
    }

    // pop ROB entry
    // TODO:
    ROB_.pop();

    DT(2, "Commit: " << *instr);

    assert(perf_stats_.instrs <= fetched_instrs_);
    ++perf_stats_.instrs;

    // handle program termination
    if (exe_flags.is_exit) {
      exited_ = true;
    }
  }

  ROB_.dump();
}