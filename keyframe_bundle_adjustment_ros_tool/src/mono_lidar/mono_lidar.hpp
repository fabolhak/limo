#pragma once

#include <diagnostic_updater/diagnostic_updater.h>
#include <diagnostic_updater/publisher.h>
#include <dynamic_reconfigure/server.h>
#include <ros/ros.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <matches_msg_depth_ros/MatchesMsgWithOutlierFlag.h>
#include <sensor_msgs/CameraInfo.h>

#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/exact_time.h>

#include <Eigen/Eigen>
#include <keyframe_bundle_adjustment/bundle_adjuster_keyframes.hpp>
#include <keyframe_bundle_adjustment/keyframe_selector.hpp>


#include "keyframe_bundle_adjustment_ros_tool/MonoLidarInterface.h"

namespace keyframe_bundle_adjustment_ros_tool {

// Forward declaration of scale provider;
class MotionExtrapolator;

class MonoLidar {

    using Interface = MonoLidarInterface;
    using ReconfigureConfig = MonoLidarConfig;
    using ReconfigureServer = dynamic_reconfigure::Server<ReconfigureConfig>;

    using TrackletsMsg = matches_msg_depth_ros::MatchesMsgWithOutlierFlag;
    using CameraInfoMsg = sensor_msgs::CameraInfo;

    using SyncPolicy = message_filters::sync_policies::ExactTime<TrackletsMsg, CameraInfoMsg>;
    using Synchronizer = message_filters::Synchronizer<SyncPolicy>;

public:
    MonoLidar(ros::NodeHandle, ros::NodeHandle);

    // Define destructor for writing down sequence with points for plotting.
    ~MonoLidar();

private:
    void setupDiagnostics();
    void checkSensorStatus(diagnostic_updater::DiagnosticStatusWrapper&);


    ///@brief process the input from ros, execute whatever, publish it
    void callbackSubscriber(const TrackletsMsg::ConstPtr&, const CameraInfoMsg::ConstPtr& camera_info_msg);

    void reconfigureRequest(const ReconfigureConfig&, uint32_t);

    Interface interface_;
    ReconfigureServer reconfigure_server_;

    tf2_ros::Buffer tf_buffer_;
    tf2_ros::TransformListener tf_listener_;
    tf2_ros::TransformBroadcaster tf_broadcaster_;

    diagnostic_updater::Updater updater_; ///< Diagnostic updater
    // std::unique_ptr<diagnostic_updater::DiagnosedPublisher<std_msgs::Header>>
    // publisherDiagnosed_;
    diagnostic_msgs::DiagnosticStatus diagnostic_status_; ///< Current diagnostic status

    ///@brief sync subscribers
    std::unique_ptr<Synchronizer> sync_;

    ///////////////////////////////////////
    /// \brief bundle_adjuster_ptr_, class for doing the bundle adjustment
    ///
    keyframe_bundle_adjustment::BundleAdjusterKeyframes::Ptr bundle_adjuster_;

    //////////////////////////////////////////////////
    /// \brief keyframe_selector_ptr_, selection for keyframes
    ///
    keyframe_bundle_adjustment::KeyframeSelector keyframe_selector_;

    /**
     * @brief maybeSendPoseTf, if tf frame ids are set in interface, pusblish last keyframe pose in
     * tf
     * @param timestamp, timestamp at which it shall be published
     * @param last_pose, pose toi be published
     */
    void maybeSendPoseTf(ros::Time timestamp, Eigen::Isometry3d pose);

    Eigen::Isometry3d motion_prior_camera_t0_t1; ///< save prior, so that if 5 point fails, we still have a prior

    Eigen::Isometry3d trf_camera_vehicle; ///< extrinsic calibraion of camera defined from vechicle
                                          /// frame to camera frame
    std::shared_ptr<MotionExtrapolator>
        motion_provider_;          ///< Class for calculate scale from previous estimation shared,
                                   /// since uqine has a problem with forward declarations
    ros::Time last_ts_solved_{0.0}; ///< Last timestamp when bundle_adjuster_.solve was called
};
} // namespace keyframe_bundle_adjustment_ros_tool
