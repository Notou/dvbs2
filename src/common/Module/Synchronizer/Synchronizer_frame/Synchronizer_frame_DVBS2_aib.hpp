#ifndef SYNCHRONIZER_FRAME_DVBS2_AIB_HPP
#define SYNCHRONIZER_FRAME_DVBS2_AIB_HPP

#include <vector>
#include <complex>

#include "Module/Synchronizer/Synchronizer_frame/Synchronizer_frame.hpp"
#include "Module/Filter/Variable_delay/Variable_delay_cc_naive.hpp"

namespace aff3ct
{
namespace module
{
template <typename R = float>
class Synchronizer_frame_DVBS2_aib : public Synchronizer_frame<R>
{
private:
	const std::vector<std::complex<R> >  conj_SOF_PLSC = {std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,-1), std::complex<R>(0,-1), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,-1), std::complex<R>(0,0), std::complex<R>(0,1), std::complex<R>(0,0), std::complex<R>(0,1)};
	std::complex            <R>   reg_channel;

	const size_t sec_SOF_sz;
	const size_t sec_PLSC_sz;

	std::vector<std::complex<R> > corr_buff;
	std::vector<R               > corr_vec;
	int head;
	int SOF_PLSC_sz;
	Variable_delay_cc_naive<R> output_delay;

public:
	Synchronizer_frame_DVBS2_aib (const int N, const int n_frames = 1);
	virtual ~Synchronizer_frame_DVBS2_aib();
	void step(const std::complex<R>* x_elt, R* y_elt);
	void reset();

protected:
	void _synchronize(const R *X_N1,  R *Y_N2, const int frame_id);

};

}
}

#endif //SYNCHRONIZER_FRAME_DVBS2_AIB_HPP