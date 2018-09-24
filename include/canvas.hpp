//
//  canvas.hpp
//  hydra
//
//  Used to represent objects in the hyperbolic plane.
//
//  Created by Maximilian Katzmann on 23.09.18.
//

#ifndef DEBUG
#define NDEBUG
#endif

#ifndef canvas_hpp
#define canvas_hpp

#include <iostream>
#include <string>

namespace hydra {

  /**
   * Represents a point in the hyperbolic plane using native
   * coordinates.
   */
  struct Pol {

    Pol(double r, double phi) {
      this->r = r;
      this->phi = phi;
    }

    double r = 0.0;
    double phi = 0.0;

    friend std::ostream &operator<<(std::ostream &os, const Pol &p) {
      return os << p.to_string();
    }

    std::string to_string() const {
      return std::string("Pol(") + std::to_string(this->r) + ", " +
             std::to_string(this->phi) + ")";
    }
  };
}

#endif /* canvas_hpp */
