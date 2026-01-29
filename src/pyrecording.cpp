#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "recording.h"

namespace py = pybind11;

namespace {
lsl::stream_info refresh_stream_info(const lsl::stream_info &info) {
	if (!info.uid().empty()) {
		auto results = lsl::resolve_stream("uid", info.uid(), 1, 1.0);
		if (!results.empty())
			return results.front();
	}
	if (!info.source_id().empty()) {
		auto results = lsl::resolve_stream("source_id", info.source_id(), 1, 1.0);
		if (!results.empty())
			return results.front();
	}
	if (!info.name().empty() && !info.type().empty()) {
		std::string query = "name='" + info.name() + "' and type='" + info.type() + "'";
		auto results = lsl::resolve_stream(query, 1, 1.0);
		if (!results.empty())
			return results.front();
	}
	return info;
}

std::vector<lsl::stream_info> parse_stream_infos(const py::iterable &items) {
	std::vector<lsl::stream_info> infos;
	for (py::handle item : items) {
		std::string xml;
		if (py::isinstance<py::str>(item)) {
			xml = py::cast<std::string>(item);
		} else {
			py::object obj = py::reinterpret_borrow<py::object>(item);
			if (!py::hasattr(obj, "as_xml"))
				throw py::type_error("streams must be XML strings or objects with as_xml()");
			xml = py::cast<std::string>(obj.attr("as_xml")());
		}
		auto info = lsl::stream_info::from_xml(xml);
		infos.emplace_back(refresh_stream_info(info));
	}
	return infos;
}

} // namespace

PYBIND11_MODULE(labrecorder, m) {
	m.doc() = "LabRecorder recording bindings";

	py::class_<recording, std::unique_ptr<recording>>(m, "Recording")
		.def(
			py::init([](const std::string &filename, py::iterable streams,
						 const std::vector<std::string> &watchfor,
						 const std::map<std::string, int> &sync_options,
						 bool collect_offsets) {
				auto stream_infos = parse_stream_infos(streams);
				return std::make_unique<recording>(
					filename, stream_infos, watchfor, sync_options, collect_offsets);
			}),
			py::arg("filename"),
			py::arg("streams"),
			py::arg("watchfor") = std::vector<std::string>{},
			py::arg("sync_options") = std::map<std::string, int>{},
			py::arg("collect_offsets") = true)
		.def("request_stop", &recording::requestStop)
		.def("stop", &recording::requestStop)
		.def(
			"__enter__",
			[](recording &self) -> recording & { return self; },
			py::return_value_policy::reference_internal)
		.def("__exit__", [](recording &self, py::object, py::object, py::object) {
			self.requestStop();
			return false;
		});
}
