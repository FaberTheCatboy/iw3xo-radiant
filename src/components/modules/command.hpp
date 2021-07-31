#pragma once

namespace components
{
	class command : public component
	{
	public:
		command();
		~command();
		const char* get_name() override { return "command"; };

		static void command_thread();
        static void register_command(const std::string &name, std::function<void(std::vector<std::string>)> cb);
		static void execute_command(std::vector<std::string> args);

	private:
        static std::map<std::string, std::function<void(std::vector<std::string>)>> cmd;
	};
}
