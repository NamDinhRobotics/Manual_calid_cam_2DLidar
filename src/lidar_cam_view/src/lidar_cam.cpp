#include <cstdlib>
#include <cstdio>
#include <math.h>
#include <algorithm>
#include <map>


#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>

#include "std_msgs/String.h"
#include <sensor_msgs/LaserScan.h>

#include <pcl/common/eigen.h>
#include "laser_geometry/laser_geometry.h"
#include "sensor_msgs/PointCloud.h"
#include <math.h>
#include <angles/angles.h>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/opencv.hpp"

#include "iostream"


bool CAM_HANDLE = false;

cv::Mat Image_pub;
cv::Mat Image_pub_right;

cv::Mat tmp ;
ros::Publisher	scan_pub;
ros::Subscriber	scan_sub;

//ros::Publisher	pub_im;
image_transport::Publisher pub_im;

//image_transport::Publisher  img_pub;

#define WIDTH 1280  //ZED hd usb cam
#define HEIGHT 720 // ZED hd usb cam

using namespace std;
using namespace cv;

void scanCallBack(const	sensor_msgs::LaserScan::ConstPtr&	scan2)
{
    if( CAM_HANDLE)
    {
        tmp = Image_pub.clone();

        Eigen::MatrixXd camera(3,3);
        Eigen::MatrixXd external(3,4);
        Eigen::MatrixXd mid_result(3,4);

        Eigen::MatrixXd ThreeDpoint(4,1);
        Eigen::MatrixXd RealImage(3,1);

        // camera matrix
        camera(0,0) = 698.008;    camera(0,1) = 0;          camera(0,2) = 631.536;
        camera(1,0) = 0;          camera(1,1) = 698.008;    camera(1,2) = 342.469;
        camera(2,0) = 0;          camera(2,1) = 0;          camera(2,2) = 1;


        // calibration result matrix
        external(0,0) = 1.00;           external(0,1) = 0.00;           external(0,2) = 0.00;        external(0,3) = 0.12;// later need to fix Y_c
        external(1,0) = 0.00;           external(1,1) = 1.00;           external(1,2) = 0.00;        external(1,3) = 0.345;
        external(2,0) = 0.00;           external(2,1) = 0.00;           external(2,2) = 1.00;        external(2,3) = -0.0335;

        mid_result = camera*external;

        int	ranges = scan2->ranges.size();
        //populate	the	LaserScan	message
        sensor_msgs::LaserScan	scan;
        scan.header.stamp	    =	scan2->header.stamp;
        scan.header.frame_id	=	scan2->header.frame_id;
        scan.angle_min	        =	scan2->angle_min;
        scan.angle_max	        =	scan2->angle_max;
        scan.angle_increment	=	scan2->angle_increment;
        scan.time_increment	    =	scan2->time_increment;
        scan.range_min	        =	0.0;
        scan.range_max	        =	30.0;

        scan.ranges.resize(ranges);

        for(int	i	=	0;	i	<	ranges;	++i)
        {
            /*
                ThreeDpoint(0,0) = -1*laserCloudIn.points[i]z.y;
                ThreeDpoint(1,0) = -1*laserCloudIn.points[i].z;
                ThreeDpoint(2,0) = laserCloudIn.points[i].x;
                ThreeDpoint(3,0) = 1;
           */
            // cos(angle_min + (double) index * angle_increment);
            double x= scan2->ranges[i] * cos(scan2->angle_min + (double)i * scan2->angle_increment);
            double y= scan2->ranges[i] * sin(scan2->angle_min + (double)i * scan2->angle_increment);
            cout <<"x is:"<<x<<"y is"<<y<<"\n";

            ThreeDpoint(0,0) = -1.0*y;
            ThreeDpoint(1,0) = 0.0;
            ThreeDpoint(2,0) = 1.0*x;
            ThreeDpoint(3,0) = 1.0;
            // projector.projectLaser(scan, cloud_out, -1.0, laser_geometry::channel_option::Index);
            //   scan.ranges[i]	=	scan2->ranges[i]	+	1;
            RealImage = mid_result*ThreeDpoint;
            //  cout << RealImage;
            int u = 0;
            int v = 0;

            if( RealImage(2,0) != 0){
                u = RealImage(0,0)/RealImage(2,0);
                v = RealImage(1,0)/RealImage(2,0);

                //u= camera(0,0)*u+camera(0,2);
                //v= camera(1,1)*v+camera(1,2);
                cout <<u<<"--"<<v<<"next_step"<<"\n";// show the result of coordinators in pixel level
            }


            if(u > 0 && u < WIDTH && v > 0 && v < HEIGHT)
            {
                tmp.at<Vec3b>(v, u) = Vec3b(0,0,255);
            }

            // for( int m=1;m<500; m++)
            //     tmp.at<Vec3b>(200, m) =  Vec3b(0,0,255);
/*
        cv_bridge::CvImage cv_image;
        cv_image.image = tmp;
        cv_image.encoding = "brg8";
        sensor_msgs::Image ros_image;
        cv_image.toImageMsg(ros_image);
*/
            //   img_pub.publish(ros_image);
            //  pub.publish(ros_image);
            //   imshow("lidar_Cam", tmp);
            //  cv::waitKey(10);

        }
        sensor_msgs::ImagePtr msg_img = cv_bridge::CvImage(std_msgs::Header(), "bgr8", tmp).toImageMsg();

        imshow("camera", tmp);
        cv::waitKey(10);


        scan_pub.publish(scan);
        pub_im.publish(msg_img);
    }

}
void  imageCallback_right(const sensor_msgs::ImageConstPtr& msg)
{
    cv_bridge::CvImagePtr cv_ptr;
    CAM_HANDLE = true;

    try
    {
        cv_ptr=cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
        //  cv::imshow("view_cam_lidar", Image_pub);
        //  cv::waitKey(10);
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("Not able to convert sensor_msgs::Image to OpenCV::Mat format %s", e.what());
        return;
    }
    Image_pub_right = cv_ptr->image;

    // imshow("camera", Image_pub);
    //  cv::waitKey(10);
}

