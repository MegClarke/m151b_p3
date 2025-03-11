// Copyright 2024 blaise
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vector>

namespace tinyrv {

// register status table
// track the mapping from ROB index to RS index
typedef std::vector<int> RegisterStatusTable;

}
//end of given code
// when ROB commits, find RS index and clear RS entry
// index of RST = index of ROB so RST[ROB] = RS index

// class RegisterStatusTable {
// public:
//     //initialize to -1 (meaning not waiting for ROB result)
//     RegisterStatusTable(uint32_t num_registers) : table_(num_registers, -1) {}

//     // Set the ROB index for an architectural register
//     void set(int arch_reg, int rob_index) {
//         assert(arch_reg >= 0 && static_cast<size_t>(arch_reg) < table_.size()); //index within bounds
//         table_[arch_reg] = rob_index;
//     }

//     // Get the ROB index for an architectural register
//     int get(int arch_reg) const {
//         assert(arch_reg >= 0 && static_cast<size_t>(arch_reg) < table_.size()); //index within bounds
//         return table_[arch_reg];
//     }

//     // Clear the ROB index when the instruction commits
//     void clear(int arch_reg) {
//         assert(arch_reg >= 0 && static_cast<size_t>(arch_reg) < table_.size()); //index within bounds
//         table_[arch_reg] = -1; //no longer waiting for ROB result
//     }

//     // Check if a register is waiting on the ROB
//     bool exists(int arch_reg) const {
//         assert(arch_reg >= 0 && static_cast<size_t>(arch_reg) < table_.size());
//         return table_[arch_reg] != -1;
//     }

//     private:
//     std::vector<int> table_;  // Maps architectural registers → ROB index (-1 means no mapping)
// };

// }