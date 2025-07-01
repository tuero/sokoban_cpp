// pysokoban.cpp
// Python bindings

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "sokoban/sokoban.h"

namespace py = pybind11;

PYBIND11_MODULE(pysokoban, m) {
    m.doc() = "Sokoban environment module docs.";
    using T = sokoban::SokobanGameState;

    py::class_<T>(m, "SokobanGameState")
        .def(py::init<const std::string &>())
        .def_readonly_static("name", &T::name)
        .def_readonly_static("num_actions", &sokoban::kNumActions)
        .def(py::self == py::self)    // NOLINT (misc-redundant-expression)
        .def(py::self != py::self)    // NOLINT (misc-redundant-expression)
        .def("__hash__", [](const T &self) { return self.get_hash(); })
        .def("__copy__", [](const T &self) { return T(self); })
        .def("__deepcopy__", [](const T &self, py::dict) { return T(self); })
        .def("__repr__",
             [](const T &self) {
                 std::stringstream stream;
                 stream << self;
                 return stream.str();
             })
        .def(py::pickle(
            [](const T &self) {    // __getstate__
                auto s = self.pack();
                return py::make_tuple(s.rows, s.cols, s.agent_idx, s.hash, s.reward_signal, s.board_static, s.is_box);
            },
            [](py::tuple t) -> T {    // __setstate__
                if (t.size() != 7) {
                    throw std::runtime_error("Invalid state");
                }
                T::InternalState s;
                s.rows = t[0].cast<int>();                         // NOLINT(*-magic-numbers)
                s.cols = t[1].cast<int>();                         // NOLINT(*-magic-numbers)
                s.agent_idx = t[2].cast<int>();                    // NOLINT(*-magic-numbers)
                s.hash = t[3].cast<uint64_t>();                    // NOLINT(*-magic-numbers)
                s.reward_signal = t[4].cast<uint64_t>();           // NOLINT(*-magic-numbers)
                s.board_static = t[5].cast<std::vector<int>>();    // NOLINT(*-magic-numbers)
                s.is_box = t[6].cast<std::vector<bool>>();         // NOLINT(*-magic-numbers)
                return {std::move(s)};
            }))
        .def("apply_action",
             [](T &self, int action) {
                 if (action < 0 || action >= T::action_space_size()) {
                     throw std::invalid_argument("Invalid action.");
                 }
                 self.apply_action(static_cast<sokoban::Action>(action));
             })
        .def("is_solution", &T::is_solution)
        .def("is_terminal", &T::is_solution)
        .def("observation_shape", &T::observation_shape)
        .def("observation_shape", [](const T &self) { return self.observation_shape(false); })
        .def("get_observation",
             [](const T &self) {
                 py::array_t<float> out = py::cast(self.get_observation(false));
                 return out.reshape(self.observation_shape(false));
             })
        .def("image_shape", &T::image_shape)
        .def("to_image",
             [](T &self) {
                 py::array_t<uint8_t> out = py::cast(self.to_image());
                 const auto obs_shape = self.observation_shape();
                 return out.reshape({static_cast<py::ssize_t>(obs_shape[1] * sokoban::SPRITE_HEIGHT),
                                     static_cast<py::ssize_t>(obs_shape[2] * sokoban::SPRITE_WIDTH),
                                     static_cast<py::ssize_t>(sokoban::SPRITE_CHANNELS)});
             })
        .def("get_reward_signal", &T::get_reward_signal)
        .def("get_agent_index", &T::get_agent_index)
        .def("get_box_indices", &T::get_box_indices)
        .def("get_empty_goal_indices", &T::get_empty_goal_indices)
        .def("get_solved_goal_indices", &T::get_solved_goal_indices)
        .def("get_all_goal_indices", &T::get_all_goal_indices);
}
