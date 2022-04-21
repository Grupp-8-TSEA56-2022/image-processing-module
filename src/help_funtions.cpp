#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#include "help_funtions.h"

#define MIN_INT -5000;
// using namespace std;
using std::cout;
using std::endl;
using std::vector;



// ### Help functions
void print_lines_on_image(vector<cv::Vec2f> lines, cv::Mat& image, cv::Scalar color = cv::Scalar(255, 100, 15)) {
    for (size_t i = 0; i < lines.size(); i++) {
        float rho = lines[i][0];
        float theta = lines[i][1];
        cv::Point pt1, pt2;
        double cos_var = cos(theta);
        double sin_var = sin(theta);
        double x0 = cos_var*rho;
        double y0 = sin_var*rho;

        pt1.x = cvRound(x0 + 1000*(-sin_var));
        pt1.y = cvRound(y0 + 1000*(cos_var));
        pt2.x = cvRound(x0 - 1000*(-sin_var));
        pt2.y = cvRound(y0 - 1000*(cos_var));
        cv::line(image, pt1, pt2, color, 5, cv::LINE_AA);
    }
    return;
}

void remove_negative_rho(vector<cv::Vec2f>& lines) {
    for (unsigned int i = 0; i < lines.size(); i++) {
        if (lines[i][0] < 0) {
            lines[i][1] += PI;
            lines[i][0] = abs(lines[i][0]);
        }
    }
    return;
}

cv::Mat print_circles_on_image(vector<cv::Vec3f> circles, cv::Mat& image) {
    for (unsigned int i=0; i < circles.size(); i++) {
        cv::Point cp;
        cp.x = cvRound(circles[i][0]);
        cp.y = cvRound(circles[i][1]);
        cv::circle(image, cp, cvRound(circles[i][2]), CV_RGB(0, 0, 255), 5);
    }
    return image;
}

bool comp_rho(cv::Vec2f line1, cv::Vec2f line2) {
    return line1[0] > line2[0];
}
bool comp_rho_rev(cv::Vec2f line1, cv::Vec2f line2) {
    return line1[0] < line2[0];
}

float angle_difference(float angle_1, float angle_2) {
    float diff = fmod(abs(angle_1 - angle_2), PI);
    if (diff > PI/2) {
        diff = PI - diff;
    }
    return diff;
}

float circle_line_dist(cv::Vec3f circle, cv::Vec2f line) {
    float rho = line[0];
    float theta = line[1];
    float x = circle[0];
    float y = circle[1];
    float r = circle[2];

    float dist = abs(x*cos(theta) + y*sin(theta) + rho) - r;

    return dist;
}

bool circle_between_lines(cv::Vec2f line1, cv::Vec2f line2, cv::Vec3f circle) {
    float rho_l, rho_r, theta_l, theta_r;
    if (line1[0] > line2[0]) {
        rho_l =  line1[0];
        theta_l = line1[1];
        rho_r =  line2[0];
        theta_r = line2[1];
    } else {
        rho_l =  line2[0];
        theta_l = line2[1];
        rho_r =  line1[0];
        theta_r = line1[1];
    }
    float theta = ((theta_l + theta_r)) / 2;
    float x = circle[0];
    float y = circle[1];
    // float r = circle[2];
    float rho = cos(theta)*x + y*sin(theta);
    return rho_l < rho && rho < 1*rho_r;
}

float line_vertical_deviation(cv::Vec2f line) {
    float dev = angle_difference(line[1], 0);
    return dev;
}

bool line_is_horizontal(cv::Vec2f line) {
    return angle_difference(line[1], PI/2) < 20*PI/180;
}

bool lines_parallell(cv::Vec2f line_1, cv::Vec2f line_2) {
    return abs(angle_difference(line_1[1], line_2[1])) < 20*PI/180;
}

bool  lines_perpendicular(cv::Vec2f line_1, cv::Vec2f line_2) {
    return abs(angle_difference(line_1[1], line_2[1]) - PI/2) < 20*PI/180;
}

float get_rho(cv::Vec2f line) {
    float rho = abs(line[0]);
    return rho;
}

