string(TIMESTAMP ts0)
message(STATUS "Default: ${ts0}")

string(TIMESTAMP ts1 "%Y-%m-%d")
message(STATUS "Date: ${ts1}")

string(TIMESTAMP ts2 "%H:%M:%S")
message(STATUS "Time: ${ts2}")

string(TIMESTAMP ts3 "%Y-%m-%dT%H:%M:%S%z")
message(STATUS "ISO: ${ts3}")

string(TIMESTAMP ts4 "%Y%m%d%H%M%S" UTC)
message(STATUS "UTC compact: ${ts4}")

string(TIMESTAMP ts5 "%b %d, %Y %I:%M %p")
message(STATUS "Friendly: ${ts5}")

string(TIMESTAMP ts6 "%j")
message(STATUS "Day of year: ${ts6}")

string(TIMESTAMP ts7 "%U")
message(STATUS "Week of year: ${ts7}")

string(TIMESTAMP ts8 "%s" UTC)
message(STATUS "Unix epoch: ${ts8}")

# UUID generation
string(UUID uuid1 NAMESPACE 6ba7b810-9dad-11d1-80b4-00c04fd430c8 NAME "test" TYPE SHA1)
message(STATUS "UUID SHA1: ${uuid1}")

string(UUID uuid2 NAMESPACE 6ba7b810-9dad-11d1-80b4-00c04fd430c8 NAME "test" TYPE MD5)
message(STATUS "UUID MD5: ${uuid2}")

string(UUID uuid3 NAMESPACE 6ba7b811-9dad-11d1-80b4-00c04fd430c8 NAME "different" TYPE SHA1 UPPER)
message(STATUS "UUID UPPER: ${uuid3}")
