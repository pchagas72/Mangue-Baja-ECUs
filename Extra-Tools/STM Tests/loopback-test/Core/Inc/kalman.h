/*
 * kalman.h
 *
 *  Created on: May 17, 2026
 *      Author: pedro
 */

#ifndef KALMAN_H
#define KALMAN_H

typedef struct {
    float Q_angle;
    float Q_bias;
    float R_measure;
    float angle;
    float bias;
    float rate;
    float P[2][2];
} Kalman_t;

void Kalman_Init(Kalman_t *kalman);
float Kalman_getAngle(Kalman_t *kalman, float newAngle, float newRate, float dt);


#endif /* INC_KALMAN_H_ */
