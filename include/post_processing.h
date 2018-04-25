#ifndef POST_PROC_H
#define POST_PROC_H

#include "rasteriser_screen.h"

#include <opencv/cv.hpp>

cv::Mat cvUnpackDepthBuffer(screen *s);
void findEdges(cv::Mat &mat);
void maskImage(cv::Mat im1, cv::Mat im2, cv::Mat mask, cv::Mat &out);

#endif
