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

  BT::NodeStatus GripperInterface::open()
  {
    _open = true;
    std::cout << "GripperInterface::open" << std::endl;
    return BT::NodeStatus::SUCCESS;
  }

  BT::NodeStatus GripperInterface::close()
  {
    std::cout << "GripperInterface::close" << std::endl;
    _open = false;
    return BT::NodeStatus::SUCCESS;
  }

  SaySomething::SaySomething(const std::string &name, const BT::NodeConfig &config)
      : BT::SyncActionNode(name, config)
  {
  }

  BT::PortsList SaySomething::providedPorts()
  {
    return {BT::InputPort<std::string>("message")};
  }

  BT::NodeStatus SaySomething::tick()
  {
    BT::Expected<std::string> msg = BT::TreeNode::getInput<std::string>("message");

    if (!msg)
    {
      throw BT::RuntimeError("missing required input [message]: ",
                             msg.error());
    }

    std::cout << "Robot says: " << msg.value() << std::endl;
    return BT::NodeStatus::SUCCESS;
  }

  ThinkWhatToSay::ThinkWhatToSay(const std::string &name, const BT::NodeConfig &config)
      : BT::SyncActionNode(name, config)
  {
  }

  BT::PortsList ThinkWhatToSay::providedPorts()
  {
    return {BT::OutputPort<std::string>("text")};
  }

  BT::NodeStatus ThinkWhatToSay::tick()
  {
    BT::TreeNode::setOutput("text", "The answer is 42");
    return BT::NodeStatus::SUCCESS;
  }

  CalculateGoal::CalculateGoal(const std::string &name, const BT::NodeConfig &config)
      : BT::SyncActionNode(name, config)
  {
  }

  BT::PortsList CalculateGoal::providedPorts()
  {
    return {BT::OutputPort<Position2D>("goal")};
  }

  BT::NodeStatus CalculateGoal::tick()
  {
    Position2D mygoal = {1.1, 2.3};
    BT::TreeNode::setOutput<Position2D>("goal", mygoal);
    return BT::NodeStatus::SUCCESS;
  }

  PrintTarget::PrintTarget(const std::string &name, const BT::NodeConfig &config)
      : BT::SyncActionNode(name, config)
  {
  }

  BT::PortsList PrintTarget::providedPorts()
  {
    const char *description = "Simply print the goal on console...";
    return {BT::InputPort<Position2D>("target", description)};
  }

  BT::NodeStatus PrintTarget::tick()
  {
    auto res = BT::TreeNode::getInput<Position2D>("target");
    if (!res)
    {
      throw BT::RuntimeError("error reading port [target]:", res.error());
    }

    Position2D target = res.value();
    std::printf("Target positions: [ %.1f, %.1f ]\n", target.x, target.y);
    return BT::NodeStatus::SUCCESS;
  }

  BT::NodeStatus MoveBaseAction::onStart()
  {
    if (!getInput<Pose2D>("goal", _goal))
    {
      throw BT::RuntimeError("missing required input [goal]");
    }
    printf("[ MoveBase: SEND REQUEST ]. goal: x=%f y=%f theta=%f\n",
           _goal.x, _goal.y, _goal.theta);

    // We use this counter to simulate an action that takes a certain
    // amount of time to be completed (200 ms)
    _completion_time = chr::system_clock::now() + chr::milliseconds(220);

    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus MoveBaseAction::onRunning()
  {
    // Pretend that we are checking if the reply has been received
    // you don't want to block inside this function too much time.
    std::this_thread::sleep_for(chr::milliseconds(10));

    // Pretend that, after a certain amount of time,
    // we have completed the operation
    if (chr::system_clock::now() >= _completion_time)
    {
      std::cout << "[ MoveBase: FINISHED ]" << std::endl;
      return BT::NodeStatus::SUCCESS;
    }
    return BT::NodeStatus::RUNNING;
  }

  void MoveBaseAction::onHalted()
  {
    printf("[ MoveBase: ABORTED ]");
  }

} // namespace mybt
