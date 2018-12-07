#include <fstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <boost/program_options.hpp>

#include "TTCN3.hh"
#include "EtsiTs103097Module.hh"


IEEE1609dot2::CertificateBase *
decodeEtsiTs103097Certificate(const OCTETSTRING& stream)
{
	TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_DEFAULT);
	TTCN_EncDec::clear_error();

	TTCN_Buffer ttcn_buffer(stream);
	IEEE1609dot2::CertificateBase *ret_val = new IEEE1609dot2::CertificateBase;

	ret_val->decode(EtsiTs103097Module::EtsiTs103097Certificate_descr_, ttcn_buffer, TTCN_EncDec::CT_OER);

	if (TTCN_EncDec::get_last_error_type() == TTCN_EncDec::ET_NONE) {
		ttcn_buffer.cut();
	}
	else   {
		delete ret_val;
		ret_val = NULL;
	}

	return ret_val;
}


using namespace boost::program_options;
namespace po  = boost::program_options;

class ToolCommand {
public:
	ToolCommand(std::string const& val): _cmd(val) {
		std::cout << "Command: " << val << '\n';
	};
	std::string  getCMD(void) const { return _cmd;};
private:
	std::string _cmd;
};


void
validate(boost::any& v, 
		std::vector<std::string> const& values,
		ToolCommand* /* target_type */,
		int)
{
	po::validators::check_first_occurrence(v);
	std::string const& cmd = validators::get_single_string(values);

	if (cmd == "text")
		v = boost::any(ToolCommand(cmd));
	else
		throw po::validation_error(validation_error::invalid_option_value);
}


void
on_input_file(std::string in_file)
{
	std::cout << "Input file: " << in_file << '\n';
}

std::vector<unsigned char>
read_bytes( const std::string& path_to_file )
{
	std::ifstream file( path_to_file, std::ios::binary ) ;
	return { std::istream_iterator<unsigned char>(file), std::istream_iterator<unsigned char>() } ;
}


int
main(int argc, const char *argv[])
{
	po::options_description desc{"CmdArguments"};
	variables_map cmd_vm;
	std::string format("");

	try {
		desc.add_options()
			("command", po::value<ToolCommand>(), "PKI ITS tool command")
			("in,i", value<std::string>()->notifier(on_input_file), "input file")
			("format,f", value<std::string>(&format)->default_value("json"), "output format")
			("help,h", "Help screen")
			;

		po::positional_options_description pos_desc;
		pos_desc.add("command", 1);

		po::command_line_parser parser{argc, argv};
		parser.options(desc).positional(pos_desc).allow_unregistered();
		po::parsed_options parsed_options = parser.run();

		po::store(parsed_options, cmd_vm);
		notify(cmd_vm);
	}
	catch (const error &ex)   {
		std::cerr << ex.what() << '\n';
	}

	if (cmd_vm.count("help"))   {
		std::cout << desc << '\n';
	}
	else if (cmd_vm.count("command"))   {
		std::string cmd = cmd_vm["command"].as<ToolCommand>().getCMD();
		std::cout << "CMD: " << cmd << '\n';

		if (!cmd.compare("text"))   {
			if (!cmd_vm.count("in"))   {
				std::cout << "Error: for \"text\" command input file is mandatory!" << "\n";
				exit(-1);
			}

			if (!format.compare("json") && !format.compare("yaml"))   {
				std::cout << "Invalid output format '" << format << "'.\n";
				exit(-1);
			}

			std::vector<unsigned char> in_data = read_bytes(cmd_vm["in"].as<std::string>() );
			std::cout << std::hex ;
			for( unsigned char b : in_data ) std::cout << int(b) << ' ' ;
			std::cout << '\n' << std::hex ;

			IEEE1609dot2::module_object.pre_init_module();
			OCTETSTRING os_18(in_data.size(), in_data.data());
			IEEE1609dot2::CertificateBase *res = decodeEtsiTs103097Certificate(os_18);
			if (res == NULL)   {
				std::cout << "Cannot parse IEEE1609dot2::CertificateBase.\n";
				exit(-1);
			}
			std::cout << "Version " << res->version() << '\n';

			if (format.compare("json") == 0)   {
				TTCN_Logger::begin_event(TTCN_Logger::USER_UNQUALIFIED, 1);
				res->log();
				char *res_log = TTCN_Logger::end_event_log2str().to_JSON_string();
				std::cout << "TTCN-log:\n" << res_log << "\n\n";
				Free(res_log);
			}
			else if (format.compare("yaml") == 0)   {
				YAML::Emitter yaml;
				yaml << YAML::BeginMap << YAML::Key << "EtsiTs103097Certificate" << YAML::Value;
				res->YAML_emitter_write(yaml);
				yaml << YAML::EndMap;
				std::cout << "Here's the output YAML:\n" << yaml.c_str() << "\n\n" << std::flush;
			}
			else   {
				std::cout << "\"CMD\" is required!" << "\n";
				exit(-1);
			}

			delete res;
		}
		else   {
		}
	}
	else   {
		std::cout << "\"CMD\" is required!" << "\n";
		exit(-1);
	}
	
	TTCN_Logger::clear_parameters();
	TTCN_EncDec::clear_error();
	TTCN_Logger::terminate_logger();
	TTCN_Snapshot::terminate();
	TTCN_Runtime::clean_up();
	return 0;
}
