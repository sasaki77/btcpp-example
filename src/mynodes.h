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
  class GripperInterface
  {
  public:
    GripperInterface() : _open(true) {}

    BT::NodeStatus open();
    BT::NodeStatus close();

  private:
    bool _open; // shared information
  };

  class SaySomething : public BT::SyncActionNode
  {
  public:
    SaySomething(const std::string &name, const BT::NodeConfig &config);

    static BT::PortsList providedPorts();

    BT::NodeStatus tick() override;
  };

  class ThinkWhatToSay : public BT::SyncActionNode
  {
  public:
    ThinkWhatToSay(const std::string &name, const BT::NodeConfig &config);

    static BT::PortsList providedPorts();

    BT::NodeStatus tick() override;
  };

  struct Position2D
  {
    double x;
    double y;
  };

  class CalculateGoal : public BT::SyncActionNode
  {
  public:
    CalculateGoal(const std::string &name, const BT::NodeConfig &config);

    static BT::PortsList providedPorts();

    BT::NodeStatus tick() override;
  };

  class PrintTarget : public BT::SyncActionNode
  {

  public:
    PrintTarget(const std::string &name, const BT::NodeConfig &config);

    static BT::PortsList providedPorts();

    BT::NodeStatus tick() override;
  };

  struct Pose2D
  {
    double x, y, theta;
  };

  namespace chr = std::chrono;

  class MoveBaseAction : public BT::StatefulActionNode
  {
  public:
    // Any TreeNode with ports must have a constructor with this signature
    MoveBaseAction(const std::string &name, const BT::NodeConfig &config)
        : StatefulActionNode(name, config)
    {
    }

    // It is mandatory to define this static method.
    static BT::PortsList providedPorts()
    {
      return {BT::InputPort<Pose2D>("goal")};
    }

    // this function is invoked once at the beginning.
    BT::NodeStatus onStart() override;

    // If onStart() returned RUNNING, we will keep calling
    // this method until it return something different from RUNNING
    BT::NodeStatus onRunning() override;

    // callback to execute if the action was aborted by another node
    void onHalted() override;

  private:
    Pose2D _goal;
    chr::system_clock::time_point _completion_time;
  };

} // namespace mybt

namespace BT
{
  template <>
  inline mybt::Position2D convertFromString(StringView str)
  {
    // We expect real numbers separated by semicolons
    auto parts = splitString(str, ';');
    if (parts.size() != 2)
    {
      throw RuntimeError("invalid input)");
    }
    else
    {
      mybt::Position2D output;
      output.x = convertFromString<double>(parts[0]);
      output.y = convertFromString<double>(parts[1]);
      return output;
    }
  }

  template <>
  inline mybt::Pose2D convertFromString(StringView str)
  {
    // We expect real numbers separated by semicolons
    auto parts = splitString(str, ';');
    if (parts.size() != 3)
    {
      throw RuntimeError("invalid input)");
    }
    else
    {
      mybt::Pose2D output;
      output.x = convertFromString<double>(parts[0]);
      output.y = convertFromString<double>(parts[1]);
      output.theta = convertFromString<double>(parts[1]);
      return output;
    }
  }
}
