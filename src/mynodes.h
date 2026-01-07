#pragma once
#include <behaviortree_cpp/bt_factory.h>

namespace mybt
{

  class ApproachObject : public BT::SyncActionNode
  {
  public:
    explicit ApproachObject(const std::string &name)
        : BT::SyncActionNode(name, {}) {}
    BT::NodeStatus tick() override;
  };

  BT::NodeStatus CheckBattery(BT::TreeNode &self);

} // namespace mybt
