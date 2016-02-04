#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../communicator.h"

namespace rococo {

class TxnCommand;
class MultiPaxosCommo : public Communicator {
 public:
  using Communicator::Communicator;
  void BroadcastPrepare(parid_t par_id,
                        slotid_t slot_id,
                        ballot_t ballot,
                        const function<void(Future *fu)> &callback);
  void BroadcastAccept(parid_t par_id,
                       slotid_t slot_id,
                       ballot_t ballot,
                       TxnCommand& cmd,
                       const function<void(Future*)> &callback);
  void BroadcastDecide(parid_t par_id,
                       ballot_t ballot,
                       TxnCommand& cmd) {
    verify(0);
  }
};

} // namespace rococo