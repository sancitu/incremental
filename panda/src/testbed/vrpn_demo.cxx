#include "framework.h"

#include <eventHandler.h>
#include <string>

#include <transform2sg.h>
#include <trackerTransform.h>
#include <trackerNode.h>
#include <vrpnClient.h>
#include <dataRelation.h>
#include <renderRelation.h>
#include <namedNode.h>

#include <trackball.h>
#include <mouseWatcher.h>

////////////////////////////////////////////////
//Globals
////////////////////////////////////////////////
VrpnClient *vrpn_client;
Transform2SG *tracker2cam;
TrackerTransform *evil_transform;
TrackerNode *evil_tracker;

//From framework
extern PT_NamedNode data_root;
extern RenderRelation* first_arc;
extern PT(Trackball) trackball;
extern PT(MouseWatcher) mouse_watcher;

void demo_keys(EventHandler&) {
  vrpn_client = new VrpnClient(string("evildyne"));
  evil_tracker = new TrackerNode(vrpn_client, string("Isense"), 2);
  evil_transform = new TrackerTransform("evil_transform");

  new DataRelation(data_root, evil_tracker);
  new DataRelation(evil_tracker, evil_transform);

  tracker2cam = new Transform2SG("tracker2cam");
  tracker2cam->set_arc(first_arc);

  new DataRelation(evil_transform, tracker2cam);
}

int main(int argc, char *argv[]) {
  define_keys = &demo_keys;
  return framework_main(argc, argv);
}
