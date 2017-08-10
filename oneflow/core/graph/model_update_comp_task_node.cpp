#include "oneflow/core/graph/model_update_comp_task_node.h"
#include "oneflow/core/graph/model_update_task_graph.h"
#include "oneflow/core/operator/operator_manager.h"

namespace oneflow {

void MdUpdtCompTaskNode::BuildExecAndEnrollLbn2Regsts(TaskGraph* gph) {
  CHECK(IsFwNode());
  auto md_updt_gph = static_cast<MdUpdtTaskGraph*>(gph);
  CompTaskNode* diff_acc_task = md_updt_gph->diff_acc_task();
  std::shared_ptr<RegstDesc> model_diff_acc_regst;
  if (diff_acc_task != nullptr) {
    model_diff_acc_regst =
        diff_acc_task->GetProducedRegstDesc("model_diff_acc");
  }
  TakeOverRegstDesc(fw_task_, "model");
  TakeOverRegstDesc(fw_task_, "model_tmp");
  auto model_regst = GetProducedRegstDesc("model");

  ExecNode* exec_node = mut_exec_gph().NewNode();
  exec_node->mut_op() = chain_node()->SoleOp();
  const std::string ibn = "model_diffs";
  if (model_diff_acc_regst != nullptr) {
    exec_node->BindBnInOpAndRegst(ibn, model_diff_acc_regst);
    ConsumeRegstDesc(ibn, model_diff_acc_regst);
  }
  exec_node->BindBnInOpAndRegst(exec_node->op()->SoleObn(), model_regst);
  auto data_tmp_regst = NewProducedRegstDesc("data_tmp");
  for (const std::string& dtbn : exec_node->op()->data_tmp_bns()) {
    const std::string& lbn = exec_node->op()->Lbn4BnInOp(dtbn);
    data_tmp_regst->EnrollLbn(lbn);
    exec_node->BindBnInOpAndRegst(dtbn, data_tmp_regst);
  }
  mut_exec_gph().UpdateSourceAndSink();
}

void MdUpdtCompTaskNode::InferShapeOfBlobsInProducedRegsts(TaskGraph* gph) {
  CHECK(IsFwNode());
  ExecNode* exec_node = exec_gph().SoleNode();
  auto model_diffs_regst = GetConsumedRegstDesc("model_diffs");
  Shape packed_model_diffs_shape({model_diffs_regst->CompElemCntOfAllBlob()});
  exec_node->op()->InferShape4FwBlobs(
      [&](const std::string& bn_in_op) -> Shape* {
        if (bn_in_op == "model_diffs") {
          return &packed_model_diffs_shape;
        } else {
          return exec_node->GetMutShapePtr4BnInOpFunc()(bn_in_op);
        }
      },
      kDataParallel, 0, 0);
}

}  // namespace oneflow
