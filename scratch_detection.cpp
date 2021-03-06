#include <iostream>
#include <opencv2\opencv.hpp>
#include <vector>


using namespace std;
using namespace cv;


Mat imgRotate(Mat &img, float angle)
{
	Mat retMat = Mat::zeros(390, 1200, CV_8UC1);  // 因为裁剪的是这个size大小的区域
	float anglePI = (float)(angle * CV_PI / 180);
	int xR, yR;
	for (int i = 0; i < retMat.rows; i++)
		for (int j = 0; j < retMat.cols; j++)			
		{	
	        // (i-retMat.rows/2)和(j-retMat.cols/2)是因为图像按(retMat.cols/2,retMat.rows/2)为旋转点旋转的
		    xR = (int)((i - retMat.rows / 2)*cos(anglePI) - (j - retMat.cols / 2)*sin(anglePI) + 0.5);
		    yR = (int)((i - retMat.rows / 2)*sin(anglePI) + (j - retMat.cols / 2)*cos(anglePI) + 0.5);
		    xR += img.rows / 2;	 
			// xR和yR表示在计算完之后，需要再次还原到相对左上角原点的旧坐标
		    yR += img.cols / 2;
		    if (xR >= img.rows || yR >= img.cols || xR <= 0 || yR <= 0)
			{
				retMat.at<uchar>(i, j) = uchar(0);  
				// 如果是彩色图用reMat.at<Vec3b>(i,j) = Vec3b(0, 0);
			}
			else
			{
				retMat.at<uchar>(i, j) = img.at<uchar>(xR, yR);
			}
        }
 return retMat;
}


void colsNormalization(Mat src, Mat& out)  // 去掉钢条的边
{
	out = src.clone();
	int value;
	int count=100;   
	// 可以理解为钢条边界白色点最小长度
	int pixel=5;	
	// 像素阈值
	if (src.type() == CV_8UC1)
	{
		for (size_t i = 0; i < src.cols; i++)
		{
			int Count = 0;
			for (size_t j = 0; j < src.rows; j++)
			{				
				value = src.at<uchar>(j, i); 
				// .at<>(行,列)  灰度图用uchar，彩色图用Vec3
				if (value > pixel)
				{
					Count++;	
					// 像素点大于阈值的计数器
				}
			}
			if (Count > count)
			{
				for (size_t n = 0; n < src.rows; n++)
				{
					out.at<uchar>(n, i) = 0;	
					// 全列置0
				}
			}
		}
	}
}

int main()
{
	Mat input_img = imread("G:\\detect_img\\5.bmp");  // size(1440,1920);
	
	Mat gray_out;
	cvtColor(input_img, gray_out, CV_BGR2GRAY);
	
	Mat ROI_img;
	ROI_img = gray_out(Rect(330, 1050, 1200, 390));

	Mat gaussian_out;
	GaussianBlur(ROI_img, gaussian_out, Size(11, 11), 1, 1);
	
	Mat median_out;
	medianBlur(gaussian_out, median_out, 5);
	
	Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));
	Mat dilate_out, erode_out;
	dilate(median_out, dilate_out, element);
	erode(dilate_out, erode_out, element);
	
	Mat thresh_out;
	adaptiveThreshold(erode_out, thresh_out, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 25, -15);
	namedWindow("aa", WINDOW_NORMAL);
	resizeWindow("aa", 500, 500);
	imshow("aa", thresh_out);
	
	vector<Vec4i> lines;
	double theta;
	HoughLinesP(thresh_out, lines, 1, CV_PI/180, 50, 300, 300);  
	// 为了求钢条倾斜的角度
	double sum = 0;
	for (size_t i = 0; i < lines.size(); i++)
	{
		//Vec4i l = lines[i];
		//line(thresh_out, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(55, 100, 195), 1, CV_AA);
		theta = CV_PI/180*lines[i][1];  // 弧度转换角度
		sum += theta;
	}
	double average = sum / lines.size();
	double angle = average - 2.0;  
	// 钢条倾斜角

	Mat rotate_img;
	rotate_img = imgRotate(thresh_out, -angle);  
	// 负数代表逆时针旋转

	Mat gaussian;
	GaussianBlur(rotate_img, gaussian, Size(15, 15), 1, 1);
	
	Mat normalization_out;
	colsNormalization(gaussian, normalization_out);
	namedWindow("normalization", WINDOW_NORMAL);
	resizeWindow("normalization", 500, 500);
	imshow("normlization", normalization_out);
	
	Mat rotate_Img;  // 得到的划痕区域
	rotate_Img = imgRotate(normalization_out, angle);


	//****************************************************划痕区域以提取出来************************************************

	Mat dst = Mat::zeros(input_img.size(), CV_8UC1);
	Mat mask = rotate_Img;

	Mat imgROI = dst(Rect(330, 1050, 1200, 390));
	mask.copyTo(imgROI, mask);  
	// 将划痕放到原尺寸大小的全0矩阵中

	vector<Vec4i> Lines;
	HoughLinesP(dst, Lines, 1, CV_PI / 180, 150, 5, 5);  // 找划痕
	for (size_t i = 0; i < Lines.size(); i++)
	{
		Vec4i l = Lines[i];
		line(dst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255, 255, 255), 1, CV_AA);
	}
	cvtColor(dst, dst, CV_GRAY2BGR);
	
	Mat Dst;
	vector<Mat> mv;
	split(dst, mv);
	for (int i = 0; i < mv[0].rows; i++)
	{
		uchar* data = mv[0].ptr<uchar>(i);
		uchar* data1 = mv[1].ptr<uchar>(i);
		for (int j = 0; j < mv[0].cols; j++)
		{
			data[j] = 0 * data[j];
			data1[j] = 0;
		}
	}
	merge(mv, Dst);  // 划痕已染红

	Mat out;
	add(input_img, Dst, out);
	// 划痕变色图与原图像相加
	namedWindow("out", WINDOW_NORMAL);
	imshow("out", out);
	waitKey(0);
}
