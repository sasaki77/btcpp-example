#include <behaviortree_cpp/bt_factory.h>
#include <cadef.h>
#include "mynodes.h"
#include "canode.h"

int main(int argc, char **argv)
{

  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <path/to/tree.xml>\n";
    return 1;
  }
  const std::string xml_path = argv[1];

  BT::BehaviorTreeFactory factory;

  // Register nodes
  factory.registerNodeType<mybt::ApproachObject>("ApproachObject");
  factory.registerSimpleAction("CheckBattery", mybt::CheckBattery);

  mybt::GripperInterface gripper;
  factory.registerSimpleAction("OpenGripper", [&](BT::TreeNode &)
                               { return gripper.open(); });
  factory.registerSimpleAction("CloseGripper", [&](BT::TreeNode &)
                               { return gripper.close(); });

  factory.registerNodeType<mybt::SaySomething>("SaySomething");
  factory.registerNodeType<mybt::ThinkWhatToSay>("ThinkWhatToSay");

  factory.registerNodeType<mybt::CalculateGoal>("CalculateGoal");
  factory.registerNodeType<mybt::PrintTarget>("PrintTarget");

  factory.registerSimpleCondition("BatteryOK", mybt::CheckBattery);
  factory.registerNodeType<mybt::MoveBaseAction>("MoveBase");

  factory.registerNodeType<mybt::CaGetAction>("CaGetAction");
  factory.registerNodeType<mybt::ShowCAResult>("ShowCAResult");

  BT::Tree tree;
  try
  {
    factory.registerBehaviorTreeFromFile(xml_path);
    tree = factory.createTree("MainTree");
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to load XML: " << e.what() << "\n";
    return 2;
  }

  tree.tickWhileRunning();

  return 0;
}