void classify_lines(vector<cv::Vec2f> lines, vector<cv::Vec2f> &side_lines, vector<cv::Vec2f> &stop_lines) {
    if (lines.size() == 1) {
        side_lines = lines;
        return;
    } else {
        for (unsigned int i=0; i < lines.size(); i++) {
            if (abs(line_vertical_deviation(lines[i])) < PI/4) {
                side_lines.push_back(lines[i]);
            }
        }

    //  Add stop_lines that are perpendicular to side_line and closest to button.
        if (side_lines.size() >= 1) {
            sort(lines.begin(), lines.end(), comp_rho);
            for (unsigned int i=0; i < lines.size(); i++) {
                if (lines_perpendicular(lines[i], side_lines[0])) {
                    stop_lines.push_back(lines[i]);
                    return;
                }
            }
        }
    }
    return;
}

cv::Mat perspective_transform_init() {
    cv::Point2f pts1[4];
    pts1[0] = cv::Point2f(240.0f, 50.0f);
    pts1[1] = cv::Point2f(390.0f, 50.0f);
    pts1[2] = cv::Point2f(200.0f, 400.0f);
    pts1[3] = cv::Point2f(430.0f, 400.0f);

    cv::Point2f pts2[4];
    pts2[0] = cv::Point2f(270.0f, 30.0f);
    pts2[1] = cv::Point2f(430.0f, 30.0f);
    pts2[2] = cv::Point2f(270.0f, 500.0f);
    pts2[3] = cv::Point2f(430.0f, 500.0f);
    cv::Mat matrix = cv::getPerspectiveTransform(pts1, pts2);
    return matrix;
}

void perspective_transform(cv::Mat& image, const cv::Mat& matrix) {
    // cv::Mat ipm;
    cv::Size size(image.size().width, image.size().height);
    cv::Scalar value(255, 255, 255);
    warpPerspective(image, image, matrix, size, cv::INTER_LINEAR, cv::BORDER_CONSTANT, value);
    return;
}

void kalman(float &P, float &x_model, float z, float R) {
    float K = P / (P+R);
    x_model = x_model + K*(z-x_model);
    P = (1-K)*P;
}

float average_rho(vector<cv::Vec2f> lines) {
    float sum = 0;
    for (unsigned int i=0; i < lines.size(); i++) {
        sum += lines[i][0];
    }
    return sum/static_cast<float>(lines.size());
}

float average_circle_coord(vector<cv::Vec3f> lines, int position) {
    float sum = 0;
    for (unsigned int i=0; i < lines.size(); i++) {
        sum += lines[i][position];
    }
    return sum/static_cast<float>(lines.size());
}


float average_theta(vector<cv::Vec2f> lines) {
    float x = 0, y = 0;
    for (unsigned int i=0; i < lines.size(); i++) {
        x += cos(lines[i][1]);
        y += sin(lines[i][1]);
    }
    return atan2(y, x);
}

cv::Vec2f average_line(vector<cv::Vec2f> lines) {
    cv::Vec2f line;
    float x = 0, y = 0;
    for (unsigned int i=0; i < lines.size(); i++) {
        line[0] += lines[i][0];  // rho
        x += cos(lines[i][1]);
        y += sin(lines[i][1]);
    }
    line[0] = line[0]/static_cast<float>(lines.size());
    line[1] = atan2(y, x);
    return line;
}

cv::Vec3f average_circle(vector<cv::Vec3f> circles) {
    cv::Vec3f circle;
    float size = static_cast<float>(circles.size());
    for (unsigned int i=0; i < circles.size(); i++) {
        circle[0] += circles[i][0];  // x
        circle[1] += circles[i][1];  // y
        circle[2] += circles[i][2];  // r
    }
    circle[0] = circle[0]/size;
    circle[1] = circle[1]/size;
    circle[2] = circle[2]/size;
    return circle;
}

