#ifndef PROBE_OCCURRENCE_HPP_
#define PROBE_OCCURRENCE_HPP_

#include <string>
#include <vector>
#include <typeindex>
#include <cstdint>

#include "Module/Probe/Probe.hpp"
#include "Tools/Reporter/Reporter_probe.hpp"

namespace aff3ct
{
namespace module
{
template <typename T>
class Probe_occurrence : public Probe<T>
{
protected:
	int64_t occurrences;

public:
	Probe_occurrence(const int size, const std::string &col_name, tools::Reporter_probe& reporter, const int n_frames = 1);

	virtual ~Probe_occurrence() = default;

	virtual std::type_index get_datatype() const;

	virtual void probe(const T *in, const int frame_id);

	virtual void reset();

	int64_t get_occurrences() const;
};
}
}

#endif /* PROBE_OCCURRENCE_HPP_ */
