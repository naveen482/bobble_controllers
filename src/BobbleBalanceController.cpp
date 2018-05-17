/******************************************************************************
 * Document this if it actually works
 * MMM
*******************************************************************************/

#include <sys/mman.h>

#include <bobble_controllers/BobbleBalanceController.h>
#include <pluginlib/class_list_macros.h>

namespace bobble_controllers
{	
	BobbleBalanceController::BobbleBalanceController(void)
	{
	}
	
	BobbleBalanceController::~BobbleBalanceController(void)
	{
		sub_command_.shutdown();
	}
		
	bool BobbleBalanceController::init(hardware_interface::EffortJointInterface *robot,ros::NodeHandle &n)
	{
		node_=n;
		robot_=robot;
		
		XmlRpc::XmlRpcValue joint_names;
		if(!node_.getParam("joints",joint_names))
		{
			ROS_ERROR("No 'joints' in controller. (namespace: %s)",
			        node_.getNamespace().c_str());
			return false;
		}
		
		if(joint_names.getType() != XmlRpc::XmlRpcValue::TypeArray)
		{
			ROS_ERROR("'joints' is not a struct. (namespace: %s)",
			        node_.getNamespace().c_str());
			return false;
		}
		
		for(int i=0; i < joint_names.size();i++)
		{
			XmlRpc::XmlRpcValue &name_value=joint_names[i];
			if(name_value.getType() != XmlRpc::XmlRpcValue::TypeString)
			{
				ROS_ERROR("joints are not strings. (namespace: %s)",
				        node_.getNamespace().c_str());
				return false;
			}
			
			hardware_interface::JointHandle j=robot->
			        getHandle((std::string)name_value);
			joints_.push_back(j);
		}
// Example of registering a call back function to take in external commands
		sub_imu_sensor_ = node_.subscribe("bno055",1000,
		        &BobbleBalanceController::imuCB, this);
//		sub_command_=node_.subscribe("command",1000,
//		        &BobbleBalanceController::commandCB, this);

// Example of dynamically sizing gain matrix based on num joints... for
// generic controllers
//		Kp.resize(chain.getNrOfJoints(),chain.getNrOfJoints());
//		Kd.resize(chain.getNrOfJoints(),chain.getNrOfJoints());
		
		return true;
	}
	
	void BobbleBalanceController::starting(const ros::Time& time)
	{
		Pitch = 0.0;
		PitchDot = 0.0;
		LeftWheelPosition = 0.0;
		LeftWheelVelocity = 0.0;
		RightWheelPosition = 0.0;
		RightWheelVelocity = 0.0;
		Kp.setZero();
		Kd.setZero();
        struct sched_param param;
        param.sched_priority=sched_get_priority_max(SCHED_FIFO);
        if(sched_setscheduler(0,SCHED_FIFO,&param) == -1)
        {
                ROS_WARN("Failed to set real-time scheduler.");
                return;
        }
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1)
                ROS_WARN("Failed to lock memory.");
	}
	
	void BobbleBalanceController::update(const ros::Time& time, const ros::Duration& duration)
	{
        LeftWheelPosition = joints_[0].getPosition();
		LeftWheelVelocity = joints_[0].getVelocity();
		RightWheelPosition = joints_[1].getPosition();
		RightWheelVelocity = joints_[1].getVelocity();
		ROS_INFO("IMU Pitch : %0.3f", Pitch);
		ROS_INFO("IMU Pitch Rate: %0.3f", PitchDot);
		ROS_INFO("Left Wheel Position : %0.3f", LeftWheelPosition);
		ROS_INFO("Left Wheel Velocity : %0.3f", LeftWheelVelocity);
		ROS_INFO("Right Wheel Position : %0.3f", RightWheelPosition);
		ROS_INFO("Right Wheel Velocity : %0.3f", RightWheelVelocity);
/*
		for(unsigned int i=0;i < fext.size();i++) fext[i].Zero();
		
		v.data=ddqr.data+Kp*(qr.data-q.data)+Kd*(dqr.data-dq.data);
		if(idsolver->CartToJnt(q,dq,v,fext,torque) < 0)
		        ROS_ERROR("KDL inverse dynamics solver failed.");
		
		for(unsigned int i=0;i < joints_.size();i++)
		        joints_[i].setCommand(torque(i));
*/
	}

	void BobbleBalanceController::imuCB(const sensor_msgs::Imu::ConstPtr &imuData)
	{
		tf::Quaternion q(imuData->orientation.x, imuData->orientation.y, imuData->orientation.z, imuData->orientation.w);
        tf::Matrix3x3 m(q);
		double roll, yaw;
		PitchDot = imuData->angular_velocity.y;
		m.getRPY(roll, Pitch, yaw);
	}

}

PLUGINLIB_EXPORT_CLASS(bobble_controllers::BobbleBalanceController, controller_interface::ControllerBase)
