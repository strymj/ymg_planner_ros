#ifndef DIRECTION_ADJUST_PLANNER_H_
#define DIRECTION_ADJUST_PLANNER_H_

#include <vector>
#include <Eigen/Core>
#include <tf/tf.h>
#include <geometry_msgs/PoseStamped.h>
#include <base_local_planner/local_planner_limits.h>
#include <base_local_planner/trajectory_cost_function.h>
#include <base_local_planner/trajectory_sample_generator.h>
#include <base_local_planner/trajectory_search.h>
#include <ymg_local_planner/obstacle_cost_function_kai.h>
#include <ymg_local_planner/util_functions.h>

namespace ymglp {

class DirAdjustPlanner {

	public:

		~DirAdjustPlanner() {}
		DirAdjustPlanner() {}
		DirAdjustPlanner(
				base_local_planner::ObstacleCostFunctionKai* obstacle_critic,
				UtilFcn* utilfcn);

		void setTolerance(double distance_tolerance, double direction_tolerance,
				double yaw_goal_tolerance, int obstacle_tolerance)
		{/*{{{*/
			distance_tolerance_ = distance_tolerance;
			direction_tolerance_ = direction_tolerance;
			yaw_goal_tolerance_ = yaw_goal_tolerance;
			obstacle_tolerance_ = obstacle_tolerance;
		}/*}}}*/

		void setParameters(
				double sim_time, double sim_granularity, double angular_sim_granularity, double sim_period,
				double path_tolerance, int obstacle_tolerance)
		{/*{{{*/
			sim_time_ = sim_time;
			sim_granularity_ = sim_granularity;
			angular_sim_granularity_ = angular_sim_granularity;
			sim_period_ = sim_period;
			path_tolerance_ = path_tolerance;
			obstacle_tolerance_ = obstacle_tolerance;
		}/*}}}*/

		bool haveToHandle();

		void initialize(
				base_local_planner::LocalPlannerLimits* limits,
				const Eigen::Vector3f& pos,
				const Eigen::Vector3f& vel,
				const Eigen::Vector3f& vsamples);

		bool findBestTrajectory(
				base_local_planner::Trajectory& traj, std::vector<base_local_planner::Trajectory>* all_explored = 0);

		void resetRotateDirection();


	private:

		void defineDirection();

		bool generateTrajectory(
				Eigen::Vector3f pos, Eigen::Vector3f vel, Eigen::Vector3f sample_target_vel,
				base_local_planner::Trajectory& traj);

		Eigen::Vector3f computeNewPositions(
				const Eigen::Vector3f& pos, const Eigen::Vector3f& vel, double dt);

		enum Direction {CCW=1, UNDEFINED=0, CW=-1};
		Direction rotate_direction_;
		double target_direction_;
		bool handle_latch_;
		int obstacle_tolerance_;
		double distance_tolerance_, direction_tolerance_, yaw_goal_tolerance_, path_tolerance_;
		double sim_time_, sim_granularity_, angular_sim_granularity_, sim_period_;
		base_local_planner::LocalPlannerLimits* limits_;
		Eigen::Vector3f pos_, vel_, vsamples_;
		Eigen::Vector3f max_vel_, min_vel_;

		base_local_planner::ObstacleCostFunctionKai* obstacle_critic_;
		UtilFcn* utilfcn_;
};  // class DirAdjustPlanner

} // namespace

#endif 