void get_unique_lines(vector<cv::Vec2f> &lines, float theta_margin = 5, float rho_margin = 60) {
    if (lines.size() <= 1) {
        return;
    }
    remove_negative_rho(lines);
    vector<vector<cv::Vec2f>> line_clusters;

    // Group similar lines
    vector<cv::Vec2f> line;
    line.push_back(lines[0]);
    line_clusters.push_back(line);
    bool sim = false;

    for (unsigned int i=0; i < lines.size(); i++) {
        sim = false;
        for (unsigned int j=0; j < line_clusters.size(); j++) {
            float delta_rho = average_rho(line_clusters[j]);
            float delta_theta = average_theta(line_clusters[j]);
            delta_rho = abs(delta_rho - lines[i][0]);
            delta_theta = angle_difference(delta_theta, lines[i][1]);
            if (delta_theta < theta_margin*PI/180 && delta_rho < rho_margin) {
                line_clusters[j].push_back(lines[i]);
                sim = true;
                break;
            }
        }
        if (sim == false) {
            vector<cv::Vec2f> line2;
            line2.push_back(lines[i]);
            line_clusters.push_back(line2);
        }
    }

    // Merge similar lines by averaging
    vector<cv::Vec2f> unique_lines;
    for (unsigned int i=0; i<line_clusters.size(); i++) {
        cv::Vec2f averaged_line = average_line(line_clusters[i]);
        unique_lines.push_back(averaged_line);
    }
    lines = unique_lines;
    return;
}

vector<cv::Vec3f> get_unique_circles(vector<cv::Vec3f> circles) {
    // Group similar circles
    vector<vector<cv::Vec3f>> circle_clusters;
    vector<cv::Vec3f> circle;
    circle.push_back(circles[0]);
    circle_clusters.push_back(circle);
    bool sim = false;

    for (unsigned int i=0; i < circles.size(); i++) {
        sim = false;
        for (unsigned int j=0; j < circle_clusters.size(); j++) {
            float delta_x = average_circle_coord(circle_clusters[j], 0);
            float delta_y = average_circle_coord(circle_clusters[j], 1);
            float delta_r = average_circle_coord(circle_clusters[j], 2);

            delta_x = abs(delta_x - circles[i][0]);
            delta_y = abs(delta_y - circles[i][1]);
            delta_r = abs(delta_r - circles[i][2]);

            if (delta_x < 50 && delta_y < 50 &&  delta_r < 50) {
                circle_clusters[j].push_back(circles[i]);
                sim = true;
                break;
            }
        }
        if (sim == false) {
            // vector<cv::Vec3f> circle;
            circle.push_back(circles[i]);
            circle_clusters.push_back(circle);
        }
    }
    // Merge similar lines by averaging
    vector<cv::Vec3f> unique_circles;
    for (unsigned int i=0; i < circle_clusters.size(); i++) {
        cv::Vec3f averaged_circle = average_circle(circle_clusters[i]);
        unique_circles.push_back(averaged_circle);
    }
    return unique_circles;
}

// ### Position calculation
float get_lateral_position(vector<cv::Vec2f> side_lines, float image_w, float image_h) {
    sort(side_lines.begin(), side_lines.end(), comp_rho_rev);
    float rho_l = side_lines[0][0];
    float theta_l = side_lines[0][1];
    float rho_r = side_lines[1][0];
    float theta_r = side_lines[1][1];

    // cout << "rho_l: " << rho_l << "\ttheta_l: " << theta_l << endl;
    // cout << "rho_r: " << rho_r << "\ttheta_r: "  << theta_r << endl;

    float x_l = (rho_l - image_h*sin(theta_l)) / cos(theta_l);
    float x_r = (rho_r - image_h*sin(theta_r)) / cos(theta_r);
    // cout <<  "x_l: " << x_l << "\tx_r: " << x_r << endl;

    float b_v = x_r - x_l;
    float b_vl = image_w/2 - x_l;
    float b_vr = x_r - image_w/2;
    // cout << "b_vl: " << b_vl << "\tb_vr: " << b_vr << endl;

    float lat = (b_vr*cos(theta_r) + b_v - b_vl*cos(theta_l))/2;

    // cout << "deviation: " <<  (theta_l+theta_r)/2 <<
    //       "\nl_lat left: " << b_v - b_vl*cos(theta_l) <<
    //       "\tl_lat right: " << b_vr*cos(theta_r);
    return lat;
}

