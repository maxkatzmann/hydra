//
//  pol.hpp
//  hydra
//
//  Used to represent polar coordinates.
//
//  Created by Maximilian Katzmann on 26.09.18.
//

#ifndef DEBUG
#define NDEBUG
#endif

#ifndef pol_hpp
#define pol_hpp

#include <cmath>
#include <iostream>

namespace hydra {

class Pol {
public:

  Pol() {}

  /**
   * The angular and radial coordinates.
   */
  double r = 0.0;
  double phi = 0.0;

  Pol(double r, double phi);
  Pol(const Pol &p);

  friend std::ostream &operator<<(std::ostream &os, const Pol &p) {
    return os << p.to_string();
  }

  std::string to_string() const;

  void rotate_by(const double angle);

  void translate_horizontally_by(const double distance);

  double distance_to(const Pol &other);

  static double theta(double r_1, double r_2, double R);

};
}  // namespace hydra

#endif /* pol_hpp */
