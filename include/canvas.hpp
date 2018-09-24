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

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace hydra {

  /**
   * Representation of a Euclidean coordinate. E.g. the ones used when
   * drawing the canvas.
   */
  struct Euc {

    Euc(double x, double y) {
      this->x = x;
      this->y = y;
    }

    double x = 0.0;
    double y = 0.0;

    friend std::ostream &operator<<(std::ostream &os, const Euc &p) {
      return os << p.to_string();
    }

    std::string to_string() const {
      return std::string("Euc(") + std::to_string(this->x) + ", " +
        std::to_string(this->y) + ")";
    }
  };

  /**
   * Represents a point in the hyperbolic plane using native
   * coordinates.
   */
  struct Pol {

    Pol() {}

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

    /**
     * Converts a polar coordinate to a Euclidean coordinate.
     */
    Euc to_euc() const {
      return Euc(this->r * cos(this->phi), this->r * sin(this->phi));
    }

    void rotate_by(const double angle) {
      this->phi = std::fmod(phi + angle, 2.0 * M_PI);

      while (this->phi < 0.0) {
        this->phi += 2.0 * M_PI;
      }
    }

    static double theta(double r_1, double r_2, double R) {
      try {
        return acos((cosh(r_1) * cosh(r_2) - cosh(R)) / (sinh(r_1) * sinh(r_2)));
      } catch (std::domain_error &de) {}

      return -1.0;
    }
  };

  /**
   * A path object consists of multiple points and can either be
   * closed or not.
   */
  struct Path {

    std::vector<Pol> points;
    bool is_closed = false;

    /**
     * Convenience method to add points to a path.
     */
    void push_back(const Pol &point) {
      this->points.push_back(point);
    }

    int size() const {
      return this->points.size();
    }

    bool empty() const {
      return this->points.empty();
    }

  };

  /**
   * A mark represents a point on the plain. In contrast to a simple
   * coordinate, the mark has a radius.
   */
  struct Circle {
    Circle(const Pol &center, const double radius)
        : center(center), radius(radius) {}

    Pol center;
    double radius;
    bool is_filled;
  };

  class Canvas {
   public:

    Canvas() {}

    /**
     * All paths that are currently on the canvas
     */
    std::vector<Path> paths;

    /**
     * All marks that are currently on the canvas.
     */
    std::vector<Circle> marks;

    /**
     * Impacts the number of points that are used to draw objects. The
     * higher the resolution the more points are used.
     */
    double resolution = 100.0;

    /**
     * Convenience method to add a path to the canvas.
     */
    void add_path(const Path &path);

    /**
     * Convenience method to add a mark to the canvas.
     */
    void add_mark(const Circle &mark);

    /**
     * Removes all marks and paths from the canvas.
     */
    void clear();

    /**
     * Writes the current canvas to file.
     */
    void save_to_file(const std::string &file_name) const;

    /**
     * Creates the content of an Ipe file that represents the current
     * canvas.
     */
    void ipe_canvas_representation(std::string &ipe_representation) const;

    /**
     * Creates a string that represents the passed path.
     */
    static void ipe_path_representation(const Path &path,
                                        std::string &ipe_path_representation);

    /**
     * Creates a string that represents the passed mark.
     */
    static void ipe_circle_representation(
        const Circle &circle, std::string &ipe_circle_representation);

    // "Rendering":

    /**
     * Determines that path that represents the circle with the passed
     * center and radius. The passed resolution determines how
     * detailed the path is. The higher the resolution, the more
     * points are used.
     */
    static void path_for_circle(const Pol &center, double radius,
                                double resolution, Path &path);
  };
  }  // namespace hydra

#endif /* canvas_hpp */
