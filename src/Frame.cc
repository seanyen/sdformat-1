/*
 * Copyright 2019 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#include <string>
#include <ignition/math/Pose3.hh>
#include "sdf/Frame.hh"
#include "sdf/Error.hh"
#include "sdf/Types.hh"
#include "Utils.hh"

using namespace sdf;

class sdf::FramePrivate
{
  /// \brief Name of the frame.
  public: std::string name = "";

  /// \brief Name of the attached-to frame.
  public: std::string attachedTo = "";

  /// \brief Pose of the frame object
  public: ignition::math::Pose3d pose = ignition::math::Pose3d::Zero;

  /// \brief Name of the relative-to frame.
  public: std::string poseRelativeTo = "";

  /// \brief The SDF element pointer used during load.
  public: sdf::ElementPtr sdf;

  /// \brief Name of graph source.
  std::string graphSourceName = "";

  /// \brief Weak pointer to model's or worlds's Frame Attached-To Graph.
  public: std::weak_ptr<const sdf::FrameAttachedToGraph> frameAttachedToGraph;

  /// \brief Weak pointer to model's or world's Pose Relative-To Graph.
  public: std::weak_ptr<const sdf::PoseRelativeToGraph> poseRelativeToGraph;
};

/////////////////////////////////////////////////
Frame::Frame()
  : dataPtr(new FramePrivate)
{
}

/////////////////////////////////////////////////
Frame::Frame(Frame &&_frame)
{
  this->dataPtr = _frame.dataPtr;
  _frame.dataPtr = nullptr;
}

/////////////////////////////////////////////////
Frame::~Frame()
{
  delete this->dataPtr;
  this->dataPtr = nullptr;
}

//////////////////////////////////////////////////
Frame::Frame(const Frame &_frame)
  : dataPtr(new FramePrivate)
{
  *this->dataPtr = *_frame.dataPtr;
}

//////////////////////////////////////////////////
Frame &Frame::operator=(const Frame &_frame)
{
  if (!this->dataPtr)
  {
    this->dataPtr = new FramePrivate;
  }
  *this->dataPtr = *_frame.dataPtr;
  return *this;
}

//////////////////////////////////////////////////
Frame &Frame::operator=(Frame &&_frame)
{
  this->dataPtr = _frame.dataPtr;
  _frame.dataPtr = nullptr;
  return *this;
}

/////////////////////////////////////////////////
Errors Frame::Load(ElementPtr _sdf)
{
  Errors errors;

  this->dataPtr->sdf = _sdf;

  // Check that the provided SDF element is a <frame>
  // This is an error that cannot be recovered, so return an error.
  if (_sdf->GetName() != "frame")
  {
    errors.push_back({ErrorCode::ELEMENT_INCORRECT_TYPE,
        "Attempting to load a Frame, but the provided SDF element is not a "
        "<frame>."});
    return errors;
  }

  // Read the frames's name
  if (!loadName(_sdf, this->dataPtr->name))
  {
    errors.push_back({ErrorCode::ATTRIBUTE_MISSING,
                     "A frame name is required, but the name is not set."});
  }

  // Check that the frame's name is valid
  if (isReservedName(this->dataPtr->name))
  {
    errors.push_back({ErrorCode::RESERVED_NAME,
                     "The supplied frame name [" + this->dataPtr->name +
                     "] is reserved."});
  }

  // Read the frame's attached_to attribute
  if (_sdf->HasAttribute("attached_to"))
  {
    auto pair = _sdf->Get<std::string>("attached_to", "");
    if (pair.second)
    {
      this->dataPtr->attachedTo = pair.first;
    }
  }

  // Load the pose. Ignore the return value since the pose is optional.
  loadPose(_sdf, this->dataPtr->pose, this->dataPtr->poseRelativeTo);

  return errors;
}

/////////////////////////////////////////////////
const std::string &Frame::Name() const
{
  return this->dataPtr->name;
}

/////////////////////////////////////////////////
void Frame::SetName(const std::string &_name) const
{
  this->dataPtr->name = _name;
}

/////////////////////////////////////////////////
const std::string &Frame::AttachedTo() const
{
  return this->dataPtr->attachedTo;
}

/////////////////////////////////////////////////
const ignition::math::Pose3d &Frame::RawPose() const
{
  return this->dataPtr->pose;
}

/////////////////////////////////////////////////
const std::string &Frame::PoseRelativeTo() const
{
  return this->dataPtr->poseRelativeTo;
}

/////////////////////////////////////////////////
void Frame::SetAttachedTo(const std::string &_frame)
{
  this->dataPtr->attachedTo = _frame;
}

/////////////////////////////////////////////////
void Frame::SetRawPose(const ignition::math::Pose3d &_pose)
{
  this->dataPtr->pose = _pose;
}

/////////////////////////////////////////////////
void Frame::SetPoseRelativeTo(const std::string &_frame)
{
  this->dataPtr->poseRelativeTo = _frame;
}

/////////////////////////////////////////////////
void Frame::SetFrameAttachedToGraph(
    std::weak_ptr<const FrameAttachedToGraph> _graph)
{
  this->dataPtr->frameAttachedToGraph = _graph;
}

/////////////////////////////////////////////////
void Frame::SetPoseRelativeToGraph(
    std::weak_ptr<const PoseRelativeToGraph> _graph)
{
  this->dataPtr->poseRelativeToGraph = _graph;
  auto graph = this->dataPtr->poseRelativeToGraph.lock();
  if (graph)
  {
    this->dataPtr->graphSourceName = graph->sourceName;
  }
}

/////////////////////////////////////////////////
Errors Frame::ResolveAttachedToBody(std::string &_body) const
{
  Errors errors;

  auto graph = this->dataPtr->frameAttachedToGraph.lock();
  if (!graph)
  {
    errors.push_back({ErrorCode::ELEMENT_INVALID,
        "Frame has invalid pointer to FrameAttachedToGraph."});
    return errors;
  }

  std::string body;
  errors = resolveFrameAttachedToBody(body, *graph, this->dataPtr->name);
  if (errors.empty())
  {
    _body = body;
  }
  return errors;
}

/////////////////////////////////////////////////
sdf::SemanticPose Frame::SemanticPose() const
{
  return sdf::SemanticPose(
      ignition::math::Pose3d::Zero,
      this->dataPtr->name,
      this->dataPtr->graphSourceName,
      this->dataPtr->poseRelativeToGraph);
}

/////////////////////////////////////////////////
sdf::ElementPtr Frame::Element() const
{
  return this->dataPtr->sdf;
}