#include <vector>
#include <numeric>
#include <string>
#include <iostream>

#include <aff3ct.hpp>

#include "Factory/DVBS2O/DVBS2O.hpp"

using namespace aff3ct;

int main(int argc, char** argv)
{
	// get the parameter to configure the tools and modules
	auto params = factory::DVBS2O(argc, argv);

	std::cout << "[trace]" << std::endl;
	std::map<std::string,tools::header_list> headers;
	std::vector<factory::Factory*> param_vec;
	param_vec.push_back(&params);
	tools::Header::print_parameters(param_vec, false, std::cout);

	std::vector<std::unique_ptr<tools ::Reporter>>              reporters;
	            std::unique_ptr<tools ::Terminal>               terminal;
	                            tools ::Sigma<>                 noise;

	// the list of the allocated modules for the simulation
	std::vector<const module::Module*> modules;
	std::vector<const module::Module*> modules_each_snr;

	// construct tools
	std::unique_ptr<tools::Constellation           <float>> cstl    (new tools::Constellation_user<float>(params.constellation_file));
	std::unique_ptr<tools::Interleaver_core        <     >> itl_core(factory::DVBS2O::build_itl_core<>(params));
	                tools::BCH_polynomial_generator<      > poly_gen(params.N_bch_unshortened, 12, params.bch_prim_poly);

	// construct modules
	std::unique_ptr<module::Source<>                        > source       (factory::DVBS2O::build_source                   <>(params                 ));
	std::unique_ptr<module::Channel<>                       > channel      (factory::DVBS2O::build_channel                  <>(params                 ));
	std::unique_ptr<module::Scrambler<>                     > bb_scrambler (factory::DVBS2O::build_bb_scrambler             <>(params                 ));
	std::unique_ptr<module::Encoder<>                       > BCH_encoder  (factory::DVBS2O::build_bch_encoder              <>(params, poly_gen       ));
	std::unique_ptr<module::Decoder_HIHO<>                  > BCH_decoder  (factory::DVBS2O::build_bch_decoder              <>(params, poly_gen       ));
	std::unique_ptr<module::Codec_SIHO<>                    > LDPC_cdc     (factory::DVBS2O::build_ldpc_cdc                 <>(params                 ));
	std::unique_ptr<module::Interleaver<>                   > itl_tx       (factory::DVBS2O::build_itl                      <>(params, *itl_core      ));
	std::unique_ptr<module::Interleaver<float,uint32_t>     > itl_rx       (factory::DVBS2O::build_itl<float,uint32_t>        (params, *itl_core      ));
	std::unique_ptr<module::Modem<>                         > modem        (factory::DVBS2O::build_modem                    <>(params, std::move(cstl)));
	std::unique_ptr<module::Filter_UPRRC_ccr_naive<>        > shaping_flt  (factory::DVBS2O::build_uprrc_filter             <>(params)                 );
	std::unique_ptr<module::Multiplier_sine_ccc_naive<>     > freq_shift   (factory::DVBS2O::build_freq_shift               <>(params)                 );
	std::unique_ptr<module::Filter_Farrow_ccr_naive<>       > chn_delay    (factory::DVBS2O::build_channel_delay            <>(params)                 );
	std::unique_ptr<module::Synchronizer_frame_cc_naive<>   > sync_frame   (factory::DVBS2O::build_synchronizer_frame       <>(params)                 );
	std::unique_ptr<module::Synchronizer_LR_cc_naive<>      > sync_lr      (factory::DVBS2O::build_synchronizer_lr          <>(params                 ));
	std::unique_ptr<module::Synchronizer_fine_pf_cc_DVBS2O<>> sync_fine_pf (factory::DVBS2O::build_synchronizer_fine_pf     <>(params                 ));
	std::unique_ptr<module::Framer<>                        > framer       (factory::DVBS2O::build_framer                   <>(params                 ));
	std::unique_ptr<module::Scrambler<float>                > pl_scrambler (factory::DVBS2O::build_pl_scrambler             <>(params                 ));
	std::unique_ptr<module::Filter_unit_delay<>             > delay        (factory::DVBS2O::build_unit_delay               <>(params                 ));
	std::unique_ptr<module::Monitor_BFER<>                  > monitor      (factory::DVBS2O::build_monitor                  <>(params                 ));
	std::unique_ptr<module::Filter_RRC_ccr_naive<>          > matched_flt  (factory::DVBS2O::build_matched_filter           <>(params                 ));
	std::unique_ptr<module::Synchronizer_Gardner_cc_naive<> > sync_gardner (factory::DVBS2O::build_synchronizer_gardner     <>(params                 ));
	std::unique_ptr<module::Multiplier_AGC_cc_naive<>       > mult_agc     (factory::DVBS2O::build_agc_shift                <>(params                 ));
	std::unique_ptr<module::Synchronizer_coarse_freq<>      > sync_coarse_f(factory::DVBS2O::build_synchronizer_coarse_freq <>(params                 ));
	std::unique_ptr<module::Synchronizer_step_mf_cc<>       > sync_step_mf (factory::DVBS2O::build_synchronizer_step_mf_cc  <>(params,
	                                                                                                                           sync_coarse_f.get(),
	                                                                                                                           matched_flt  .get(),
	                                                                                                                           sync_gardner .get()    ));

	auto& LDPC_encoder = LDPC_cdc->get_encoder();
	auto& LDPC_decoder = LDPC_cdc->get_decoder_siho();

	LDPC_encoder ->set_custom_name("LDPC Encoder");
	LDPC_decoder ->set_custom_name("LDPC Decoder");
	BCH_encoder  ->set_custom_name("BCH Encoder" );
	BCH_decoder  ->set_custom_name("BCH Decoder" );
	sync_lr      ->set_custom_name("L&R F Syn"   );
	sync_fine_pf ->set_custom_name("Fine P/F Syn");
	sync_gardner ->set_custom_name("Gardner Syn" );
	sync_frame   ->set_custom_name("Frame Syn"   );
	matched_flt  ->set_custom_name("Matched Flt" );
	shaping_flt  ->set_custom_name("Shaping Flt" );
	sync_coarse_f->set_custom_name("Coarse_Synch");
	sync_step_mf ->set_custom_name("MF_Synch"    );

	// allocate reporters to display results in the terminal
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise     <>(noise   ))); // report the noise values (Es/N0 and Eb/N0)
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER      <>(*monitor))); // report the bit/frame error rates
	reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*monitor))); // report the simulation throughputs

	// allocate a terminal that will display the collected data from the reporters
	terminal = std::unique_ptr<tools::Terminal>(new tools::Terminal_std(reporters));

	// display the legend in the terminal
	terminal->legend();

	// fulfill the list of modules
	modules = { bb_scrambler.get(), BCH_encoder .get(), BCH_decoder .get(), LDPC_encoder .get(),
	            LDPC_decoder.get(), itl_tx      .get(), itl_rx      .get(), modem        .get(),
	            framer      .get(), pl_scrambler.get(), monitor     .get(), freq_shift   .get(),
	            sync_lr     .get(), sync_fine_pf.get(), shaping_flt .get(), chn_delay    .get(),
	            mult_agc    .get(), sync_frame  .get(), delay       .get(), sync_coarse_f.get(),
	            matched_flt .get(), sync_gardner.get(), sync_step_mf.get(), source       .get(),
	            channel     .get(),                                                             };

	// configuration of the module tasks
	for (auto& m : modules)
		for (auto& ta : m->tasks)
		{
			ta->set_autoalloc      (true        ); // enable the automatic allocation of the data in the tasks
			ta->set_autoexec       (false       ); // disable the auto execution mode of the tasks
			ta->set_debug          (params.debug); // disable the debug mode
			ta->set_debug_limit    (100         ); // display only the 16 first bits if the debug mode is enabled
			ta->set_debug_precision(8           );
			ta->set_stats          (params.stats); // enable the statistics
			ta->set_fast           (false       ); // disable the fast mode
		}

	using namespace module;

	// socket binding
	// TX
	(*BCH_encoder )[enc::sck::encode      ::U_K ].bind((*bb_scrambler)[scr::sck::scramble    ::X_N2]);
	(*LDPC_encoder)[enc::sck::encode      ::U_K ].bind((*BCH_encoder )[enc::sck::encode      ::X_N ]);
	(*itl_tx      )[itl::sck::interleave  ::nat ].bind((*LDPC_encoder)[enc::sck::encode      ::X_N ]);
	(*modem       )[mdm::sck::modulate    ::X_N1].bind((*itl_tx      )[itl::sck::interleave  ::itl ]);
	(*framer      )[frm::sck::generate    ::Y_N1].bind((*modem       )[mdm::sck::modulate    ::X_N2]);
	(*pl_scrambler)[scr::sck::scramble    ::X_N1].bind((*framer      )[frm::sck::generate    ::Y_N2]);
	(*shaping_flt )[flt::sck::filter      ::X_N1].bind((*pl_scrambler)[scr::sck::scramble    ::X_N2]);

	// Channel
	(*chn_delay   )[flt::sck::filter      ::X_N1].bind((*shaping_flt )[flt::sck::filter      ::Y_N2]);
	(*freq_shift  )[mlt::sck::imultiply   ::X_N ].bind((*chn_delay   )[flt::sck::filter      ::Y_N2]);

	// RX
	(*sync_lr     )[syn::sck::synchronize ::X_N1].bind((*pl_scrambler)[scr::sck::descramble  ::Y_N2]);
	(*sync_fine_pf)[syn::sck::synchronize ::X_N1].bind((*sync_lr     )[syn::sck::synchronize ::Y_N2]);
	(*framer      )[frm::sck::remove_plh  ::Y_N1].bind((*sync_fine_pf)[syn::sck::synchronize ::Y_N2]);
	(*modem       )[mdm::sck::demodulate  ::Y_N1].bind((*framer      )[frm::sck::remove_plh  ::Y_N2]);
	(*itl_rx      )[itl::sck::deinterleave::itl ].bind((*modem       )[mdm::sck::demodulate  ::Y_N2]);
	(*LDPC_decoder)[dec::sck::decode_siho ::Y_N ].bind((*itl_rx      )[itl::sck::deinterleave::nat ]);
	(*BCH_decoder )[dec::sck::decode_hiho ::Y_N ].bind((*LDPC_decoder)[dec::sck::decode_siho ::V_K ]);
	(*bb_scrambler)[scr::sck::descramble  ::Y_N1].bind((*BCH_decoder )[dec::sck::decode_hiho ::V_K ]);

	// monitor
	(*monitor     )[mnt::sck::check_errors::U   ].bind((*delay       )[flt::sck::filter      ::Y_N2]);
	(*monitor     )[mnt::sck::check_errors::V   ].bind((*bb_scrambler)[scr::sck::descramble  ::Y_N2]);

	// reset the memory of the decoder after the end of each communication
	monitor->add_handler_check(std::bind(&module::Decoder::reset, LDPC_decoder));

	// a loop over the various SNRs
	for (auto ebn0 = params.ebn0_min; ebn0 < params.ebn0_max; ebn0 += params.ebn0_step)
	{
		(*bb_scrambler)[scr::sck::scramble   ::X_N1].bind((*source      )[src::sck::generate   ::U_K ]);
		(*channel     )[chn::sck::add_noise  ::X_N ].bind((*freq_shift  )[mlt::sck::imultiply  ::Z_N ]);
		(*sync_step_mf)[syn::sck::synchronize::X_N1].bind((*channel     )[chn::sck::add_noise  ::Y_N ]);
		(*mult_agc    )[mlt::sck::imultiply  ::X_N ].bind((*sync_step_mf)[syn::sck::synchronize::Y_N2]);
		(*sync_frame  )[syn::sck::synchronize::X_N1].bind((*mult_agc    )[mlt::sck::imultiply  ::Z_N ]);
		(*pl_scrambler)[scr::sck::descramble ::Y_N1].bind((*sync_frame  )[syn::sck::synchronize::Y_N2]);
		(*delay       )[flt::sck::filter     ::X_N1].bind((*source      )[src::sck::generate   ::U_K ]);

		// compute the code rate
		const float R = (float)params.K_bch / (float)params.N_ldpc;

		// compute the current sigma for the channel noise
		const auto esn0  = tools::ebn0_to_esn0 (ebn0, R, params.bps);
		const auto sigma = tools::esn0_to_sigma(esn0);

		noise.set_noise(sigma, ebn0, esn0);

		// update the sigma of the modem and the channel
		LDPC_cdc->set_noise(noise);
		modem   ->set_noise(noise);
		channel ->set_noise(noise);

		shaping_flt  ->reset();
		chn_delay    ->reset();
		freq_shift   ->reset();
		sync_coarse_f->reset();
		sync_coarse_f->disable_update();
		sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 1e-4);
		matched_flt  ->reset();
		sync_gardner ->reset();
		sync_frame   ->reset();
		sync_lr      ->reset();
		sync_fine_pf ->reset();
		delay        ->reset();

		char buf[256];
		char head_lines[]  = "# -------|-------|-----------------|---------|-------------------|-------------------|-------------------";
		char heads[]       = "#  Phase |    m  |        mu       |  Frame  |      PLL CFO      |      LR CFO       |       F CFO       ";
		char pattern[]     = "#    %2d  |  %4d |   %2.6e  |  %6d |    %+2.6e  |    %+2.6e  |    %+2.6e  ";

		if(!params.no_sync_info)
		{
			std::cerr << head_lines << "\n" << heads << "\n" <<head_lines << "\n";
			std::cerr.flush();
		}

		//(*sync_step_mf )[syn::tsk::synchronize].set_debug(true);
		//(*pl_scrambler )[scr::tsk::descramble ].set_debug(true);

		// tasks execution
		int the_delay = 0;
		auto n_phase = 1;

		for (int m = 0; m < 500; m++)
		{
			(*source       )[src::tsk::generate   ].exec();
			(*bb_scrambler )[scr::tsk::scramble   ].exec();
			(*BCH_encoder  )[enc::tsk::encode     ].exec();
			(*LDPC_encoder )[enc::tsk::encode     ].exec();
			(*itl_tx       )[itl::tsk::interleave ].exec();
			(*modem        )[mdm::tsk::modulate   ].exec();
			(*framer       )[frm::tsk::generate   ].exec();
			(*pl_scrambler )[scr::tsk::scramble   ].exec();
			(*shaping_flt  )[flt::tsk::filter     ].exec();
			(*chn_delay    )[flt::tsk::filter     ].exec();
			(*freq_shift   )[mlt::tsk::imultiply  ].exec();
			(*channel      )[chn::tsk::add_noise  ].exec();

			if(n_phase < 3)
			{
				the_delay = sync_step_mf->get_delay();
				(*sync_step_mf )[syn::tsk::synchronize].exec();
				(*mult_agc     )[mlt::tsk::imultiply  ].exec();
				(*sync_frame   )[syn::tsk::synchronize].exec();
				sync_coarse_f->enable_update();
				the_delay = (2*params.pl_frame_size - sync_frame->get_delay() + the_delay) %  params.pl_frame_size;
				sync_coarse_f->set_curr_idx(the_delay);
				(*pl_scrambler )[scr::tsk::descramble].exec();
			}
			else // n_phase == 3
			{
				(*sync_coarse_f)[syn::tsk::synchronize].exec();
				(*matched_flt  )[flt::tsk::filter     ].exec();
				(*sync_gardner )[syn::tsk::synchronize].exec();
				(*mult_agc     )[mlt::tsk::imultiply  ].exec();
				(*sync_frame   )[syn::tsk::synchronize].exec();
				(*pl_scrambler )[scr::tsk::descramble ].exec();
				(*sync_lr      )[syn::tsk::synchronize].exec();
				(*sync_fine_pf )[syn::tsk::synchronize].exec();
			}

			if(!params.no_sync_info)
			{
				sprintf(buf, pattern, n_phase, m+1,
						sync_gardner ->get_mu(),
						sync_coarse_f->get_estimated_freq(),
						the_delay,
						sync_lr      ->get_est_reduced_freq() / (float)params.osf,
						sync_fine_pf ->get_estimated_freq()   / (float)params.osf);
				std::cerr << buf << "\r";
				std::cerr.flush();
			}

			if (m == 149)
			{
				n_phase++;
				sync_coarse_f->set_PLL_coeffs(1, 1/std::sqrt(2.0), 5e-5);
				if(!params.no_sync_info)
					std::cerr << buf << std::endl;
			}

			if (m == 299)
			{
				n_phase++;
				(*sync_coarse_f)[syn::sck::synchronize ::X_N1].bind((*channel      )[chn::sck::add_noise   ::Y_N ]);
				(*matched_flt  )[flt::sck::filter      ::X_N1].bind((*sync_coarse_f)[syn::sck::synchronize ::Y_N2]);
				(*sync_gardner )[syn::sck::synchronize ::X_N1].bind((*matched_flt  )[flt::sck::filter      ::Y_N2]);
				(*mult_agc     )[mlt::sck::imultiply   ::X_N ].bind((*sync_gardner )[syn::sck::synchronize ::Y_N2]);
				(*sync_frame   )[syn::sck::synchronize ::X_N1].bind((*mult_agc     )[mlt::sck::imultiply   ::Z_N ]);
				sync_coarse_f->disable_update();
				if(!params.no_sync_info)
					std::cerr << buf << std::endl;
			}
		}

		if(!params.no_sync_info)
			std::cerr << buf << "\n" << head_lines << "\n";

		monitor->reset();
		if(params.ter_freq != std::chrono::nanoseconds(0))
			terminal->start_temp_report(params.ter_freq);

		for (auto& m : modules)
			for (auto& ta : m->tasks)
				ta->reset_stats();

		int n_frames = 0;
		while (!monitor->is_done() && !terminal->is_interrupt())
		{
			(*source       )[src::tsk::generate    ].exec();
			(*bb_scrambler )[scr::tsk::scramble    ].exec();
			(*BCH_encoder  )[enc::tsk::encode      ].exec();
			(*LDPC_encoder )[enc::tsk::encode      ].exec();
			(*itl_tx       )[itl::tsk::interleave  ].exec();
			(*modem        )[mdm::tsk::modulate    ].exec();
			(*framer       )[frm::tsk::generate    ].exec();
			(*pl_scrambler )[scr::tsk::scramble    ].exec();
			(*shaping_flt  )[flt::tsk::filter      ].exec();
			(*chn_delay    )[flt::tsk::filter      ].exec();
			(*freq_shift   )[mlt::tsk::imultiply   ].exec();
			(*channel      )[chn::tsk::add_noise   ].exec();
			(*sync_coarse_f)[syn::tsk::synchronize ].exec();
			(*matched_flt  )[flt::tsk::filter      ].exec();
			(*sync_gardner )[syn::tsk::synchronize ].exec();
			(*mult_agc     )[mlt::tsk::imultiply   ].exec();
			(*sync_frame   )[syn::tsk::synchronize ].exec();
			(*pl_scrambler )[scr::tsk::descramble  ].exec();
			(*sync_lr      )[syn::tsk::synchronize ].exec();
			(*sync_fine_pf )[syn::tsk::synchronize ].exec();
			(*framer       )[frm::tsk::remove_plh  ].exec();
			(*modem        )[mdm::tsk::demodulate  ].exec();
			(*itl_rx       )[itl::tsk::deinterleave].exec();
			(*LDPC_decoder )[dec::tsk::decode_siho ].exec();
			(*BCH_decoder  )[dec::tsk::decode_hiho ].exec();
			(*bb_scrambler )[scr::tsk::descramble  ].exec();
			(*delay        )[flt::tsk::filter      ].exec();
			(*monitor      )[mnt::tsk::check_errors].exec();

			if (n_frames < 1) // first frame is delayed
				monitor->reset();

			n_frames++;
		}

		// display the performance (BER and FER) in the terminal
		terminal->final_report();
		if(!params.no_sync_info)
			terminal->final_report(std::cerr);

		// reset the monitors and the terminal for the next SNR
		monitor ->reset();
		terminal->reset();

		if (params.stats)
		{
			std::vector<const module::Module*> modules_stats(modules.size());
			for (size_t m = 0; m < modules.size(); m++)
				modules_stats.push_back(modules[m]);

			std::cout << "#" << std::endl;
			const auto ordered = true;
			tools::Stats::show(modules_stats, ordered);

			for (auto& m : modules)
				for (auto& ta : m->tasks)
					ta->reset_stats();

			if (ebn0 + params.ebn0_step < params.ebn0_max)
			{
				std::cout << "#" << std::endl;
				terminal->legend();
			}

		}

	}

	std::cout << "#" << std::endl;
	std::cout << "# End of the simulation" << std::endl;

	return EXIT_SUCCESS;
}