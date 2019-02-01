/* peter extracted from gps.h Jan 2016 */

struct PMTfxos8700 {
  float yaw;         // 0 < psi < 360  NED (North, East, Down)
  float pitch;       // -90 < theta < 90 NED
  float roll;        // -180 < phi < 180 NED
  float ecompassmag; // magnetic compass = (average of 5 rho readings)
  int diefaceUSS;    // dieface equivalent of yaw, pitch,roll; unsteady state
};
