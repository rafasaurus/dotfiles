#include "KALMAN.h"

float getAngle(float newAngle, float newRate, float dt) {
	// KasBot V2  -  Kalman filter module - http://www.x-firm.com/?page_id=145
	// Modified by Kristian Lauszus
	// See my blog post for more information: http://blog.tkjelectronics.dk/2012/09/a-practical-approach-to-kalman-filter-and-how-to-implement-it

	// Discrete Kalman filter time update equations - Time Update ("Predict")
	// Update xhat - Project the state ahead
	/* Step 1 */
	rate = newRate - bias;
	angle += dt * rate;

	// Update estimation error covariance - Project the error covariance ahead
	/* Step 2 */
	P[0][0] += dt * (dt*P[1][1] - P[0][1] - P[1][0] + Q_angle);
	P[0][1] -= dt * P[1][1];
	P[1][0] -= dt * P[1][1];
	P[1][1] += Q_bias * dt;

	// Discrete Kalman filter measurement update equations - Measurement Update ("Correct")
	// Calculate Kalman gain - Compute the Kalman gain
	/* Step 4 */
	float S = P[0][0] + R_measure; // Estimate error
	/* Step 5 */
	float K[2]; // Kalman gain - This is a 2x1 vector
	K[0] = P[0][0] / S;
	K[1] = P[1][0] / S;

	// Calculate angle and bias - Update estimate with measurement zk (newAngle)
	/* Step 3 */
	float y = newAngle - angle; // Angle difference
	/* Step 6 */
	angle += K[0] * y;
	bias += K[1] * y;

	// Calculate estimation error covariance - Update the error covariance
	/* Step 7 */
	float P00_temp = P[0][0];
	float P01_temp = P[0][1];

	P[0][0] -= K[0] * P00_temp;
	P[0][1] -= K[0] * P01_temp;
	P[1][0] -= K[1] * P00_temp;
	P[1][1] -= K[1] * P01_temp;

	return angle;
};
void Kalman_init() {
	/* We will set the variables like so, these can also be tuned by the user */
	Q_angle =0.001f;
	Q_bias = 0.003f;
	R_measure = 0.03f;

	angle = 0.0f; // Reset the angle
	bias = 0.0f; // Reset bias

	P[0][0] = 0.0f; // Since we assume that the bias is 0 and we know the starting angle (use setAngle), the error covariance matrix is set like so - see: http://en.wikipedia.org/wiki/Kalman_filter#Example_application.2C_technical
	P[0][1] = 0.0f;
	P[1][0] = 0.0f;
	P[1][1] = 0.0f;
};
float getAngle_1(float newAngle, float newRate, float dt) {
	// KasBot V2  -  Kalman filter module - http://www.x-firm.com/?page_id=145
	// Modified by Kristian Lauszus
	// See my blog post for more information: http://blog.tkjelectronics.dk/2012/09/a-practical-approach-to-kalman-filter-and-how-to-implement-it

	// Discrete Kalman filter time update equations - Time Update ("Predict")
	// Update xhat - Project the state ahead
	/* Step 1 */
	rate_1 = newRate - bias_1;
	angle_1 += dt * rate_1;

	// Update estimation error covariance - Project the error covariance ahead
	/* Step 2 */
	P_1[0][0] += dt * (dt*P_1[1][1] - P_1[0][1] - P_1[1][0] + Q_angle_1);
	P_1[0][1] -= dt * P_1[1][1];
	P_1[1][0] -= dt * P_1[1][1];
	P_1[1][1] += Q_bias_1 * dt;

	// Discrete Kalman filter measurement update equations - Measurement Update ("Correct")
	// Calculate Kalman gain - Compute the Kalman gain
	/* Step 4 */
	float S_1 = P_1[0][0] + R_measure_1; // Estimate error
	/* Step 5 */
	float K_1[2]; // Kalman gain - This is a 2x1 vector
	K_1[0] = P_1[0][0] / S_1;
	K_1[1] = P_1[1][0] / S_1;

	// Calculate angle and bias - Update estimate with measurement zk (newAngle)
	/* Step 3 */
	float y_1 = newAngle - angle_1; // Angle difference
	/* Step 6 */
	angle_1 += K_1[0] * y_1;
	bias_1 += K_1[1] * y_1;

	// Calculate estimation error covariance - Update the error covariance
	/* Step 7 */
	float P00_temp_1 = P_1[0][0];
	float P01_temp_1 = P_1[0][1];

	P_1[0][0] -= K_1[0] * P00_temp_1;
	P_1[0][1] -= K_1[0] * P01_temp_1;
	P_1[1][0] -= K_1[1] * P00_temp_1;
	P_1[1][1] -= K_1[1] * P01_temp_1;

	return angle_1;
};
void Kalman_init_1() {
	/* We will set the variables like so, these can also be tuned by the user */
	Q_angle_1 =0.01;
	Q_bias_1 = 0.003f;
	R_measure_1 = 0.03f;

	angle_1 = 0.0f; // Reset the angle
	bias_1 = 0.0f; // Reset bias

	P_1[0][0] = 0.0f; // Since we assume that the bias is 0 and we know the starting angle (use setAngle), the error covariance matrix is set like so - see: http://en.wikipedia.org/wiki/Kalman_filter#Example_application.2C_technical
	P_1[0][1] = 0.0f;
	P_1[1][0] = 0.0f;
	P_1[1][1] = 0.0f;
};