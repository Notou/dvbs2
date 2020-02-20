/*!
 * \file
 * \brief Class tools::Reporter_probe_decstat.
 */
#ifndef REPORTER_PROBE_DECSTAT_HPP_
#define REPORTER_PROBE_DECSTAT_HPP_

#include "Reporter_probe.hpp"

namespace aff3ct
{
namespace tools
{

class Reporter_probe_decstat : public Reporter_probe
{
public:
	Reporter_probe_decstat(const std::string &group_name,
	                       const std::string &group_description,
	                       const int n_frames = 1);

	explicit Reporter_probe_decstat(const std::string &group_name,
	                                const int n_frames = 1);

	virtual ~Reporter_probe_decstat() = default;

	virtual void probe(const std::string &id, const void *data, const std::type_index &datatype, const int frame_id);
};
}
}

#endif /* REPORTER_PROBE_DECSTAT_HPP_ */
