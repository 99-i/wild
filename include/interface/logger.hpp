#pragma once
#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/WinApi.h>
#include "console.hpp"

namespace wild
{
	namespace logging
	{
		wild::console* c;
		class WildFormatter
		{
		public:
			static plog::util::nstring header() // This method returns a header for a new file. In our case it is empty.
			{
				return plog::util::nstring();
			}

			static plog::util::nstring format(const plog::Record& record) // This method returns a string from a record.
			{
				c->reset_line();
				tm t;
				plog::util::localtime_s(&t, &record.getTime().time);

				plog::util::nostringstream ss;

				ss << "[" << std::setfill(L'0') << std::setw(2) << t.tm_mday << "-" << std::setfill(L'0') << std::setw(2) << t.tm_mon << "-" << (t.tm_year + 1900) << "] ";

				//todo: :joy:
				const char* file = record.getFile() + strlen("C:/Users/myfav/Desktop/wild/src/");
				size_t file_len = strlen(file);

				ss << "[" << record.getTid() << "-" << std::wstring(file, file + file_len) << ":" << record.getLine() << "] ";
				ss << "[" << severityToString(record.getSeverity()) << "] ";

				ss << record.getMessage() << "\n";
				return ss.str();
			}
		};

		class WildAppender : public plog::ConsoleAppender<WildFormatter>
		{
		public:
			WildAppender(plog::OutputStream outStream = plog::streamStdOut)
				: plog::ConsoleAppender<WildFormatter>(outStream)
				, m_originalAttr()
			{
				if (this->m_isatty)
				{
					CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
					GetConsoleScreenBufferInfo(this->m_outputHandle, &csbiInfo);

					m_originalAttr = csbiInfo.wAttributes;
				}
			}

			virtual void write(const plog::Record& record) PLOG_OVERRIDE
			{
				plog::util::nstring str = WildFormatter::format(record);
				plog::util::MutexLock lock(this->m_mutex);

				setColor(record.getSeverity());
				this->writestr(str);
				resetColor();
				c->place_line();
			}

		protected:
			void setColor(plog::Severity severity)
			{
				if (this->m_isatty)
				{
					switch (severity)
					{
					case plog::fatal:
						SetConsoleTextAttribute(this->m_outputHandle, plog::foreground::kRed | plog::foreground::kGreen | plog::foreground::kBlue | plog::foreground::kIntensity | plog::background::kRed); // white on red plog::background
						break;

					case plog::error:
						SetConsoleTextAttribute(this->m_outputHandle, static_cast<WORD>(plog::foreground::kRed | plog::foreground::kIntensity | (m_originalAttr & 0xf0))); // red
						break;

					case plog::warning:
						SetConsoleTextAttribute(this->m_outputHandle, static_cast<WORD>(plog::foreground::kRed | plog::foreground::kGreen | plog::foreground::kIntensity | (m_originalAttr & 0xf0))); // yellow
						break;

					case plog::debug:
					case plog::verbose:
						SetConsoleTextAttribute(this->m_outputHandle, static_cast<WORD>(plog::foreground::kGreen | plog::foreground::kBlue | plog::foreground::kIntensity | (m_originalAttr & 0xf0))); // cyan
						break;
					default:
						break;
					}
				}
			}

			void resetColor()
			{
				if (this->m_isatty)
				{
					SetConsoleTextAttribute(this->m_outputHandle, m_originalAttr);
				}
			}

		private:
			WORD m_originalAttr;
		};
	}
}
