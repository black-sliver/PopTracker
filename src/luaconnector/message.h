#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "common.h"

using json = nlohmann::json;

namespace LuaConnector
{
	namespace Net
	{
		struct MessageHeader
		{
			uint32_t size = 0;
		};

		struct Message
		{
			MessageHeader header{};
			std::vector<uint8_t> body;

			Message() {}

			Message(const json& j)
			{
				// dump json to string
				std::string s = j.dump();

				//printf("New message contents: %s\n", s.c_str());

				// get size of string
				std::size_t size = s.size();

				// resize body
				body.resize(size);

				// copy string into message body
				std::memcpy(body.data(), s.c_str(), size);

				// recalculate header
				header.size = body.size();

				// use network byte order
				header.size = htonl(header.size);
			}

			json GetJson() const
			{
				// initialize string
				std::string s;

				// resize string to fit body
				s.resize(body.size() + 1, '\0');

				// copy message body into string
				std::memcpy(&s[0], body.data(), body.size());

				json j;

				try
				{
					j = json::parse(s);
				}
				catch( const json::parse_error& e )
				{
					printf("Failed to parse json: %s\n", e.what());
				}

				return j;
			}

			std::size_t size() const
			{
				return sizeof(MessageHeader) + body.size();
			}
		};
	}
}
#endif
