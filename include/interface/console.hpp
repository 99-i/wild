#pragma once
#include "../net/server.hpp"
#include <conio.h>
#include <iostream>
#include <plog/Log.h>
#include <string>

// TODO: fix this horrible and shitty code
namespace wild
{
struct console
{
  private:
	bool quit = false;
	void execute_command()
	{
		std::string command = this->current_command;
		this->current_command = "";
		std::cout << '\n';
		if (command == "rs")
			{
				this->server->restart();
			}
		else if (command == "exit" || command == "stop")
			{
				this->quit = true;
				this->server->graceful_exit();
			}
		else
			{
				PLOGI << "Unknown Command";
			}
	}

  public:
	wild::server *server;
	std::string current_command;
	// start
	void start()
	{
		int c;
		while (!quit)
			{
				c = _getch();
				switch (c)
					{
						// enter
					case 13:
						this->execute_command();
						break;
						// backspace
					case 8:
						if (this->current_command.size() > 0)
							{
								this->current_command =
									this->current_command.substr(
										0, this->current_command.size() - 1);
								this->reset_line();
								place_line();
							}
						break;
					default:
						this->current_command.push_back(c);
						std::cout << (char)c;
						break;
					}
			}
	}
	// remove the > and text so that text can be outputted.
	void reset_line() { std::cout << "\x1b[1M"; }

	// replace the > and the current command.
	void place_line() { std::cout << ">" << current_command; }
};
} // namespace wild
