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

#include <pol.hpp>

namespace hydra {

  /**
   * Representation of a Euclidean coordinate. E.g. the ones used when
   * drawing the canvas.
   */
struct Euc {

  double x = 0.0;
  double y = 0.0;

  Euc(double x, double y) {
    this->x = x;
    this->y = y;
  }

  Euc(const Pol &p) {
    this->x = p.r * cos(p.phi);
    this->y = p.r * sin(p.phi);
  }

  Euc(const Pol &p, double scale) {
    this->x = scale * p.r * cos(p.phi);
    this->y = scale * p.r * sin(p.phi);
  }

  friend std::ostream &operator<<(std::ostream &os, const Euc &p) {
    return os << p.to_string();
  }

  std::string to_string() const {
    return std::string("Euc(") + std::to_string(this->x) + ", " +
           std::to_string(this->y) + ")";
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
     * Since we don't want a hyperbolic length of 1.0 to be
     * represented by 1 pixel, we add a scale. Not sure whether the
     * scale directly translates to the number of pixels though.
     */
    double scale = 15.0;

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
     * Creates a string that represents the passed path, by applying
     * the scale to each point and moving them by the passed offset.
     */
    static void ipe_path_representation(const Path &path,
                                        std::string &ipe_path_representation,
                                        double scale = 1.0,
                                        Euc offset = Euc(0.0, 0.0));

    /**
     * Creates a string that represents the passed mark.
     */
    static void ipe_circle_representation(
        const Circle &circle, std::string &ipe_circle_representation,
        double scale = 1.0, Euc offset = Euc(0.0, 0.0));

    // "Rendering":

    /**
     * Determines that path that represents the circle with the passed
     * center and radius. The passed resolution determines how
     * detailed the path is. The higher the resolution, the more
     * points are used.
     */
    static void path_for_circle(const Pol &center, double radius,
                                double resolution, Path &path);

    static void path_for_line(const Pol &from, const Pol &to, double resolution,
                              Path &path);
  };
  }  // namespace hydra

#endif /* canvas_hpp */
