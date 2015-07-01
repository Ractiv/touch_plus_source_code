#include "camera_initializer_new.h"

float CameraInitializerNew::exposure_val;
float CameraInitializerNew::exposure_max;

uchar CameraInitializerNew::l_exposure_old;

LowPassFilter CameraInitializerNew::low_pass_filter;

void CameraInitializerNew::init(Camera* camera)
{
	exposure_max = 15;
	exposure_val = exposure_max;
	l_exposure_old = 0;

	camera->disableAutoExposure(Camera::both);
	camera->disableAutoWhiteBalance(Camera::both);
	camera->turnLEDsOn();
	camera->setExposureTime(Camera::both, exposure_val);

	preset1(camera);
}

bool CameraInitializerNew::adjust_exposure(Camera* camera, Mat& image_in)
{
	static int count = 0;

	static bool step0 = false;
	static bool step1 = false;

	if (step0 == true && step1 == true)
		return true;
	else
		++count;

	static Mat image_leds_on = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);
	static Mat image_leds_off = Mat::zeros(HEIGHT_SMALL, WIDTH_SMALL, CV_8UC1);

	if (step0 == false)
	{
		if (count == 10)
		{
			image_leds_on = image_in;
			camera->turnLEDsOff();
		}

		if (count == 20)
			step0 = true;		
	}
	else if (step1 == false)
	{
		image_leds_off = image_in;
		camera->turnLEDsOn();
		step1 = true;
	}

	if (step0 == true && step1 == true)
	{
		vector<uchar> vec_leds_on;
		vector<uchar> vec_leds_off;

		for (int i = 0; i < WIDTH_SMALL; ++i)
			for (int j = HEIGHT_SMALL; j > 0; --j)
			{
				vec_leds_on.push_back(image_leds_on.ptr<uchar>(j, i)[0]);
				vec_leds_off.push_back(image_leds_off.ptr<uchar>(j, i)[0]);
			}

		sort(vec_leds_on.begin(), vec_leds_on.end());
		sort(vec_leds_off.begin(), vec_leds_off.end());

		// const int i_min = vec_leds_on.size() * 0.25;
		// const int i_max = vec_leds_on.size() * 0.75;
		const int i_min = 0;
		const int i_max = vec_leds_on.size() - 1;

		int gray_mean_on = 0;
		int gray_mean_off = 0;
		int gray_mean_count = 0;

		for (int i = i_min; i <= i_max; ++i)
		{
			gray_mean_on += vec_leds_on[i];
			gray_mean_off += vec_leds_off[i];
			++gray_mean_count;
		}

		gray_mean_on /= gray_mean_count;
		gray_mean_off /= gray_mean_count;

		int gray_diff = gray_mean_on - gray_mean_off;
		if (gray_diff > 25)
			gray_diff = 25;
		else if (gray_diff < 0)
			gray_diff = 0;

		exposure_val = map_val(gray_diff, 0, 25, 1, 10);
		camera->setExposureTime(Camera::both, exposure_val);

		float r_val = map_val(gray_diff, 0, 25, 1.5, 2.0);
		camera->setColorGains(0, r_val, 1.0, 2.0);
		camera->setColorGains(1, r_val, 1.0, 2.0);

		// if (exposure_val < 5)
			// preset2(camera);

		COUT << "exposure is " << exposure_val << endl;
		return true;		
	}

	return false;
}

void CameraInitializerNew::preset0(Camera* camera)
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 3.0, 1.5, 2.0);
	camera->setColorGains(1, 3.0, 1.5, 2.0);	
}

void CameraInitializerNew::preset1(Camera* camera)//universal indoors
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 2.0, 1.0, 2.0);
	camera->setColorGains(1, 2.0, 1.0, 2.0);
}

void CameraInitializerNew::preset2(Camera* camera)//universal outdoor
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 1.5, 1.0, 2.0);
	camera->setColorGains(1, 1.5, 1.0, 2.0);
}

void CameraInitializerNew::preset3(Camera* camera)
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 2.0, 1.0, 1.5);
	camera->setColorGains(1, 2.0, 1.0, 1.5);
}

void CameraInitializerNew::preset4(Camera* camera)//afternoon sun facing laptop screen
{
	camera->setGlobalGain(0, 1.0);
	camera->setGlobalGain(1, 1.0);
	camera->setColorGains(0, 3.0, 2.0, 3.0);
	camera->setColorGains(1, 3.0, 2.0, 3.0);
}