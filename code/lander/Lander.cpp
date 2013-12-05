#include "Controller.h"
#include "LanderStates.h"
#include "QuadcopterState.h"

#include "ros/ros.h"
#include "std_msgs/Float64MultiArray.h"

#include "roscopter/Attitude.h"
#include "roscopter/RC.h"
#include "roscopter/VFR_HUD.h"

using namespace LanderStates;

void attitudeCallback(const roscopter::Attitude::ConstPtr& attitudeMsg) {
	QuadcopterState::updateAttitude(attitudeMsg);
}

/**
* State transitions happen when we receive one of the following:
* New pose estimate
* New RC input
* New quadcopter state estimate
*/

void hudCallback(const roscopter::VFR_HUD::ConstPtr& hudMsg) {
	QuadcopterState::updateTelemetry(hudMsg);

	if (isLanderActive()) {
		if (getState() == States.LAND_LOW) {
			performAction();
		}
		// we don't care about the other states at the moment
	}
}


void poseCallback(const std_msgs::Float64MultiArray::ConstPtr& poseMsg) {
	QuadcopterState::updatePose(poseMsg);

	if (isLanderActive()) {
		if (getState() == States.SEEK_HOME) {
			// We must be above the landing pad (valid pose_estimate)
			setStateAndPerformAction(States.LAND_HIGH);
		} else if (getState() == States.LAND_HIGH) {
			performAction();
		}
	} 
	// not active, don't do anything
}

void rcCallback(const roscopter::RC::ConstPtr& rcMsg)) {
	//TODO: Store RC values?
	if (updateLanderActive(rcMsg.channel[4])) { //CHECK CHANNEL
		if (isLanderActive() && (getState == States.FLYING)) {
			// Activate lander
			setStateAndPerformAction(States.SEEK_HOME);
		} else {
			// Revert to manual control
			setStateAndPerformAction(States.FLYING);
		}
	}
	// else no change
}

ros::Publisher rcPub;

void setStateAndPerformAction(States newState) {
	setState(newState);
	performAction();
}

void performAction() {
	roscopter::RC controlMsg = performStateAction();
	rcPub.publish(controlMsg);
}

int main(int argc, char **argv) {
	ros::init(argc, argv, "Lander");
    ros::NodeHandle node;
    int const queueSize = 1000;
    ros::Rate loopRate(4);  // publish messages at 4 Hz

    // Subscribe to pose estimates
    ros::Subscriber simplePoseSub = \
    	node.subscribe("simplePose", queueSize, poseCallback);

    // Subscribe to RC inputs
    ros::Subscriber rcSub = \
    	node.subscribe("rc", queueSize, rcCallback);

    // Subscribe to quadcopter state
    ros::Subscriber hudSub = \
    	node.subscribe("vfr_hud", queueSize, hudCallback);

    ros::Subscriber attitudeSub = \
    	node.subscribe("attitude", queueSize, attitudeCallback);

    // Set up publisher for RC output
    rcPub = \
    	node.advertise<roscopter::RC>("send_rc", queueSize);

    ros::spin();

    return 0;
}