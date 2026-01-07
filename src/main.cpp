#include <behaviortree_cpp/bt_factory.h>
#include "mynodes.h"

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

  BT::Tree tree;
  try
  {
    tree = factory.createTreeFromFile(xml_path);
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to load XML: " << e.what() << "\n";
    return 2;
  }

  tree.tickWhileRunning();

  return 0;
}