float get_road_angle(vector<cv::Vec2f> side_lines) {
    float theta_l = side_lines[0][1];
    float theta_r = side_lines[1][1];
    return (theta_l+theta_r)/2;
}

int get_stop_line_distance(cv::Vec2f stop_line, float image_w, float image_h) {
    float theta;
    float rho;
    rho = stop_line[0];
    theta = stop_line[1];
    return cvRound(image_h*sin(theta) + image_w/2*cos(theta) - rho);
}

void prefilter(int& lateral_position, int pre_lateral_position, bool& is_down, bool& is_up) {
    if (lateral_position == INT_MIN) {
        cout << "No sidelines found/n";
        return;
    }
    int lateral_diff = lateral_position - pre_lateral_position;
    // cout << mesument_diff << endl;
    if (is_down) {
        if (lateral_diff  > 100) {
            is_down = false;
        } else {
            lateral_position += lateral_diff;
        }
    } else if (is_up) {
        if (lateral_diff  < 100) {
            is_up = false;
        } else {
            lateral_position += lateral_diff;
        }
    } else if (lateral_diff < -100) {
        is_down = true;

    } else if (lateral_diff > 100) {
        is_up = true;
    } else {
         cout << "something wrong with prefilter/n";
    }
}

int image_process(cv::Mat& image, bool print_lines, float &lateral_position, float &road_angle, int &stop_distance) {
    cv::Mat edges, gray, gauss;
    vector<cv::Vec2f> lines, side_lines, stop_lines;
    vector<cv::Vec3f> circles;
    vector<cv::Vec3f> filted_circles;
    float image_height = static_cast<float>(image.size().height);
    float image_width = static_cast<float>(image.size().width);

    // cv::cvtColor(image, gray, cv::COLOR_RGB2GRAY);

    // cv::Mat dst, map1, map2,new_camera_matrix;
    // cv::Size imageSize(cv::Size(image.cols,image.rows));
    
    // // Refining the camera matrix using parameters obtained by calibration
    // new_camera_matrix = cv::getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 1, imageSize, 0);
    
    // // Method 1 to undistort the image
    // cv::undistort(image, dst, new_camera_matrix, distCoeffs, new_camera_matrix);
    
    // // Method 2 to undistort the image
    // cv::initUndistortRectifyMap(cameraMatrix, distCoeffs, cv::Mat(), new_camera_matrix, imageSize, CV_16SC2, map1, map2);
    
    // cv::remap(image, dst, map1, map2, cv::INTER_LINEAR);


    cv::GaussianBlur(image, gauss, cv::Size(3, 3), 0, 0);
    cv::Canny(gauss, edges, 50, 200, 3);
    cv::HoughLines(edges, lines, 1, PI/180, 80, 0, 0);
    get_unique_lines(lines, 10, 58);
    classify_lines(lines, side_lines, stop_lines);
    if (side_lines.size() >= 2) {
        lateral_position = get_lateral_position(side_lines, image_width, image_height);
        road_angle = get_road_angle(side_lines);
        // cout << "l_lat: " << lateral_position << endl;
    } else {
        return -1;
    }
    /*cv::HoughCircles(edges, circles, cv::HOUGH_GRADIENT, 10, //Resulution
                 edges.rows/16,  // Distance between unique circles
                 100, 30, 1, 30 // canny, center, min_r, max_r
    );
    for (unsigned int i=0; i<circles.size(); i++) {
        if (not circle_between_lines(side_lines[0], side_lines[1], circles[i])) {
            filted_circles.push_back(circles[i]);
        }
    }*/
    // output_image = print_circles_on_image(circles, output_image);
    if (print_lines) {
        print_lines_on_image(side_lines, gauss, cv::Scalar(255, 100, 15));
    }

    if (stop_lines.size() != 0) {
        stop_distance = get_stop_line_distance(stop_lines[0], image_width, image_height);
        // cout << "l_s: " << stop_distance << endl;
        if (print_lines) {
            print_lines_on_image(stop_lines, gauss, cv::Scalar(0, 255, 0));
        }
    }
    image = gauss;
    return 1;
}

