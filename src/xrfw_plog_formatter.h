#pragma once
#include <iomanip>
#include <plog/Log.h>

namespace plog {
class MyFormatter {
public:
  static util::nstring header() // This method returns a header for a new file.
                                // In our case it is empty.
  {
    return util::nstring();
  }

  static util::nstring
  format(const Record &record) // This method returns a string from a record.
  {
    tm t;
    util::localtime_s(&t, &record.getTime().time);

    util::nostringstream ss;
    ss << "["
       // hour
       << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_hour
       << PLOG_NSTR(":")
       // min
       << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_min
       << PLOG_NSTR(".")
       // sec
       << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_sec
       << "]"
       // message
       << record.getFunc() << ": " << record.getMessage() << "\n";

    return ss.str();
  }
};
} // namespace plog
