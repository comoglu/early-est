/*
 * File:   student_t.h
 * Author: anthony
 *
 * Created on January 23, 2014, 1:58 PM
 */

// Computes the integral from minus infinity to t of the Student
// t distribution with integer k > 0 degrees of freedom:
double student_t(int k, double t);
// Functional inverse of Student's t distribution
double student_t_inv(int k, double p);

