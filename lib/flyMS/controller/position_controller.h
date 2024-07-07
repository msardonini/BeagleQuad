/**
 * @file position_controller.h
 * @author Mike Sardonini (msardonini@gmail.com)
 * @brief Declaration of PositionController class
 * @version 0.1
 * @date 2022-09-21
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <memory>
#include <mutex>

#include "Eigen/Dense"
#include "flyMS/controller/DigitalFilter.h"
#include "mavlink_v2/common/mavlink.h"
#include "yaml-cpp/yaml.h"

namespace flyMS {

/**
 * @brief The position controller object is responsible for generating roll/pitch/yaw commands given the estimated
 * XYZ position, and the desitred XYZ position. It uses an outer/inner loop structure to control the drone's position
 * in 3D space. All member functions of this object are thread-safe
 *
 */
class PositionController {
 public:
  /**
   * @brief Construct a new Position Controller object. The PID constants used in this constructor are 2 arrays of 3
   * values each. The first array is the output-loop PID constants and the second array is the inner loop PID constants.
   *
   * @param pid_coeffs_x PID coefficients for controlling the X direction
   * @param pid_coeffs_y PID coefficients for controlling the Y direction
   * @param pid_coeffs_z PID coefficients for controlling the Z direction
   * @param RPY_saturation_limits The maximum commanded roll/pitch/yaw values
   */
  PositionController(std::array<std::array<double, 3>, 2> pid_coeffs_x,
                     std::array<std::array<double, 3>, 2> pid_coeffs_y,
                     std::array<std::array<double, 3>, 2> pid_coeffs_z, std::array<float, 3> RPY_saturation_limits);

  /**
   * @brief Construct a new Position Controller object using a YAML::Node
   *
   * @param config_params A yaml node containing all of the parameters found in the main constructors
   */
  PositionController(const YAML::Node &config_params);

  /**
   * @brief Destroy the Position Controller object
   *
   */
  ~PositionController();

  // Leave all other c'tors as the default version
  PositionController(const PositionController &) = default;
  PositionController(PositionController &&) = default;
  PositionController &operator=(PositionController &&) = default;
  PositionController &operator=(const PositionController &) = default;

  /**
   * @brief Receive Visual-Inertial-Odometry information (the XYZ position)
   *
   * @param vio The estimated visual inertial odometry object
   */
  void ReceiveVio(const mavlink_odometry_t &vio);

  /**
   * @brief Gets the current setpoint state and current yaw position
   *
   * @return std::tuple<Eigen::Vector3f, float>. The roll/pitch/yaw setpoint, and the currnet yaw position
   */
  std::tuple<Eigen::Vector3f, float> GetSetpoint() const;

  /**
   * @brief Set the Reference Position. This is the 'commanded' or 'desired' position we want to be in.
   *
   * @param position The reference position in XYZ coordinates
   */
  void SetReferencePosition(const Eigen::Vector3f &position);

  /**
   * @brief Resets the values and filters used in the object.s
   *
   */
  void ResetController();

 private:
  // Setpoint Position, velocity and orientation
  Eigen::Vector3f setpoint_position_;     //< The setpoint position
  Eigen::Vector3f setpoint_velocity_;     //< The setpoint velocity
  Eigen::Vector3f setpoint_orientation_;  //< setpoint orientation
  float yaw_;                             //< The measured yaw component

  std::array<float, 3> RPY_saturation_limits_;  //< Roll pitch and yaw saturdation limits. If the output is larger in
                                                // magnitude than this, it will be clamped

  Eigen::Matrix3f XYZ_to_RollPitchThrottle_;  //< Conversion matrix to map roll, pitch, throttle commands from PID
                                              // output in XYZ frame

  std::array<std::array<DigitalFilter, 2>, 3>
      pid_;  //< Filters to calculate the PID output. First dimention is X,Y,Z controllers, second is outer loop and
             // inner loop. Ex. pid_[0][1] is inner loop for X axis, pid_[3][0] is outer loop for Z axis

  std::unique_ptr<std::mutex> output_mutex_;  //< The mutex to allow thread safety
};

}  // namespace flyMS
