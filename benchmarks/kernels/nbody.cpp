#include <algorithm>
#include <cmath>
#include <utility>

// Direct gravitational N-body with equal masses, G=1 and softened distances.
// Each vector has three consecutive coordinates in its array.
void body_acceleration(const double *positions, double *acceleration,
                       int bodies, int target) {
  double ax = 0, ay = 0, az = 0;
  double mass = 1.0 / bodies;
  double softening_squared = 0.001;
  for (int source = 0; source < bodies; ++source) {
    if (source == target)
      continue;
    double dx = positions[3 * source] - positions[3 * target];
    double dy = positions[3 * source + 1] - positions[3 * target + 1];
    double dz = positions[3 * source + 2] - positions[3 * target + 2];
    double distance_squared = dx * dx + dy * dy + dz * dz + softening_squared;
    double factor = mass / (distance_squared * std::sqrt(distance_squared));
    ax += dx * factor;
    ay += dy * factor;
    az += dz * factor;
  }
  acceleration[0] = ax;
  acceleration[1] = ay;
  acceleration[2] = az;
}
// One call processes a contiguous group of targets, sharing the source array.
void accelerate_bodies(const double *positions, double *accelerations,
                       int bodies, int first, int block_size) {
  int end = std::min(first + block_size, bodies);
  for (int body = first; body < end; ++body)
    body_acceleration(positions, &accelerations[3 * (body - first)], bodies,
                      body);
}
void compute_accelerations(const double *positions, double *accelerations,
                           int bodies, int block_size) {
  for (int first = 0; first < bodies; first += block_size)
    accelerate_bodies(&positions[0], &accelerations[3 * first], bodies, first,
                      block_size);
}
void drift_body(double *position, double *velocity, const double *acceleration,
                double timestep) {
  for (int axis = 0; axis < 3; ++axis) {
    velocity[axis] += 0.5 * timestep * acceleration[axis];
    position[axis] += timestep * velocity[axis];
  }
}
void drift_bodies(double *positions, double *velocities,
                  const double *accelerations, int bodies, int first,
                  int block_size, double timestep) {
  int count = std::min(block_size, bodies - first);
  for (int body = 0; body < count; ++body)
    drift_body(&positions[3 * body], &velocities[3 * body],
               &accelerations[3 * body], timestep);
}
void drift_particles(double *positions, double *velocities,
                     const double *accelerations, int bodies, int block_size,
                     double timestep) {
  for (int first = 0; first < bodies; first += block_size)
    drift_bodies(&positions[3 * first], &velocities[3 * first],
                 &accelerations[3 * first], bodies, first, block_size,
                 timestep);
}
void kick_body(double *velocity, const double *acceleration, double timestep) {
  for (int axis = 0; axis < 3; ++axis)
    velocity[axis] += 0.5 * timestep * acceleration[axis];
}
void kick_bodies(double *velocities, const double *accelerations, int bodies,
                 int first, int block_size, double timestep) {
  int count = std::min(block_size, bodies - first);
  for (int body = 0; body < count; ++body)
    kick_body(&velocities[3 * body], &accelerations[3 * body], timestep);
}
void kick_particles(double *velocities, const double *accelerations, int bodies,
                    int block_size, double timestep) {
  for (int first = 0; first < bodies; first += block_size)
    kick_bodies(&velocities[3 * first], &accelerations[3 * first], bodies,
                first, block_size, timestep);
}
void simulate_particles(double *positions, double *velocities,
                        double *accelerations, double *next_accelerations,
                        int bodies, int steps, int block_size,
                        double timestep) {
  compute_accelerations(positions, accelerations, bodies, block_size);
  for (int step = 0; step < steps; ++step) {
    drift_particles(positions, velocities, accelerations, bodies, block_size,
                    timestep);
    compute_accelerations(positions, next_accelerations, bodies, block_size);
    kick_particles(velocities, next_accelerations, bodies, block_size,
                   timestep);
    std::swap(accelerations, next_accelerations);
  }
}
