/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Ioan Sucan */

#include <moveit/rviz_plugin_render_tools/planning_scene_render.h>
#include <moveit/rviz_plugin_render_tools/robot_state_visualization.h>
#include <moveit/rviz_plugin_render_tools/render_shapes.h>
#include <rviz_common/display_context.hpp>
#include <rviz_rendering/objects/movable_text.hpp>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

namespace moveit_rviz_plugin
{

class TextThatActuallyGoesWhereYouTellItToo : public Ogre::SceneNode
{
public:
  TextThatActuallyGoesWhereYouTellItToo(Ogre::SceneManager * manager, const std::string & text, const Ogre::Vector3 & position)
    : SceneNode(manager, text + "_text_node"),
      m_text(text, "Liberation Sans", 0.03, Ogre::ColourValue::Black)
  {
    Ogre::SceneNode::attachObject(&m_text);
    setPosition(position);
    m_text.showOnTop(true);
  }

private:
  rviz_rendering::MovableText m_text;
};


PlanningSceneRender::PlanningSceneRender(Ogre::SceneNode* node, rviz_common::DisplayContext* context,
                                         const RobotStateVisualizationPtr& robot)
  : planning_scene_geometry_node_(node->createChildSceneNode()), context_(context), scene_robot_(robot)
{
  render_shapes_.reset(new RenderShapes(context));
}

PlanningSceneRender::~PlanningSceneRender()
{
  context_->getSceneManager()->destroySceneNode(planning_scene_geometry_node_);
}

void PlanningSceneRender::clear()
{
  render_shapes_->clear();
  for (auto rt : render_texts_) {
    planning_scene_geometry_node_->removeChild(rt);
    delete rt;
  }
  render_texts_.clear();
}

void PlanningSceneRender::renderPlanningScene(const planning_scene::PlanningSceneConstPtr& scene,
                                              const Ogre::ColourValue& default_env_color,
                                              const Ogre::ColourValue& default_attached_color,
                                              OctreeVoxelRenderMode octree_voxel_rendering,
                                              OctreeVoxelColorMode octree_color_mode, float default_scene_alpha)
{
  if (!scene)
    return;

  clear();

  if (scene_robot_)
  {
    robot_state::RobotState* rs = new robot_state::RobotState(scene->getCurrentState());
    rs->update();

    std_msgs::msg::ColorRGBA color;
    color.r = default_attached_color.r;
    color.g = default_attached_color.g;
    color.b = default_attached_color.b;
    color.a = 1.0f;
    planning_scene::ObjectColorMap color_map;
    scene->getKnownObjectColors(color_map);
    scene_robot_->update(robot_state::RobotStateConstPtr(rs), color, color_map);
  }

  const std::vector<std::string>& ids = scene->getWorld()->getObjectIds();
  for (const std::string& id : ids)
  {
    collision_detection::CollisionEnv::ObjectConstPtr object = scene->getWorld()->getObject(id);
    Ogre::ColourValue color = default_env_color;
    float alpha = default_scene_alpha;
    if (scene->hasObjectColor(id))
    {
      const std_msgs::msg::ColorRGBA& c = scene->getObjectColor(id);
      color.r = c.r;
      color.g = c.g;
      color.b = c.b;
      alpha = c.a;
    }
    Eigen::Vector3f average_position(0.f, 0.f, 0.f);
    for (std::size_t j = 0; j < object->shapes_.size(); ++j) {
      render_shapes_->renderShape(planning_scene_geometry_node_,
        object->shapes_[j].get(),
        object->shape_poses_[j],
        octree_voxel_rendering,
        octree_color_mode,
        color,
        alpha);
      auto tl_iso = object->shape_poses_[j].translation();
      auto tl_vec = Eigen::Vector3f(tl_iso.x(), tl_iso.y(), tl_iso.z());
      average_position += tl_vec;
    }
    average_position /= object->shapes_.size();
    auto position = Ogre::Vector3(average_position.x(), average_position.y(), average_position.z());
    auto text_object = new TextThatActuallyGoesWhereYouTellItToo(context_->getSceneManager(), id, position);
    planning_scene_geometry_node_->addChild(text_object);
    render_texts_.emplace_back(text_object);
  }
}
}  // namespace moveit_rviz_plugin
