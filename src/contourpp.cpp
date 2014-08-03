#include <iostream>
#include <fstream>
#include <exception>
#include <string>
#include <vector>
#include "hid_commands.hpp"
#include "contourpp_driver.hpp"
#include "contourpp_optionparser.hpp"

void lowLevelAPI()
{
  contourpp::interface device;
  const char *begin_text = NULL, *end_text = NULL;
  //std::string text;
  std::ostream_iterator<char> oiter(std::cout);
  while (device.sync(begin_text, end_text)) {
    //text.clear();
    std::copy(begin_text, end_text, oiter);
    *oiter = '\n';
    ++oiter;
    //std::cout << text << std::endl;
  }
}


void highLevelAPI(std::vector<const char*> const& filenames, bool print_bayer_format,
  const boost::posix_time::time_duration& d, bool only_after_meal_hours)
{
  contourpp::record_parser parser;
  std::vector<contourpp::record> records;

  records.reserve(128);

  if (!filenames.empty()) {
    for (std::vector<const char*>::const_iterator f = filenames.begin(); f != filenames.end(); ++f) {
      std::ifstream ifs(*f);
      if (!ifs.good())
        throw std::runtime_error(std::string("could not open '") + (*f) + "'");

      std::string buf;
      contourpp::record rec;
      while (std::getline(ifs, buf)) {
        if (parser.parse(buf.data(), buf.data() + buf.size(), rec)) {
          if (records.size() == records.capacity())
            records.reserve(2 * records.capacity());
          records.push_back(rec);
        }
      }
    }
  }
  else
    parser.get_all(records);

  std::vector<contourpp::record>::iterator i, e(records.end());

  if (d.total_seconds() != 0) {
    for (i = records.begin(); i != e; ++i)
      i->shift_time(d);
  }
  if (print_bayer_format) {
    for (i = records.begin(); i != e; ++i) {
      if (!only_after_meal_hours || i->min_after_meal()) {
        i->print_bayer(std::cout);
        std::cout << std::endl;
      }
    }
  }
  else {
    for (i = records.begin(); i != e; ++i)
      if (!only_after_meal_hours || i->min_after_meal())
        std::cout << (*i) << std::endl;
  }
}

int main(int argc, char* argv[])
{
  option::Stats stats(usage, argc - 1, argv + 1);
  std::vector<option::Option> options(stats.options_max), buffer(stats.buffer_max);
  option::Parser optionparser(usage, argc - 1, argv + 1, options.data(), buffer.data());
  if (optionparser.error())
    return -1;

  if (options[HELP]) {
    option::printUsage(std::cout, usage, 10000);
    return 0;
  }

  std::vector<const char*> filenames;
  for (const option::Option* opt = options[INFILE]; opt; opt = opt->next())
    filenames.push_back(opt->arg);
  for (int i = 0; i < optionparser.nonOptionsCount(); i++)
    filenames.push_back(optionparser.nonOption(i));

  boost::posix_time::time_duration d(0, 0, 0, 0);
  for (const option::Option* opt = options[TIMESHIFT]; opt; opt = opt->next())
    d += boost::posix_time::duration_from_string(opt->arg);

  try {
    if (options[LOWLEVEL])
      lowLevelAPI();
    else
      highLevelAPI(filenames, options[OLDFORMAT], d, options[AFTERMEALONLY]);
  } catch(std::runtime_error e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  return 0;
}
