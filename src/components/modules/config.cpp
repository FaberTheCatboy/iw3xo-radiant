#include "std_include.hpp"

namespace components
{
	/**
	 * @brief	load dvars from config and register them as "external" dvars
	 *			the game re-registers them and uses the values of the external dvars \n
	 *			called from radiantapp::on_load_project
	 */
	void config::load_dvars()
	{
		// Load cfg file
		// T5 :: Dvar_Command / Dvar_SetFromStringByNameFromSource

		int loaded_dvar_count = 0;

		std::ifstream cfg;
		if (utils::fs::open_file_homepath("IW3xRadiant", "dvars.cfg", false, cfg))
		{
			game::printf_to_console("[CFG] Loading dvars from config 'bin/IW3xRadiant/dvars.cfg'");

			std::string input;
			std::vector<std::string> args;

			// read line by line
			while (std::getline(cfg, input))
			{
				// ignore comments
				if (input.find("//") != std::string::npos)
				{
					continue;
				}

				// ignore lines not containing ' "'
				if (input.find(" \"") == std::string::npos)
				{
					continue;
				}

#pragma warning( disable : 4305 4309 )
				// split the string on the first space following a " => 3 args (trash)
				args = utils::split(input, ' "');
#pragma warning( default : 4305 4309)

				if (args.size() != 3)
				{
					game::printf_to_console("|-> skipping line: %s :: failed to parse args", input.c_str());
					continue;
				}

				// remove trailing space on dvar name
				utils::rtrim(args[0]);

				// Dvar_SetFromStringByNameFromSource
				game::dvar_s* dvar = game::Dvar_FindVar(args[0].c_str());

				if (dvar)
				{
					// set value from string if dvar exists
					game::Dvar_SetFromStringFromSource(args[1].c_str(), dvar, 0);
				}
				else
				{
					// register external dvar -> reinterpreted when dvars get registered internally
					dvar = game::Dvar_RegisterString(args[0].c_str(), args[1].c_str(), game::dvar_flags::external, "External Dvar");
				}

				loaded_dvar_count++;
			}

			game::printf_to_console("|-> loaded %d dvars from disk.", loaded_dvar_count);
			game::printf_to_console("\n");

			cfg.close();

			// register all addon dvars
			dvars::register_addon_dvars();
			game::glob::radiant_config_loaded = true;
		}
		else
		{
			game::printf_to_console("[CFG] Could not find 'dvars.cfg'. Loading defaults!");

			dvars::register_addon_dvars();
			game::glob::radiant_config_not_found = true;
		}
	}

	/**
	 * @brief	write dvar config on shutdown \n
	 *			called from radiantapp::on_shutdown()
	 */
	void config::write_dvars()
	{
		std::ofstream cfg;

		if (utils::fs::write_file_homepath("IW3xRadiant", "dvars.cfg", false, cfg))
		{
			cfg << "// generated by iw3xo-radiant" << std::endl;

			for (auto iter = 0; iter < *game::dvarCount; ++iter)
			{
				// get the dvar from the "sorted" dvar* list
				const auto dvar = reinterpret_cast<game::dvar_s*>(game::sortedDvars[iter]);

				if (!dvar)
				{
					game::printf_to_console("[ERR]|-> Dvar was invalid! Saving ...");
					break;
				}

				// do not write paths
				if (!_stricmp(dvar->name, "fs_basepath") ||
					!_stricmp(dvar->name, "fs_basegame") ||
					!_stricmp(dvar->name, "fs_game") ||
					!_stricmp(dvar->name, "fs_cdpath") ||
					!_stricmp(dvar->name, "fs_homepath"))
				{
					continue;
				}

				const char* dvar_string = "%s \"%s\"\n";
				const char* dvar_value = game::Dvar_DisplayableValue(dvar);

				if (!dvar_value)
				{
					continue;
				}

				dvar_string = utils::va(dvar_string, dvar->name, dvar_value);
				cfg << dvar_string;
			}

			cfg.close();
		}
		else
		{
			game::Com_Error("Error while writing dvars");
		}
	}
	
	config::config()
	{ }

	config::~config()
	{ }
}
