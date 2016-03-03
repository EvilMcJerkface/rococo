#include "service.h"
#include "sched.h"

namespace rococo {

TapirServiceImpl::TapirServiceImpl(Scheduler *sched) {
  sched_ = (TapirSched*)sched;
}

void TapirServiceImpl::Handout(const SimpleCommand& cmd,
                               rrr::i32* res,
                               map<int32_t, Value>* output,
                               rrr::DeferredReply* defer) {
  sched_->OnHandoutRequest(cmd, res, output, [defer] () {defer->reply();});
}

void TapirServiceImpl::Prepare(rrr::DeferredReply* defer) {
  verify(0);
}

void TapirServiceImpl::Accept(const cmdid_t& cmd_id,
                              const ballot_t& ballot,
                              const int32_t& decision,
                              rrr::DeferredReply* defer) {
  verify(0);
}

void TapirServiceImpl::FastAccept(const cmdid_t& cmd_id,
                                  rrr::i32* res,
                                  rrr::DeferredReply* defer) {
  sched_->OnFastAccept(cmd_id, res, [defer] () {defer->reply();});
}

void TapirServiceImpl::Decide(const cmdid_t& cmd_id,
                              const rrr::i32& decision,
                              rrr::DeferredReply* defer) {
  sched_->OnDecide(cmd_id, decision, [defer] () {defer->reply();});
}

} // namespace rococo