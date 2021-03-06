#include <ros/ros.h>
#include <float.h>
#include <geometry_msgs/PoseStamped.h>
#include <ymg_local_planner/util_functions.h>
#include <tf/tf.h>

namespace ymglp {

UtilFcn::UtilFcn()
{/*{{{*/
	resetFlag();
}/*}}}*/

void UtilFcn::setPlan(const std::vector<geometry_msgs::PoseStamped>& plan)
{/*{{{*/
	if (plan.empty()) {
		ROS_WARN("[UtilFcn] set plan is empty!");
		return;
	}

	plan_ = plan;
	resetFlag();
}/*}}}*/

void UtilFcn::setPose(const geometry_msgs::PoseStamped& pose)
{/*{{{*/
	pose_ = pose;
	resetFlag();
}/*}}}*/

void UtilFcn::setPose(const tf::Stamped<tf::Pose>& pose)
{/*{{{*/
	geometry_msgs::PoseStamped p;
	tf::poseStampedTFToMsg(pose, p);
	setPose(p);
}/*}}}*/

void UtilFcn::setScoringPointOffsetX(double scoring_point_offset_x)
{/*{{{*/
	scoring_point_offset_x_ = scoring_point_offset_x;
}/*}}}*/

void UtilFcn::setSearchDist(double max_dist)
{/*{{{*/
	double max_sq_dist, min_sq_dist = 0.0;
	if (0.0 < scoring_point_offset_x_) {
		max_dist+=scoring_point_offset_x_;
		min_sq_dist = scoring_point_offset_x_ * scoring_point_offset_x_;
	}
	max_sq_dist = max_dist * max_dist;

	double now_dist;
	search_index_bgn_ = 0;
	search_index_end_ = plan_.size()-1;
	for (int i=getNearestIndex()+1; i<plan_.size(); ++i) {
		now_dist = calcSqDist(pose_, plan_[i]);

		if (now_dist < min_sq_dist) {
			search_index_bgn_ = i;
		}

		if (max_sq_dist < now_dist) {
			search_index_end_ = i;
			break;
		}
	}

}/*}}}*/

void UtilFcn::setLocalGoalDist(double local_goal_dist)
{/*{{{*/
	local_goal_dist_ = local_goal_dist + scoring_point_offset_x_;
	has_local_goal_ = false;
	has_straight_ = false;
}/*}}}*/

geometry_msgs::PoseStamped UtilFcn::getPoseStamped()
{/*{{{*/
	return pose_;
}/*}}}*/

int UtilFcn::getNearestIndex()
{/*{{{*/
	if (has_nearest_index_) {
		return nearest_index_;
	}

	nearest_index_ = -10;
	double sq_dist, min_sq_dist = DBL_MAX;

	for (int i=0; i<plan_.size(); ++i) {
		sq_dist = calcSqDist(pose_, plan_[i]);
		if (sq_dist < min_sq_dist) {
			min_sq_dist = sq_dist;
			nearest_index_ = i;
		}
	}

	has_nearest_index_ = true;
	return nearest_index_;
}/*}}}*/

void UtilFcn::getShortenedPlan(double distance, std::vector<geometry_msgs::PoseStamped>& shortened_plan)
{/*{{{*/
	int start_index = getNearestIndex();
	if (start_index < 0) {
		ROS_INFO("cannot get shorten plan.");
		return;
	}

	shortened_plan.clear();
	shortened_plan.push_back(plan_[start_index]);
	double now_distance = 0.0;
	for (int i=start_index+1; i<plan_.size(); ++i) {
		now_distance += UtilFcn::calcDist(plan_[i-1], plan_[i]);
		shortened_plan.push_back(plan_[i]);
		if (distance < now_distance) {
			return;
		}
	}
}/*}}}*/

double UtilFcn::getRobotDirection()
{/*{{{*/
	return tf::getYaw(pose_.pose.orientation);
}/*}}}*/

double UtilFcn::getNearestDirection()
{/*{{{*/
	if (has_nearest_direction_) {
		return nearest_direction_;
	}

	if (getNearestIndex() == plan_.size()-1) {
		nearest_direction_ = tf::getYaw(plan_.back().pose.orientation);
	}
	else {
		nearest_direction_ = atan2(plan_[nearest_index_+1].pose.position.y - plan_[nearest_index_].pose.position.y,
				plan_[nearest_index_+1].pose.position.x - plan_[nearest_index_].pose.position.x);
	}

	return nearest_direction_;
}/*}}}*/

double UtilFcn::getDirectionError()
{/*{{{*/
	// pi to -pi minus pi to -pi  ===>  2*pi to -2*pi
	double error = getRobotDirection() - getNearestDirection();
	return atan2(sin(error), cos(error));
}/*}}}*/

double UtilFcn::getPathDist()
{/*{{{*/
	int nearest_index = getNearestIndex();
	if (nearest_index+1 < plan_.size()) {
		++nearest_index;
	}

	return calcDist(pose_, plan_[nearest_index]);
}/*}}}*/

double UtilFcn::getPathDistHQ(double x, double y)
{/*{{{*/
	double sq_dist, min_sq_dist = DBL_MAX;
	double dx, dy;

	int nearest_index = -1;
	for (int i=search_index_bgn_; i<=search_index_end_; ++i) {
		dx = plan_[i].pose.position.x - x;
		dy = plan_[i].pose.position.y - y;
		sq_dist = dx*dx + dy*dy;
		if (sq_dist < min_sq_dist) {
			min_sq_dist = sq_dist; 
			nearest_index = i;
		}
	}

	if (nearest_index!=-1 && nearest_index+1 < plan_.size()) {
		// path vector
		double v1_x = plan_[nearest_index+1].pose.position.x - plan_[nearest_index].pose.position.x;
		double v1_y = plan_[nearest_index+1].pose.position.y - plan_[nearest_index].pose.position.y;
		// point vector
		double v2_x = x - plan_[nearest_index].pose.position.x;
		double v2_y = y - plan_[nearest_index].pose.position.y;

		double cross = v1_x * v2_y - v1_y * v2_x;
		return fabs(cross / sqrt(v1_x*v1_x + v1_y*v1_y));
	}

	return sqrt(min_sq_dist);
}/*}}}*/

double UtilFcn::getPathDist(double x, double y)
{/*{{{*/
	double sq_dist, min_sq_dist = DBL_MAX;
	double dx, dy;

	for (int i=search_index_bgn_; i<=search_index_end_; ++i) {
		dx = plan_[i].pose.position.x - x;
		dy = plan_[i].pose.position.y - y;
		sq_dist = dx*dx + dy*dy;
		if (sq_dist < min_sq_dist) {
			min_sq_dist = sq_dist; 
		}
	}

	return sqrt(min_sq_dist);
}/*}}}*/

double UtilFcn::getForwardPointPathDist(bool back_mode)
{/*{{{*/
	if (scoring_point_offset_x_ < 0.0) {
		return getPathDist();
	}

	double robot_th = getRobotDirection();
	int sign = 1;
	if (back_mode) sign = -1;
	double forward_x = pose_.pose.position.x + sign*scoring_point_offset_x_ * cos(robot_th);
	double forward_y = pose_.pose.position.y + sign*scoring_point_offset_x_ * sin(robot_th);

	return getPathDist(forward_x, forward_y);
}/*}}}*/

double UtilFcn::scoreTrajDist(base_local_planner::Trajectory& traj, bool back_mode)
{/*{{{*/
	double x, y, th;
	traj.getEndpoint(x, y, th);

	if (0.0 < scoring_point_offset_x_) {
		int sign = 1;
		if (back_mode) sign = -1;
		x += sign*scoring_point_offset_x_ * cos(th);
		y += sign*scoring_point_offset_x_ * sin(th);
	}

	return getPathDist(x, y);
}/*}}}*/

double UtilFcn::scoreTrajInPlaceDist(base_local_planner::Trajectory& traj, double goal_dist, bool back_mode)
{/*{{{*/
	double x, y, th;
	traj.getEndpoint(x, y, th);

	if (0.0 < scoring_point_offset_x_) {
		int sign = 1;
		if (back_mode) sign = -1;
		x += sign*scoring_point_offset_x_ * cos(th);
		y += sign*scoring_point_offset_x_ * sin(th);
	}

	// look goal_dist away from robot
	int start_index = getNearestIndex();
	if (start_index+1 < plan_.size()) {
		++start_index;
	}

	double goal_sq_dist = goal_dist * goal_dist;
	geometry_msgs::PoseStamped endpoint;
	endpoint.pose.position.x = x;
	endpoint.pose.position.y = y;

	for (int i=start_index; i<plan_.size(); ++i) {
		if (goal_sq_dist < calcSqDist(pose_, plan_[i])) {
			return calcDist(endpoint, plan_[i]);
		}
	}

	return calcDist(endpoint, plan_.back());
}/*}}}*/

double UtilFcn::scoreTrajStraightDist(base_local_planner::Trajectory& traj, double goal_dist, bool back_mode)
{/*{{{*/
	if (!has_straight_) {
		Eigen::Vector2d robot_pos;
		robot_pos[0] = pose_.pose.position.x;
		robot_pos[1] = pose_.pose.position.y;
		geometry_msgs::PoseStamped local_goal = getLocalGoal();
		straight_end_[0] = local_goal.pose.position.x;
		straight_end_[1] = local_goal.pose.position.y;
		straight_bgn_ = robot_pos
			+ (straight_end_ - robot_pos) * scoring_point_offset_x_ / (straight_end_ - robot_pos).norm();
	}

	double x, y, th;
	traj.getEndpoint(x, y, th);
	if (0.0 < scoring_point_offset_x_) {
		int sign = 1;
		if (back_mode) sign = -1;
		x += sign*scoring_point_offset_x_ * cos(th);
		y += sign*scoring_point_offset_x_ * sin(th);
	}
	Eigen::Vector2d scoring_point, vec1, vec2;
	scoring_point[0] = x;
	scoring_point[1] = y;

	vec1 = straight_end_ - straight_bgn_;
	vec2 = scoring_point - straight_bgn_;
	
	double ratio = vec1.dot(vec2) / (vec1.norm() * vec1.norm());

	double dist;
	if (1.0 < ratio) {
		dist = (straight_end_ - scoring_point).norm();
	}
	else if (ratio < 0.0) {
		dist = (straight_bgn_ - scoring_point).norm();
	}
	else {
		double s = vec1[0]* vec2[1] - vec1[1]*vec2[0];
		dist = s / vec1.norm();
	}

	return dist;
}/*}}}*/

geometry_msgs::PoseStamped UtilFcn::getLocalGoal()
{/*{{{*/

	if (has_local_goal_) {
		return local_goal_;
	}


	geometry_msgs::PoseStamped pose1, pose2;
	double now_dist = 0.0;
	pose1 = pose_;
	int nearest_index = getNearestIndex();

	if (nearest_index+1 < plan_.size()) {
		++nearest_index;
	}
	pose2 = plan_[nearest_index];

	now_dist += calcDist(pose1, pose2);
	if (!(local_goal_dist_ < now_dist)) {
		for (int i=nearest_index+1; i<plan_.size(); ++i) {
			pose1 = pose2;
			pose2 = plan_[i];
			now_dist += calcDist(pose1, pose2);
			if (local_goal_dist_ < now_dist) {
				break;
			}
		}
	}

	local_goal_ = pose2;
	return local_goal_;
}/*}}}*/

void UtilFcn::tfGlobal2Robot(const Eigen::Vector2d& global, Eigen::Vector2d& robot)
{/*{{{*/
	double theta = -1 * getRobotDirection();
	Eigen::Matrix2d rotation;
	rotation(0,0) =  cos(theta);
	rotation(0,1) = -sin(theta);
	rotation(1,0) =  sin(theta);
	rotation(1,1) =  cos(theta);
	Eigen::Vector2d translation; 
	translation[0] = pose_.pose.position.x;
	translation[1] = pose_.pose.position.y;
	robot = rotation * (global - translation);
}/*}}}*/

void UtilFcn::resetFlag()
{/*{{{*/
	has_nearest_index_ = false;
	has_nearest_direction_ = false;
	has_local_goal_ = false;
	search_index_bgn_ = 0;
	search_index_end_ = plan_.size()-1;
}/*}}}*/

}   // namespace ymglp
