/*
 * This software is distributed under BSD 3-clause license (see LICENSE file).
 *
 * Authors: Soeren Sonnenburg, Thoralf Klein, Pan Deng, Evgeniy Andreev, 
 *          Viktor Gal, Giovanni De Toni, Heiko Strathmann, Bjoern Esser
 */

#include <shogun/base/init.h>
#include <shogun/lib/memory.h>
#include <shogun/lib/config.h>

#include <shogun/base/Parallel.h>
#include <shogun/base/SGObject.h>
#include <shogun/base/Version.h>
#include <shogun/io/SGIO.h>
#include <shogun/lib/Signal.h>

#include <rxcpp/rx-lite.hpp>

#include <shogun/mathematics/Math.h>
#include <shogun/mathematics/Random.h>
#include <shogun/mathematics/linalg/SGLinalg.h>

#include <csignal>
#include <functional>
#include <stdlib.h>
#include <string.h>
#include <string>

#ifdef HAVE_PROTOBUF
#include <google/protobuf/stubs/common.h>
#endif

namespace shogun
{
	Parallel* sg_parallel=NULL;
	SGIO* sg_io=NULL;
	Version* sg_version=NULL;
	CRandom* sg_rand=NULL;
	std::unique_ptr<CSignal> sg_signal(nullptr);
	std::unique_ptr<SGLinalg> sg_linalg(nullptr);

	// Two global variables to over-ride CMath::fequals for certain
	// serialization
	// unit tests to pass. These should be removed if possible and serialization
	// formats should be fixed.
	float64_t sg_fequals_epsilon = 0.0;
	bool sg_fequals_tolerant = 0.0;

	/// function called to print normal messages
	std::function<void(FILE*, const char*)> sg_print_message(nullptr);

	/// function called to print warning messages
	std::function<void(FILE*, const char*)> sg_print_warning(nullptr);

	/// function called to print error messages
	std::function<void(FILE*, const char*)> sg_print_error(nullptr);

	void init_shogun(
	    const std::function<void(FILE*, const char*)> print_message,
	    const std::function<void(FILE*, const char*)> print_warning,
	    const std::function<void(FILE*, const char*)> print_error)
	{
		if (!sg_io)
			sg_io = new shogun::SGIO();
		if (!sg_parallel)
			sg_parallel=new shogun::Parallel();
		if (!sg_version)
			sg_version = new shogun::Version();
		if (!sg_rand)
			sg_rand = new shogun::CRandom();
		if (!sg_linalg)
			sg_linalg = std::unique_ptr<SGLinalg>(new shogun::SGLinalg());
		if (!sg_signal)
			sg_signal = std::unique_ptr<CSignal>(new shogun::CSignal());

		SG_REF(sg_io);
		SG_REF(sg_parallel);
		SG_REF(sg_version);
		SG_REF(sg_rand);

		sg_print_message=print_message;
		sg_print_warning=print_warning;
		sg_print_error=print_error;

		// Set up signal handler
		std::signal(SIGINT, sg_signal->handler);

		init_from_env();
	}

	void sg_global_print_default(FILE* target, const char* str)
	{
		fprintf(target, "%s", str);
	}

	void init_shogun_with_defaults()
	{
		init_shogun(&sg_global_print_default, &sg_global_print_default,
				&sg_global_print_default);
	}

	void exit_shogun()
	{
		SG_UNREF(sg_rand);
		SG_UNREF(sg_version);
		SG_UNREF(sg_parallel);
		SG_UNREF(sg_io);

		delete CSignal::m_subscriber;
		delete CSignal::m_observable;
		delete CSignal::m_subject;

#ifdef HAVE_PROTOBUF
		::google::protobuf::ShutdownProtobufLibrary();
#endif
	}

	void set_global_io(SGIO* io)
	{
		SG_REF(io);
		SG_UNREF(sg_io);
		sg_io=io;
	}

	SGIO* get_global_io()
	{
		SG_REF(sg_io);
		return sg_io;
	}

	float64_t get_global_fequals_epsilon()
	{
		return sg_fequals_epsilon;
	}

	void set_global_fequals_epsilon(float64_t fequals_epsilon)
	{
		sg_fequals_epsilon = fequals_epsilon;
	}

	void set_global_fequals_tolerant(bool fequals_tolerant)
	{
		sg_fequals_tolerant = fequals_tolerant;
	}

	bool get_global_fequals_tolerant()
	{
		return sg_fequals_tolerant;
	}

	void set_global_parallel(Parallel* parallel)
	{
		SG_REF(parallel);
		SG_UNREF(sg_parallel);
		sg_parallel=parallel;
	}

	Parallel* get_global_parallel()
	{
		SG_REF(sg_parallel);
		return sg_parallel;
	}

	void set_global_version(Version* version)
	{
		SG_REF(version);
		SG_UNREF(sg_version);
		sg_version=version;
	}

	Version* get_global_version()
	{
		SG_REF(sg_version);
		return sg_version;
	}

	void set_global_rand(CRandom* rand)
	{
		SG_REF(rand);
		SG_UNREF(sg_rand);
		sg_rand=rand;
	}

	CRandom* get_global_rand()
	{
		SG_REF(sg_rand);
		return sg_rand;
	}

	CSignal* get_global_signal()
	{
		return sg_signal.get();
	}

#ifndef SWIG // SWIG should skip this part
	SGLinalg* get_global_linalg()
	{
		return sg_linalg.get();
	}
#endif

	void init_from_env()
	{
		char* env_log_val = NULL;
		SGIO* io = get_global_io();
		env_log_val = getenv("SHOGUN_LOG_LEVEL");
		if (env_log_val)
		{
			if(strncmp(env_log_val, "DEBUG", 5) == 0)
				io->set_loglevel(MSG_DEBUG);
			else if(strncmp(env_log_val, "WARN", 4) == 0)
				io->set_loglevel(MSG_WARN);
			else if(strncmp(env_log_val, "ERROR", 5) == 0)
				io->set_loglevel(MSG_ERROR);
		}
		SG_UNREF(io);

		char* env_warnings_val = NULL;
		SGLinalg* linalg = get_global_linalg();
		env_warnings_val = getenv("SHOGUN_GPU_WARNINGS");
		if (env_warnings_val)
		{
			if (strncmp(env_warnings_val, "off", 3) == 0)
				linalg->set_linalg_warnings(false);
		}

		char* env_thread_val = NULL;
		Parallel* parallel = get_global_parallel();
		env_thread_val = getenv("SHOGUN_NUM_THREADS");
		if (env_thread_val)
		{

			try {
				int32_t num_threads = std::stoi(std::string(env_thread_val));
				parallel->set_num_threads(num_threads);
			} catch (...) {
				SG_WARNING("The specified SHOGUN_NUM_THREADS environment (%s)"
				"variable could not be parsed as integer!\n", env_thread_val);
			}
		}
	}
}
