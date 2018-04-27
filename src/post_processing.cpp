#include "post_processing.h"
#include <iostream>

using namespace cv;

Mat cvUnpackDepthBuffer(screen *s) {
  Mat mat(s->height, s->width, CV_32F, Scalar(0));
  for (uint32_t y = 0; y < s->height; ++y) {
    for (uint32_t x = 0; x < s->width; ++x) {
      float depth = s->depthBuffer[y * s->width + x];
      mat.at<float>(Point(x, y)) = depth;
    }
  }
  return mat;
}

void findEdges(Mat &src_mat) {
  Mat grad_x, grad_y;
  Mat abs_grad_x, abs_grad_y;

  Sobel(src_mat, grad_x, CV_16S, 1, 0, 3);
  convertScaleAbs(grad_x, abs_grad_x);

  Sobel(src_mat, grad_y, CV_16S, 0, 1, 3);
  convertScaleAbs(grad_y, abs_grad_y);

  addWeighted(abs_grad_x, 0.6, abs_grad_y, 0.6, 0, src_mat);

  threshold(src_mat, src_mat, 100, 255, CV_THRESH_BINARY_INV);
}

void maskImage(cv::Mat im1, cv::Mat im2, cv::Mat mask, cv::Mat &out) {
  #pragma omp parallel for collapse(2)
  for (uint32_t y = 0; y < im1.rows; ++y) {
    for (uint32_t x = 0; x < im2.cols; ++x) {
      float m = mask.at<uchar>(Point(x, y));
      out.at<Vec3b>(Point(x, y)) = m > 0.6 ? im1.at<Vec3b>(Point(x, y)) : im2.at<Vec3b>(Point(x, y));
    }
  }
}
