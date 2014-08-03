#ifndef CONTOURPP_OPTIONPARSER_H__
#define CONTOURPP_OPTIONPARSER_H__

#include <algorithm>
#include <iostream>
#include "optionparser.h"

struct Arg: public option::Arg
{
  static void printError(const char* msg1, const option::Option& opt, const char* msg2)
  {
    std::cerr << msg1;
    if (opt.name && (opt.namelen > 0))
      std::copy(opt.name, opt.name + opt.namelen, std::ostream_iterator<char>(std::cerr));
    else
      std::cerr << "\"\"";
    std::cerr << msg2;
  }

  static option::ArgStatus Unknown(const option::Option& option, bool msg)
  {
    if (msg)
      printError("Unknown option '", option, "'\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Required(const option::Option& option, bool msg)
  {
    if (option.arg != 0)
      return option::ARG_OK;

    if (msg)
      printError("Option '", option, "' requires an argument\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
  {
    if (option.arg != 0 && option.arg[0] != 0)
      return option::ARG_OK;

    if (msg)
      printError("Option '", option, "' requires a non-empty argument\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Numeric(const option::Option& option, bool msg)
  {
    char* endptr = 0;
    if (option.arg != 0 && strtol(option.arg, &endptr, 10))
      if (endptr != option.arg && *endptr == 0)
        return option::ARG_OK;

    if (msg)
      printError("Option '", option, "' requires a numeric argument\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus TimeDuration(const option::Option& option, bool msg)
  {
    const char* str = option.arg;
    const char* endptr = NULL;

    if (!str || !str[0]) {
      if (msg)
        printError("Option '", option, "' has to be formatted as [-]HH:MM[:SS]\n");
      return option::ARG_ILLEGAL;
    }

    if (str[0] == '+' || str[0] == '-')
      ++str;

    endptr = str;
    while ((*endptr >= '0') && (*endptr <= '9'))
        ++endptr; // hours
    if ((endptr <= str) || (*endptr != ':')) {
      if (msg)
        printError("Option '", option, "' has to be formatted as [-]HH:MM[:SS]\n");
      return option::ARG_ILLEGAL;
    }

    str = ++endptr;
    while ((*endptr >= '0') && (*endptr <= '9'))
        ++endptr; // minutes

    if (str < endptr) {
      if (*endptr == ':') { // seconds
        str = ++endptr;
        while ((*endptr >= '0') && (*endptr <= '9')) ++endptr;
      }
      if ((str < endptr) && (*endptr == 0))
        return option::ARG_OK;
    }

    if (msg)
      printError("Option '", option, "' has to be formatted as [-]HH:MM[:SS]\n");
    return option::ARG_ILLEGAL;
  }
};


enum optionIndex {
  UNKNOWN       = 0,
  HELP          = UNKNOWN + 1,
  LOWLEVEL      = HELP + 1,
  OLDFORMAT     = LOWLEVEL + 1,
  AFTERMEALONLY = OLDFORMAT + 1,
  INFILE        = AFTERMEALONLY + 1,
  TIMESHIFT     = INFILE + 1
};


static const option::Descriptor usage[] =
{
  {UNKNOWN,       0, "" , "",                Arg::None,
    "USAGE: contourpp [options] [input file(s)]\n\nOptions:" },

  {HELP,          0, "h", "help",            Arg::None,
    "  -h  \t--help  \tPrint usage and exit." },

  {LOWLEVEL,      0, "l", "lowlevel-api",    Arg::None,
    "  -l  \t--lowlevel-api  \tUse the low-level API. Useful for debugging." },

  {OLDFORMAT,     0, "B", "bayer-format",    Arg::None,
    "  -B  \t--bayer-format  \tPrint output in the Bayer format (as given by the meter)." },

  {AFTERMEALONLY, 0, "a", "after-meal-only", Arg::None,
    "  -a  \t--after-meal-only  \tFilter entries with after meal hours." },

  {INFILE,        0, "f", "input-file",      Arg::NonEmpty,
    "  -f <input_file>  \t--input-file=<input_file>  \tRead the entries from the infile." },

  {TIMESHIFT,     0, "t", "time-shift",      Arg::TimeDuration,
    "  -t <timeshift>  \t--time-shift=<timeshift>  \tShift the time of each reading (timeshift format: [-]HH:MM[:SS])." },

  {UNKNOWN,       0, "" , "",                Arg::None,
    "\nExamples:\n"
    "  contourpp                          Get readings from the Contour USB meter and output them in csv.\n"
    "  contourpp -t 04:00 readings.txt    Get readings from \"readings.txt\" and correct time by shifting them by 4 hours.\n"
    "  contourpp -a readings.txt          Filter readings from \"readings.txt\", printing only the ones with after meal hours.\n" },
  {0,0,0,0,0,0}
 };

#endif
