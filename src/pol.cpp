//
//  pol.cpp
//  hydra
//
//  Created by Maximilian Katzmann on 26.09.18.
//

#include <pol.hpp>

namespace hydra {

Pol::Pol(double r, double phi) : r(r), phi(phi) { normalize_phi(); }

Pol::Pol(const Pol &p) : r(p.r), phi(p.phi) { normalize_phi(); }

void Pol::normalize_phi() {
  while (this->phi < 0.0) {
    this->phi += 2.0 * M_PI;
  }

  while (this->phi > 2.0 * M_PI) {
    this->phi -= 2.0 * M_PI;
  }
}

std::string Pol::to_string() const {
  return std::string("Pol(") + std::to_string(this->r) + ", " +
         std::to_string(this->phi) + ")";
}

void Pol::rotate_by(const double angle) {
  this->phi = std::fmod(phi + angle, 2.0 * M_PI);

  while (this->phi < 0.0) {
    this->phi += 2.0 * M_PI;
  }
}

void Pol::translate_horizontally_by(const double distance) {
  /**
   * If we do not actually translate, we don't need to do anything.
   */
  if (distance == 0.0) {
    return;
  }

  /**
   * Depending on whether or not this point lies on the x-axis, we
   * have to do different things.
   */
  if (this->phi != M_PI && this->phi != 0.0) {
    /**
     * Get a copy of the original point.
     */
    Pol original_point(this->r, this->phi);

    /**
     * Determine the reference point used for the translation.
     */
    Pol reference_point(fabs(distance), 0.0);

    if (distance > 0.0) {
      reference_point.phi = M_PI;
    }

    /**
     * If the coordinate is below the x-axis, we mirror the point,
     * on the x-axis, which makes things easier.
     */
    if (original_point.phi > M_PI) {
      this->phi = (2.0 * M_PI - this->phi);
    }

    /**
     * The radial coordinate is simply the distance between the
     * reference point and the coordinate.
     */
    double radial_coordinate = this->distance_to(reference_point);

    /**
     * Determine the angular coordinate.
     */
    double angular_coordinate = 0.0;

    double enumerator =
        (cosh(fabs(distance)) * cosh(radial_coordinate)) - cosh(this->r);
    double denominator = sinh(fabs(distance)) * sinh(radial_coordinate);

    try {
      angular_coordinate = acos(enumerator / denominator);
    } catch (std::domain_error &de) {
    }

    if (isnan(angular_coordinate)) {
      angular_coordinate = 0.0;
    }

    if (distance < 0.0) {
      angular_coordinate = M_PI - angular_coordinate;
    }

    /**
     * Assign the new values, which are the result of the
     * translation.
     */
    this->r = radial_coordinate;
    this->phi = angular_coordinate;

    /**
     * Mirror back on the x-axis.
     */
    if (original_point.phi > M_PI) {
      this->phi = 2.0 * M_PI - this->phi;
    }

  } else {
    /**
     * The coordinate does lie on the x-axis so the translation only
     * moves it along the axis.
     */
    if (this->phi == 0.0) {
      /**
       * When we translate to far, we pass the origin and are on the
       * other side.
       */
      if (this->r + distance < 0.0) {
        this->phi = M_PI;
      }

      this->r = fabs(this->r + distance);
    } else {
      /**
       * If we move to far, we pass the origin and are on the other
       * side.
       */
      if (this->r - distance < 0.0) {
        this->phi = 0.0;
      }

      this->r = fabs(this->r - distance);
    }
  }
}

double Pol::distance_to(const Pol &other) {
  /**
   * Angular distance.
   */
  double delta_phi = M_PI - fabs(M_PI - fabs(this->phi - other.phi));

  try {
    return acosh((cosh(this->r) * cosh(other.r)) -
                 (sinh(this->r) * sinh(other.r) * cos(delta_phi)));
  } catch (std::domain_error &de) {
    return 0.0;
  }
}

double Pol::theta(double r_1, double r_2, double R) {
  try {
    return acos((cosh(r_1) * cosh(r_2) - cosh(R)) / (sinh(r_1) * sinh(r_2)));
  } catch (std::domain_error &de) {
  }

  return -1.0;
}
}  // namespace hydra