void  imageCallback(const sensor_msgs::ImageConstPtr& msg)
{
    cv_bridge::CvImagePtr cv_ptr;
    CAM_HANDLE = true;

  try
  {
    cv_ptr=cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
  //  cv::imshow("view_cam_lidar", Image_pub);
  //  cv::waitKey(10);
  }
  catch (cv_bridge::Exception& e)
  {
      ROS_ERROR("Not able to convert sensor_msgs::Image to OpenCV::Mat format %s", e.what());
      return;
  }
    Image_pub = cv_ptr->image;

  // imshow("camera", Image_pub);
  //  cv::waitKey(10);
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "image_listener");
 // ros::NodeHandle nh;
 // Scan2	scan2;// laser scan handle

    ros::NodeHandle	n;
    //ros::Publisher	scan_pub;
    //ros::Subscriber	scan_sub;
    //void	scanCallBack(const	sensor_msgs::LaserScan::ConstPtr&	scan2);
    //void    imageCallback(const sensor_msgs::ImageConstPtr& msg);

    scan_pub	=	n.advertise<sensor_msgs::LaserScan>("/scan2",10);
    scan_sub	=	n.subscribe<sensor_msgs::LaserScan>("/scan",10,	scanCallBack);

    image_transport::ImageTransport it(n);
    image_transport::Subscriber sub = it.subscribe("/zed/left/image_rect_color", 10, imageCallback);

    image_transport::Subscriber sub_right = it.subscribe("/zed/right/image_rect_color", 10, imageCallback_right);


    pub_im = it.advertise("camera_nam/image", 1);

    cv::startWindowThread();

 // cv::namedWindow("view");

//  image_transport::ImageTransport it(nh);
//  image_transport::Subscriber sub = it.subscribe("/zed/left/image_rect_color", 1, imageCallback);

 // image_transport::Publisher img_pub = it.advertise("camera_nam/image", 1);
   //ros::Publisher pub = nh.advertise<sensor_msgs::Image>("/static_image", 1);
   ros::Rate loop_rate(10);

 // sensor_msgs::ImagePtr msg_im=cv_bridge::CvImage(std_msgs::Header(), "bgr8", Image_pub).toImageMsg();
  //  pub.publish(msg_im);

  while(n.ok())
  {

      ros::spin();
      loop_rate.sleep();
  }

    return EXIT_SUCCESS;
}


