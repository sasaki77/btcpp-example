#include "mynodes.h"
#include <iostream>

namespace mybt
{

  BT::NodeStatus ApproachObject::tick()
  {
    std::cout << "ApproachObject: " << this->name() << std::endl;
    return BT::NodeStatus::SUCCESS;
  }

  BT::NodeStatus CheckBattery(BT::TreeNode & /*self*/)
  {
    std::cout << "[ Battery: OK ]" << std::endl;
    return BT::NodeStatus::SUCCESS;
  }

} // namespace mybt
