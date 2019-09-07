// Copyright 2017 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

#include "rclcpp/clock.hpp"
#include "rclcpp/time.hpp"

#include "builtin_interfaces/msg/duration.hpp"

#include "rcl/time.h"

#include "rclcpp/exceptions.hpp"

#include "rcutils/logging_macros.h"

namespace rclcpp
{

Duration::Duration(int32_t seconds, uint32_t nanoseconds)
{
  rcl_duration_.nanoseconds = RCL_S_TO_NS(static_cast<int64_t>(seconds));
  rcl_duration_.nanoseconds += nanoseconds;
}

Duration::Duration(int64_t nanoseconds) { rcl_duration_.nanoseconds = nanoseconds; }

Duration::Duration(std::chrono::nanoseconds nanoseconds)
{
  rcl_duration_.nanoseconds = nanoseconds.count();
}

Duration::Duration(const Duration & rhs)
{
  rcl_duration_.nanoseconds = rhs.rcl_duration_.nanoseconds;
}

Duration::Duration(const builtin_interfaces::msg::Duration & duration_msg)
{
  rcl_duration_.nanoseconds = RCL_S_TO_NS(static_cast<uint64_t>(duration_msg.sec));
  rcl_duration_.nanoseconds += duration_msg.nanosec;
}

Duration::Duration(const rcl_duration_t & duration) : rcl_duration_(duration)
{
  // noop
}

Duration::operator builtin_interfaces::msg::Duration() const
{
  builtin_interfaces::msg::Duration msg_duration;
  msg_duration.sec = static_cast<std::int32_t>(RCL_NS_TO_S(rcl_duration_.nanoseconds));
  msg_duration.nanosec =
    static_cast<std::uint32_t>(rcl_duration_.nanoseconds % (1000 * 1000 * 1000));
  return msg_duration;
}

Duration & Duration::operator=(const Duration & rhs)
{
  rcl_duration_.nanoseconds = rhs.rcl_duration_.nanoseconds;
  return *this;
}

Duration & Duration::operator=(const builtin_interfaces::msg::Duration & duration_msg)
{
  rcl_duration_.nanoseconds = RCL_S_TO_NS(static_cast<int64_t>(duration_msg.sec));
  rcl_duration_.nanoseconds += duration_msg.nanosec;
  return *this;
}

bool Duration::operator==(const rclcpp::Duration & rhs) const
{
  return rcl_duration_.nanoseconds == rhs.rcl_duration_.nanoseconds;
}

bool Duration::operator<(const rclcpp::Duration & rhs) const
{
  return rcl_duration_.nanoseconds < rhs.rcl_duration_.nanoseconds;
}

bool Duration::operator<=(const rclcpp::Duration & rhs) const
{
  return rcl_duration_.nanoseconds <= rhs.rcl_duration_.nanoseconds;
}

bool Duration::operator>=(const rclcpp::Duration & rhs) const
{
  return rcl_duration_.nanoseconds >= rhs.rcl_duration_.nanoseconds;
}

bool Duration::operator>(const rclcpp::Duration & rhs) const
{
  return rcl_duration_.nanoseconds > rhs.rcl_duration_.nanoseconds;
}

void bounds_check_duration_sum(int64_t lhsns, int64_t rhsns, uint64_t max)
{
  auto abs_lhs = (uint64_t)std::abs(lhsns);
  auto abs_rhs = (uint64_t)std::abs(rhsns);

  if (lhsns > 0 && rhsns > 0) {
    if (abs_lhs + abs_rhs > (uint64_t)max) {
      throw std::overflow_error("addition leads to int64_t overflow");
    }
  } else if (lhsns < 0 && rhsns < 0) {
    if (abs_lhs + abs_rhs > (uint64_t)max) {
      throw std::underflow_error("addition leads to int64_t underflow");
    }
  }
}

Duration Duration::operator+(const rclcpp::Duration & rhs) const
{
  bounds_check_duration_sum(
    this->rcl_duration_.nanoseconds, rhs.rcl_duration_.nanoseconds,
    std::numeric_limits<rcl_duration_value_t>::max());
  return Duration(rcl_duration_.nanoseconds + rhs.rcl_duration_.nanoseconds);
}

void bounds_check_duration_difference(int64_t lhsns, int64_t rhsns, uint64_t max)
{
  auto abs_lhs = (uint64_t)std::abs(lhsns);
  auto abs_rhs = (uint64_t)std::abs(rhsns);

  if (lhsns > 0 && rhsns < 0) {
    if (abs_lhs + abs_rhs > (uint64_t)max) {
      throw std::overflow_error("duration subtraction leads to int64_t overflow");
    }
  } else if (lhsns < 0 && rhsns > 0) {
    if (abs_lhs + abs_rhs > (uint64_t)max) {
      throw std::underflow_error("duration subtraction leads to int64_t underflow");
    }
  }
}

Duration Duration::operator-(const rclcpp::Duration & rhs) const
{
  bounds_check_duration_difference(
    this->rcl_duration_.nanoseconds, rhs.rcl_duration_.nanoseconds,
    std::numeric_limits<rcl_duration_value_t>::max());

  return Duration(rcl_duration_.nanoseconds - rhs.rcl_duration_.nanoseconds);
}

void bounds_check_duration_scale(int64_t dns, double scale, uint64_t max)
{
  auto abs_dns = static_cast<uint64_t>(std::abs(dns));
  auto abs_scale = std::abs(scale);

  if (abs_scale > 1.0 && abs_dns > static_cast<uint64_t>(max / abs_scale)) {
    if ((dns > 0 && scale > 0) || (dns < 0 && scale < 0)) {
      throw std::overflow_error("duration scaling leads to int64_t overflow");
    } else {
      throw std::underflow_error("duration scaling leads to int64_t underflow");
    }
  }
}

Duration Duration::operator*(double scale) const
{
  if (!std::isfinite(scale)) {
    throw std::runtime_error("abnormal scale in rclcpp::Duration");
  }
  bounds_check_duration_scale(
    this->rcl_duration_.nanoseconds, scale, std::numeric_limits<rcl_duration_value_t>::max());
  return Duration(static_cast<rcl_duration_value_t>(rcl_duration_.nanoseconds * scale));
}

rcl_duration_value_t Duration::nanoseconds() const { return rcl_duration_.nanoseconds; }

Duration Duration::max() { return Duration(std::numeric_limits<int32_t>::max(), 999999999); }

double Duration::seconds() const
{
  return std::chrono::duration<double>(std::chrono::nanoseconds(rcl_duration_.nanoseconds)).count();
}

rmw_time_t Duration::to_rmw_time() const
{
  // reuse conversion logic from msg creation
  builtin_interfaces::msg::Duration msg = *this;
  rmw_time_t result;
  result.sec = msg.sec;
  result.nsec = msg.nanosec;
  return result;
}

Duration Duration::from_seconds(double seconds)
{
  return Duration(static_cast<int64_t>(RCL_S_TO_NS(seconds)));
}

}  // namespace rclcpp
