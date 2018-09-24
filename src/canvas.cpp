//
//  canvas.cpp
//  hydra
//
//  Created by Maximilian Katzmann on 24.09.18.
//

#include <canvas.hpp>

#include <fstream>

namespace hydra {

void Canvas::add_path(const Path &path) { this->paths.push_back(path); }

void Canvas::add_mark(const Circle &mark) { this->marks.push_back(mark); }

void Canvas::clear() {
  this->paths.clear();
  this->marks.clear();
}

void Canvas::save_to_file(const std::string &file_name) const {
  /**
   * The stream that is used to write to the file.
   */
  std::fstream output_file_stream(file_name, std::fstream::out);

  std::string ipe_representation;
  ipe_canvas_representation(ipe_representation);

  output_file_stream << ipe_representation;
}

void Canvas::ipe_canvas_representation(std::string &ipe_representation) const {
  /**
   * Print the ipe header.
   */
  ipe_representation =
      "<?xml version=\"1.0\"?>\n"
      "<!DOCTYPE ipe SYSTEM \"ipe.dtd\">\n"
      "<ipe version=\"70206\" creator=\"Ipe 7.2.7\">\n"
      "<info created=\"D:20170719160807\" modified=\"D:20170719160807\"/>\n"
      "<ipestyle name=\"basic\">\n"
      "</ipestyle>\n"
      "<page>\n"
      "<layer name=\"alpha\"/>\n"
      "<view layers=\"alpha\" active=\"alpha\"/>\n";

  /**
   * Print Marks.
   */
  for (const Circle &mark : this->marks) {
    std::string circle_representation;
    Canvas::ipe_circle_representation(mark, circle_representation);
  }

  /**
   * Print Paths.
   */
  for (const Path &path : this->paths) {
    std::string path_representation;
    Canvas::ipe_path_representation(path, path_representation);
  }

  /**
   * Print Ipe footer.
   */
  ipe_representation +=
      "</page>\n"
      "</ipe>";
}

/**
 * Creates a string that represents the passed path.
 */
void Canvas::ipe_path_representation(const Path &path,
                                     std::string &ipe_path_representation) {
  /**
   * Only determine the representation if the path is not empty.
   */
  if (!path.empty()) {
    ipe_path_representation =
        std::string("<path stroke=\"") + "black" + "\">\n";

    /**
     * Print the first point of the path.
     */
    Euc point = path.points[0].to_euc();
    ipe_path_representation +=
        std::to_string(point.x) + " " + std::to_string(point.y) + " m\n";

    /**
     * Print the remaining points.
     */
    for (int index = 1; index < path.size(); ++index) {
      point = path.points[index].to_euc();
      ipe_path_representation +=
          std::to_string(point.x) + " " + std::to_string(point.y) + " l\n";
    }

    if (path.is_closed) {
      ipe_path_representation += "h\n";
      ipe_path_representation += "</path>\n";
    }
  }
}

void Canvas::ipe_circle_representation(const Circle &circle,
                                     std::string &ipe_circle_representation) {
  Euc center = circle.center.to_euc();

  ipe_circle_representation =
    std::string("<path stroke=\"") + "black" + "\"";

  if (circle.is_filled) {
    ipe_circle_representation += std::string(" fill=\"") + "black" + "\"";
  }

  ipe_circle_representation +=
      std::string("\">\n") + std::to_string(circle.radius) + " 0 0 " +
      std::to_string(circle.radius) + " " + std::to_string(center.x) + " " +
      std::to_string(center.y) + " e\n</path>\n";
}

void Canvas::path_for_circle(const Pol &center, double radius, double resolution,
                             Path &path) {

  /**
   * A circle path is always closed.
   */
  path.is_closed = true;

  /**
   * If the circle is centered at the origin, we simply create a
   * euclidean circle. It would, however, be better to determine this
   * beforehand and actually use a Circle for this.
   */
  if (center.r == 0.0) {

    /**
     * Create the circle angle-wise.
     */
    double angle = 0.0;
    double angle_step_size = (2.0 * M_PI) / resolution;

    while (angle < 2.0 * M_PI) {
      Pol point(radius, angle);
      path.push_back(point);
      angle += angle_step_size;
    }

    /**
     * We're done drawing the circle and don't need to add anything
     * else to the path.
     */
    return;
  }

  /**
   * If the center is not in the origin, we actually determine the
   * points on the circle.
   *
   * We first determine the points by pretending the node itself had
   * angular coordinate 0.
   */
  double r_min = std::max((radius - center.r), (center.r - radius));
  double r_max = center.r + radius;

  double step_size = (r_max - r_min) / resolution;

  double r = r_max;
  double angle = 0.0;

  Pol point(0.0, 0.0);

  /**
   * When we get closer to the origin, we need finer steps in order to
   * get a smooth circle.
   */
  double additional_detail_threshold = 5.0 * step_size;
  double additional_detail_points = resolution / 5.0;
  double additional_step_size = step_size / additional_detail_points;

  /**
   * First we determine the circle points on one side of the x-axis.
   */
  while (r >= r_min) {

    /**
     * It's actually not a problem if this fails. In this case we
     * simply use the previous angle.
     */
    double new_angle = Pol::theta(center.r, r, radius);
    if (new_angle >= 0.0) {
      angle = new_angle;
    }

    point = Pol(r, angle);
    path.push_back(point);

    /**
     * If we're close to the minimum radius, we need finer steps.
     */
    if (r >= r_min && r - r_min < additional_detail_threshold) {

      double additional_r = r - additional_step_size;

      while (additional_r > r - step_size) {
        double new_angle = Pol::theta(center.r, additional_r, radius);
        if (new_angle >= 0.0) {
          angle = new_angle;
        }

        if (additional_r >= r_min) {
          point = Pol(additional_r, angle);
          path.push_back(point);
        }

        additional_r -= additional_step_size;
      }
    }

    r -= step_size;
  }

  /**
   * Now we add the point on the x-axis. Depending on whether the
   * origin is contained in the circle, the angle of this points is
   * either pi or 0.0
   */
  double inner_point_angle = M_PI;
  if (center.r > radius) {
    inner_point_angle = 0.0;
  }
  path.push_back(Pol(r_min, inner_point_angle));

  /**
   * Now we copy all points by mirroring them on the x-axis. We exclude the
   * first and the last point, as they are lying on the x-axis. To obtain a
   * valid path we need walk from the end of the vector to the start.
   */
  int i = path.size() - 2;
  while (i > 0) {
    point = path.points[i];
    path.push_back(Pol(point.r, (2.0 * M_PI) - point.phi));

    --i;
  }

  /**
   * Finally we rotate all points around the origin to match the angular
   * coordinate of the circle center.
   */
  for (Pol &p : path.points) {
    p.rotate_by(center.phi);
  }
}

}  // namespace hydra
