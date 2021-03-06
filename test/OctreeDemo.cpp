#include <pcl/visualization/cloud_viewer.h>
#include <pcl/visualization/point_picking_event.h>
#include <pcl/visualization/keyboard_event.h>
#include <iostream>
#include <cstring>
#include "Tools.hpp"
#include "PointOctree.hpp"

namespace global {
  int    argc;
  char** argv;
  
  enum Method {
    cubic, spheric, spheric_inner
  };

  Method method = cubic;
}

void show_help() {
  std::cout << "Octree Demo\n";
  std::cout << "Usage: " << global::argv[0] << " file.off [--cube | --spheric | --spheric-inner]\n";
  std::cout << "Example: " << global::argv[0] << " cube.off --cube\n";
  std::cout << "  -h    Muestra esta ayuda\n";
  std::cout << "  -v    Muestra version\n";
}

void show_version() {
  std::cout << "Octree demo\n";
  std::cout << "version: 0.0.1\n";
}

void get_method() {
  if (!global::argv[2]) return;

  if (std::strcmp(global::argv[2], "--cube") == 0) { 
    global::method = global::cubic;
  } else if (std::strcmp(global::argv[2], "--spheric") == 0) {
    global::method = global::spheric;
  } else if (std::strcmp(global::argv[2], "--spheric-inner") == 0) {
    global::method = global::spheric_inner;
  } else {
    throw std::runtime_error("Unknown method for query");
  }
}

void keyboard_events(const pcl::visualization::KeyboardEvent& e, void *p) {
  if (!e.keyUp()) return;

  auto keycode = e.getKeyCode();
  if (keycode != 'z') return;

  auto octree = *(std::shared_ptr<qoaed::PointOctree<pcl::PointXYZRGB*, float>>*) p;
  octree->visit_dfs([](auto& p){ (*p)->r = 255; (*p)->g = 255; (*p)->b = 255; (*p)->a = 100; });
  std::cout << "Restarted all points" << std::endl;
}

void point_pick_func(const pcl::visualization::PointPickingEvent& e, void* p) {
  float x, y, z;
  e.getPoint(x,y,z);
  qoaed::Point3D<float> point(x,y,z);
  auto octree = *(std::shared_ptr<qoaed::PointOctree<pcl::PointXYZRGB*, float>>*) p;
  auto nvis = octree->find(point);

  std::cout << "Clicked " << point.x << ' ' << point.y << ' ' << point.z << '\n';

  if (!nvis) {
    std::cerr << "Seems like the point isn't in the octree" << '\n';
    return;
  }

  double radio;
  std::cout << "Input a radio: ";
  std::cin  >> radio;

  if (global::method == global::cubic) {
    qoaed::PointOctree<pcl::PointXYZRGB*, float>::Cube cube(nvis.get_point(), radio);
    octree->cubic_query(cube, [](auto& p) { (*p)->r = 255; (*p)->g = 0; (*p)->b = 0; });
  } else if (global::method == global::spheric) {
    qoaed::PointOctree<pcl::PointXYZRGB*, float>::Sphere sphere(nvis.get_point(), radio);
    octree->spheric_query(sphere, [](auto& p) { (*p)->r = 255; (*p)->g = 0; (*p)->b = 0; });
  } else {
    octree->spheric_query_inner(nvis.get_point(), radio,  [](auto& p) { (*p)->r = 255; (*p)->g = 0; (*p)->b = 0; });
  }
}

int main(int argc, char** argv) {
  global::argc = argc;
  global::argv = argv;

  if (argc < 2) {
    show_help();
    return 1;
  }

  if (std::strcmp(argv[1], "-h") == 0) {
    show_help();
    return 0;
  } 

  if (std::strcmp(argv[1], "-v") == 0) {
    show_version();
    return 0;
  }

  try {
    get_method();
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    show_help();
    return 1;
  }

  try {
    auto points = qoaed::tools::read_off<float>(argv[1]);
    auto octree = std::make_shared<qoaed::PointOctree<pcl::PointXYZRGB*, float>>();

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::visualization::CloudViewer cloud_viewer("Viewer");

    cloud->width  = points.size();
    cloud->height = 1;
    cloud->reserve(points.size());

    pcl::PointXYZRGB tmp_point;
    for (size_t ii = 0; ii < points.size(); ++ii) {
      tmp_point.x = points[ii].x;
      tmp_point.y = points[ii].y;
      tmp_point.z = points[ii].z;
      tmp_point.r = 255;
      tmp_point.g = 255;
      tmp_point.b = 255;
      cloud->push_back(tmp_point);
      octree->insert(points[ii].x, points[ii].y, points[ii].z, &cloud->back());
    }

    octree->visit_bfs([](auto& p) { 
      std::cout << p.get_x() << ' ' << p.get_y() << ' ' << p.get_z() << '\n'; 
    });


    cloud_viewer.registerPointPickingCallback(point_pick_func, &octree);
    cloud_viewer.registerKeyboardCallback(keyboard_events, &octree);
    cloud_viewer.showCloud(cloud);
    while (!cloud_viewer.wasStopped(100)) 
      cloud_viewer.showCloud(cloud);

  } catch (const std::exception& e) {
    std::cout << e.what() << '\n';
    return 1;
  }

  return 0;
}

